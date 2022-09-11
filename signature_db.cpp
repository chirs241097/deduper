//Chris Xiong 2022
//License: MPL-2.0
#include <sqlite3.h>

#include "signature_db.hpp"

enum batch_status
{
    single = 0,
    putsub,
    findsub
};

struct signature_db_priv
{
    sqlite3 *db;
    sqlite3_mutex *mtx;
    sqlite3_stmt *bst;
    batch_status batch_mode;
};

signature_db::signature_db()
{
    p = new signature_db_priv();
    sqlite3_open(":memory:", &p->db);
    p->mtx = sqlite3_db_mutex(p->db);
    sqlite3_exec(p->db, R"sql(
        create table images(
            id int primary key,
            path text,
            signature text
        );
    )sql", nullptr, nullptr, nullptr);
    sqlite3_exec(p->db, R"sql(
        create table subslices(
            image int,
            slice int,
            slicesig text
        );
    )sql", nullptr, nullptr, nullptr);
    sqlite3_exec(p->db, R"sql(
        create index ssidx on subslices(slicesig);
    )sql", nullptr, nullptr, nullptr);
    sqlite3_exec(p->db, R"sql(
        create table dupes(
            id1 int,
            id2 int,
            dist real,
            primary key (id1, id2)
        );
    )sql", nullptr, nullptr, nullptr);
    p->bst = nullptr;
    p->batch_mode = batch_status::single;
}

signature_db::~signature_db()
{
    if (p->bst)
        sqlite3_finalize(p->bst);
    sqlite3_close(p->db);
    delete p;
}

void signature_db::put_signature(size_t id, const fs::path &path, const signature &sig)
{
    sqlite3_stmt *st;
    std::string sigs = sig.to_string();
    sqlite3_prepare_v2(p->db, "insert into signatures (id, path, signature) values(?, ?, ?);", -1, &st, 0);
    sqlite3_bind_int(st, 1, id);
#if PATH_VALSIZE == 2
    sqlite3_bind_text16(st, 2, path.c_str(), -1, nullptr);
#else
    sqlite3_bind_text(st, 2, path.c_str(), -1, nullptr);
#endif
    sqlite3_bind_text(st, 3, sigs.c_str(), -1, nullptr);
    sqlite3_step(st);
    sqlite3_finalize(st);
}

std::pair<fs::path, signature> signature_db::get_signature(size_t id)
{
    sqlite3_stmt *st;
    sqlite3_prepare_v2(p->db, "select path, signature from signatures where id = ?;", -1, &st, 0);
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
    p->batch_mode = batch_status::putsub;
    sqlite3_prepare_v2(p->db, "insert into subslices (image, slice, slicesig) values(?, ?, ?);", -1, &p->bst, 0);
}

void signature_db::put_subslice(size_t id, size_t slice, const signature &slicesig)
{
    sqlite3_stmt *st = nullptr;
    if (p->batch_mode == batch_status::putsub)
        st = p->bst;
    else
        sqlite3_prepare_v2(p->db, "insert into subslices (image, slice, slicesig) values(?, ?, ?);", -1, &st, 0);
    sqlite3_bind_int(st, 1, id);
    sqlite3_bind_int(st, 2, slice);
    std::string slicesigs = slicesig.to_string();
    sqlite3_bind_text(st, 3, slicesigs.c_str(), -1, nullptr);
    sqlite3_step(st);
    if (p->batch_mode == batch_status::putsub)
        sqlite3_reset(st);
    else
        sqlite3_finalize(st);
}

void signature_db::batch_find_subslice_begin()
{
    p->batch_mode = batch_status::findsub;
    sqlite3_prepare_v2(p->db, "select image, slice from subslices where slicesig = ?;", -1, &p->bst, 0);
}

std::vector<subslice_t> signature_db::find_subslice(const signature &slicesig)
{
    sqlite3_stmt *st = nullptr;
    if (p->batch_mode == batch_status::findsub)
        st = p->bst;
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
    if (p->batch_mode == batch_status::findsub)
        sqlite3_reset(st);
    else
        sqlite3_finalize(st);
    return ret;
}

void signature_db::batch_end()
{
    p->batch_mode = batch_status::single;
    sqlite3_finalize(p->bst);
    p->bst = nullptr;
}

void signature_db::put_dupe_pair(size_t ida, size_t idb, double dist)
{
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
{sqlite3_mutex_enter(p->mtx);}
void signature_db::unlock()
{sqlite3_mutex_leave(p->mtx);}

bool signature_db::to_db_file(const fs::path &path)
{
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
