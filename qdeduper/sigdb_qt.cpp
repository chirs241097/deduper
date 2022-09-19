#include "sigdb_qt.hpp"
#include "signature_db.hpp"

#include <algorithm>

signature_config cfg_full =
{
    9,      //slices
    3,      //blur_window
    2,      //min_window
    true,   //crop
    true,   //comp
    0.5,    //pr
    1./128, //noise_threshold
    0.05,   //contrast_threshold
    0.25    //max_cropping
};

signature_config cfg_subslice =
{
    4,      //slices
    16,     //blur_window
    2,      //min_window
    false,  //crop
    true,   //comp
    0.5,    //pr
    1./64,  //noise_threshold
    0.05,   //contrast_threshold
    0.25    //max_cropping
};

SignatureDB::SignatureDB() : QObject(nullptr)
{
    sdb = new signature_db();
}

SignatureDB::SignatureDB(const fs::path &dbpath) : QObject(nullptr)
{
    sdb = new signature_db(dbpath);
}

SignatureDB::~SignatureDB()
{
    delete sdb;
}

void SignatureDB::scan_files(const std::vector<fs::path> &files, int njobs)
{
    populate_cfg_t pcfg = {
        3,
        3,
        cfg_full,
        cfg_subslice,
        0.3,
        [this](size_t c,int){Q_EMIT image_scanned(c);},
        njobs
    };
    sdb->populate(files, pcfg);

    Q_EMIT image_scanned(~size_t(0));
    
    auto ids = sdb->get_image_ids();
    sdb->batch_get_signature_begin();
    for (auto &id : ids)
    {
        fs::path p;
        std::tie(p, std::ignore) = sdb->get_signature(id);
        fmap[id] = p;
        frmap[p] = id;
    }
    sdb->batch_get_signature_end();

    auto dupes = sdb->dupe_pairs();
    for (auto &dupe : dupes)
        distmap[std::make_pair(dupe.id1, dupe.id2)] = dupe.distance;

    sdb->group_similar();
    auto gps = sdb->groups_get();
    gps.erase(std::remove_if(gps.begin(), gps.end(), [](std::vector<size_t> v){ return v.size() < 2; }), gps.end());
    this->groups = std::move(gps);
}

size_t SignatureDB::num_groups()
{
    return groups.size();
}

std::vector<size_t> SignatureDB::get_group(size_t gid)
{
    if (gid >= groups.size()) return {};
    return groups[gid];
}

std::map<std::pair<size_t, size_t>, double> SignatureDB::group_distances(size_t gid)
{
    std::map<std::pair<size_t, size_t>, double> ret;
    auto g = get_group(gid);
    for (size_t i = 0; i < g.size(); ++i)
        for (size_t j = i + 1; j < g.size(); ++j)
        {
            size_t x = g[i], y = g[j];
            if (distmap.find(std::make_pair(x, y)) != distmap.end())
                ret[std::make_pair(i, j)] = distmap[std::make_pair(x, y)];
            else if (distmap.find(std::make_pair(y, x)) != distmap.end())
                ret[std::make_pair(i, j)] = distmap[std::make_pair(y, x)];
        }
    return ret;
}

fs::path SignatureDB::get_image_path(size_t id)
{
    if (fmap.find(id) == fmap.end())
        return fs::path();
    else return fmap[id];
}

size_t SignatureDB::get_path_id(const fs::path& p)
{
    if (frmap.find(p) == frmap.end())
        return ~size_t(0);
    else return frmap[p];
}
