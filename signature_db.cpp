//Chris Xiong 2022
//License: MPL-2.0
#include <sqlite3.h>

#include <atomic>
#include <set>

#include "signature_db.hpp"
#include "subslice_signature.hpp"
#include "thread_pool.hpp"

const int SIGDB_VERSION = 3;

enum batch_status
{
    none = 0,
    putsub,
    findsub,
    setpar,
    getpar,

    BATCH_STATUS_MAX
};

struct signature_db_priv
{
    sqlite3 *db;
    sqlite3_mutex *mtx;
    sqlite3_stmt *bst[batch_status::BATCH_STATUS_MAX];

    void init_db();
    bool verify_db();

    void batch_end(batch_status s);
};

void signature_db_priv::init_db()
{
    sqlite3_exec(db, R"sql(
        create table sigdbinfo(
            version int
        );
    )sql", nullptr, nullptr, nullptr);
    sqlite3_stmt *vst;
    sqlite3_prepare_v2(db, "insert into sigdbinfo (version) values(?);", -1, &vst, 0);
    sqlite3_bind_int(vst, 1, SIGDB_VERSION);
    sqlite3_step(vst);
    sqlite3_finalize(vst);

    sqlite3_exec(db, R"sql(
        create table images(
            id integer primary key,
            path text,
            signature text
        );
    )sql", nullptr, nullptr, nullptr);
    sqlite3_exec(db, R"sql(
        create table subslices(
            image integer,
            slice integer,
            slicesig text,
            primary key (image, slice),
            foreign key (image) references images (id)
        );
    )sql", nullptr, nullptr, nullptr);
    sqlite3_exec(db, R"sql(
        create index ssidx on subslices(slicesig);
    )sql", nullptr, nullptr, nullptr);
    sqlite3_exec(db, R"sql(
        create table dupes(
            id1 integer,
            id2 integer,
            dist real,
            primary key (id1, id2),
            constraint fk_ids foreign key (id1, id2) references images (id, id)
        );
    )sql", nullptr, nullptr, nullptr);
    sqlite3_exec(db, R"sql(
        create table dspar(
            id integer,
            parent integer,
            constraint fk_ids foreign key (id, parent) references images (id, id)
        );
    )sql", nullptr, nullptr, nullptr);
}

bool signature_db_priv::verify_db()
{
    sqlite3_stmt *vst;
    sqlite3_prepare_v2(db, "select version from sigdbinfo;", -1, &vst, 0);
    if (sqlite3_step(vst) != SQLITE_ROW) {sqlite3_finalize(vst); return false;}
    if (SIGDB_VERSION != sqlite3_column_int(vst, 0)) {sqlite3_finalize(vst); return false;}
    sqlite3_finalize(vst);
    return true;
}

void signature_db_priv::batch_end(batch_status s)
{
    if (!db || !bst[s]) [[ unlikely ]] return;
    sqlite3_finalize(bst[s]);
    bst[s] = nullptr;
}

signature_db::signature_db(const fs::path &dbpath)
{
    p = new signature_db_priv();
    if (dbpath.empty())
    {
        sqlite3_open(":memory:", &p->db);
        p->init_db();
    }
    else
    {
        bool need_init = !fs::is_regular_file(dbpath);
#if PATH_VALSIZE == 2
        sqlite3_open16(dbpath.c_str(), &p->db);
#else
        sqlite3_open(dbpath.c_str(), &p->db);
#endif
        if (need_init) p->init_db();
    }

    p->mtx = sqlite3_db_mutex(p->db);
    for (int i = 0; i < batch_status::BATCH_STATUS_MAX; ++i)
        p->bst[i] = nullptr;
    if (!p->verify_db())
    {
        sqlite3_close(p->db);
        p->db = nullptr;
        p->mtx = nullptr;
    }
}

signature_db::~signature_db()
{
    if (!p->db) [[ unlikely ]]
    {
        delete p;
        return;
    }
    for (int i = 0; i < batch_status::BATCH_STATUS_MAX; ++i)
        if (p->bst[i])
            sqlite3_finalize(p->bst[i]);
    sqlite3_close(p->db);
    delete p;
}

bool signature_db::valid()
{ return static_cast<bool>(p->db); }

size_t signature_db::put_signature(const fs::path &path, const signature &sig,size_t id)
{
    if (!p->db) [[ unlikely ]] return ~size_t(0);
    sqlite3_stmt *st;
    std::string sigs = sig.to_string();
    sqlite3_prepare_v2(p->db, "insert into images (id, path, signature) values(?, ?, ?);", -1, &st, 0);
    if (!~id)
        sqlite3_bind_null(st, 1);
    else
        sqlite3_bind_int(st, 1, id);
#if PATH_VALSIZE == 2
    sqlite3_bind_text16(st, 2, path.c_str(), -1, nullptr);
#else
    sqlite3_bind_text(st, 2, path.c_str(), -1, nullptr);
#endif
    sqlite3_bind_text(st, 3, sigs.c_str(), -1, nullptr);
    sqlite3_step(st);
    sqlite3_finalize(st);
    return static_cast<size_t>(sqlite3_last_insert_rowid(p->db));
}

std::pair<fs::path, signature> signature_db::get_signature(size_t id)
{
    if (!p->db) [[ unlikely ]] return std::make_pair(fs::path(), signature());
    sqlite3_stmt *st;
    sqlite3_prepare_v2(p->db, "select path, signature from images where id = ?;", -1, &st, 0);
    sqlite3_bind_int(st, 1, id);
    int rr = sqlite3_step(st);
    if (rr == SQLITE_ROW)
    {
#if PATH_VALSIZE == 2
        fs::path path((wchar_t*)sqlite3_column_text16(st, 0));
#else
        fs::path path((char*)sqlite3_column_text(st, 0));
#endif
        std::string sigs((char*)sqlite3_column_text(st, 1));
        sqlite3_finalize(st);
        return std::make_pair(path, signature::from_string(std::move(sigs)));
    }
    else
    {
        sqlite3_finalize(st);
        return std::make_pair(fs::path(), signature());
    }
}

void signature_db::batch_put_subslice_begin()
{
    if (!p->db) [[ unlikely ]] return;
    sqlite3_prepare_v2(p->db, "insert into subslices (image, slice, slicesig) values(?, ?, ?);", -1, &p->bst[batch_status::putsub], 0);
}

void signature_db::put_subslice(size_t id, size_t slice, const signature &slicesig)
{
    if (!p->db) [[ unlikely ]] return;
    sqlite3_stmt *st = nullptr;
    if (p->bst[batch_status::putsub])
        st = p->bst[batch_status::putsub];
    else
        sqlite3_prepare_v2(p->db, "insert into subslices (image, slice, slicesig) values(?, ?, ?);", -1, &st, 0);
    sqlite3_bind_int(st, 1, id);
    sqlite3_bind_int(st, 2, slice);
    std::string slicesigs = slicesig.to_string();
    sqlite3_bind_text(st, 3, slicesigs.c_str(), -1, nullptr);
    sqlite3_step(st);
    if (p->bst[batch_status::putsub])
        sqlite3_reset(st);
    else
        sqlite3_finalize(st);
}

void signature_db::batch_put_subslice_end()
{
    p->batch_end(batch_status::putsub);
}

void signature_db::batch_find_subslice_begin()
{
    if (!p->db) [[ unlikely ]] return;
    sqlite3_prepare_v2(p->db, "select image, slice from subslices where slicesig = ?;", -1, &p->bst[batch_status::findsub], 0);
}

std::vector<subslice_t> signature_db::find_subslice(const signature &slicesig)
{
    if (!p->db) [[ unlikely ]] return {};
    sqlite3_stmt *st = nullptr;
    if (p->bst[batch_status::findsub])
        st = p->bst[batch_status::findsub];
    else
        sqlite3_prepare_v2(p->db, "select image, slice from subslices where slicesig = ?;", -1, &st, 0);

    std::string slicesigs = slicesig.to_string();
    sqlite3_bind_text(st, 1, slicesigs.c_str(), -1, nullptr);

    std::vector<subslice_t> ret;
    while (1)
    {
        int r = sqlite3_step(st);
        if (r != SQLITE_ROW) break;
        size_t im = sqlite3_column_int(st, 0);
        size_t sl = sqlite3_column_int(st, 1);
        ret.push_back({im, sl});
    }
    if (p->bst[batch_status::findsub])
        sqlite3_reset(st);
    else
        sqlite3_finalize(st);
    return ret;
}

void signature_db::batch_find_subslice_end()
{
    p->batch_end(batch_status::findsub);
}

void signature_db::put_dupe_pair(size_t ida, size_t idb, double dist)
{
    if (!p->db) [[ unlikely ]] return;
    sqlite3_stmt *st = nullptr;
    sqlite3_prepare_v2(p->db, "insert into dupes (id1, id2, dist) values(?, ?, ?);", -1, &st, 0);
    sqlite3_bind_int(st, 1, ida);
    sqlite3_bind_int(st, 2, idb);
    sqlite3_bind_double(st, 3, dist);
    sqlite3_step(st);
    sqlite3_finalize(st);
}
std::vector<dupe_t> signature_db::dupe_pairs()
{
    if (!p->db) [[ unlikely ]] return {};
    sqlite3_stmt *st = nullptr;
    sqlite3_prepare_v2(p->db, "select id1, id2, dist from dupes;", -1, &st, 0);
    std::vector<dupe_t> ret;
    while (1)
    {
        int r = sqlite3_step(st);
        if (r != SQLITE_ROW) break;
        ret.push_back({
            (size_t)sqlite3_column_int(st, 0),
            (size_t)sqlite3_column_int(st, 1),
            sqlite3_column_double(st, 2)
        });
    }
    sqlite3_finalize(st);
    return ret;
}

void signature_db::lock()
{
    if (!p->db) [[ unlikely ]] return;
    sqlite3_mutex_enter(p->mtx);
}
void signature_db::unlock()
{
    if (!p->db) [[ unlikely ]] return;
    sqlite3_mutex_leave(p->mtx);
}

bool signature_db::to_db_file(const fs::path &path)
{
    if (!p->db) [[ unlikely ]] return false;
    sqlite3 *dest;
    int r;
#if PATH_VALSIZE == 2
    r = sqlite3_open16(path.c_str(), &dest);
#else
    r = sqlite3_open(path.c_str(), &dest);
#endif
    if (r != SQLITE_OK) return false;
    sqlite3_backup *bk = sqlite3_backup_init(dest, "main", p->db, "main");
    bool ret = (bk != nullptr);
    while (ret)
    {
        r = sqlite3_backup_step(bk, -1);
        if (r == SQLITE_DONE) break;
        else if (r != SQLITE_OK)
            ret = false;
    }
    ret &= (SQLITE_OK == sqlite3_backup_finish(bk));
    ret &= (SQLITE_OK == sqlite3_close(dest));
    return ret;
}
bool signature_db::from_db_file(const fs::path &path)
{
    if (!p->db) [[ unlikely ]] return false;
    sqlite3 *src;
    int r;
#if PATH_VALSIZE == 2
    r = sqlite3_open16(path.c_str(), &src);
#else
    r = sqlite3_open(path.c_str(), &src);
#endif
    if (r != SQLITE_OK) return false;
    sqlite3_backup *bk = sqlite3_backup_init(p->db, "main", src, "main");
    bool ret = (bk != nullptr);
    while (ret)
    {
        r = sqlite3_backup_step(bk, -1);
        if (r == SQLITE_DONE) break;
        else if (r != SQLITE_OK)
            ret = false;
    }
    ret &= (SQLITE_OK == sqlite3_backup_finish(bk));
    ret &= (SQLITE_OK == sqlite3_close(src));
    return ret;
}

void signature_db::populate(const std::vector<fs::path> &paths, const populate_cfg_t &cfg)
{
    std::atomic<size_t> count(0);
    auto job_func = [&, this](int thid, const fs::path& path)
    {
        subsliced_signature ss = subsliced_signature::from_path(path, cfg.nsliceh, cfg.nslicev, cfg.scfg_full, cfg.scfg_subslice);

        this->lock();
        std::set<size_t> v;
        size_t dbid = this->put_signature(path, ss.full);

        this->batch_find_subslice_begin();
        for (size_t i = 0; i < cfg.nsliceh * cfg.nslicev; ++i)
        {
            std::vector<subslice_t> ssmatches = this->find_subslice(ss.subslices[i]);
            for (auto &match : ssmatches)
            {
                if (match.slice == i && v.find(match.id) == v.end())
                {
                    signature othersig;
                    std::tie(std::ignore, othersig) = this->get_signature(match.id);
                    double dist = ss.full.distance(othersig);
                    if (dist < cfg.threshold)
                        this->put_dupe_pair(dbid, match.id, dist);
                }
            }
        }
        this->batch_find_subslice_end();

        this->batch_put_subslice_begin();
        for (size_t i = 0; i < cfg.nsliceh * cfg.nslicev; ++i)
            this->put_subslice(dbid, i, ss.subslices[i]);
        this->batch_put_subslice_end();

        this->unlock();
        ++count;
        cfg.callback(count.load(), thid);
    };

    thread_pool tp(cfg.njobs);
    for(size_t i = 0; i < paths.size(); ++i)
    {
        tp.create_task(job_func, paths[i]);
    }
    tp.wait();
}

void signature_db::ds_init()
{
    sqlite3_exec(p->db, R"sql(
        delete from dspar;
        insert into dspar (id, parent) select id, id from images;
    )sql", nullptr, nullptr, nullptr);
}

void signature_db::batch_ds_get_parent_begin()
{
    if (!p->db) [[ unlikely ]] return;
    sqlite3_prepare_v2(p->db, "select parent from dspar where id = ?;", -1, &p->bst[batch_status::getpar], 0);
}

size_t signature_db::ds_get_parent(size_t id)
{
    if (!p->db) [[ unlikely ]] return ~size_t(0);
    sqlite3_stmt *st = nullptr;
    if (p->bst[batch_status::getpar])
        st = p->bst[batch_status::getpar];
    else
        sqlite3_prepare_v2(p->db, "select parent from dspar where id = ?;", -1, &st, 0);

    sqlite3_bind_int(st, 1, id);

    size_t ret = ~size_t(0);
    if (sqlite3_step(st) == SQLITE_ROW)
        ret = sqlite3_column_int(st, 0);

    if (p->bst[batch_status::getpar])
        sqlite3_reset(st);
    else
        sqlite3_finalize(st);
    return ret;
}

void signature_db::batch_ds_get_parent_end()
{
    p->batch_end(batch_status::getpar);
}

void signature_db::batch_ds_set_parent_begin()
{
    if (!p->db) [[ unlikely ]] return;
    sqlite3_prepare_v2(p->db, "update dspar set parent = ? where id = ?;", -1, &p->bst[batch_status::setpar], 0);
}

size_t signature_db::ds_set_parent(size_t id, size_t par)
{
    if (!p->db) [[ unlikely ]] return par;
    sqlite3_stmt *st = nullptr;
    if (p->bst[batch_status::setpar])
        st = p->bst[batch_status::setpar];
    else
        sqlite3_prepare_v2(p->db, "update dspar set parent = ? where id = ?;", -1, &st, 0);

    sqlite3_bind_int(st, 1, par);
    sqlite3_bind_int(st, 2, id);

    sqlite3_step(st);

    if (p->bst[batch_status::setpar])
        sqlite3_reset(st);
    else
        sqlite3_finalize(st);
    return par;
}

void signature_db::batch_ds_set_parent_end()
{
    p->batch_end(batch_status::setpar);
}

size_t signature_db::ds_find(size_t id)
{
    size_t p = ds_get_parent(id);
    if (id != p)
        return ds_set_parent(id, ds_find(p));
    return id;
}

void signature_db::ds_merge(size_t id1, size_t id2)
{
    id1 = ds_find(id1);
    id2 = ds_find(id2);
    ds_set_parent(id1, id2);
}

void signature_db::group_similar()
{
}

std::vector<std::vector<size_t>> signature_db::groups_get()
{
    return {};
}
