//Chris Xiong 2022
//License: MPL-2.0
#ifndef SIGNATURE_DB_HPP
#define SIGNATURE_DB_HPP

#include <functional>
#include <filesystem>
#include <vector>

#include "signature.hpp"

namespace fs = std::filesystem;

struct subslice_t {size_t id; size_t slice;};

struct dupe_t {size_t id1, id2; double distance;};

struct populate_cfg_t
{
    size_t nsliceh;
    size_t nslicev;
    signature_config scfg_full;
    signature_config scfg_subslice;
    double threshold;
    std::function<void(size_t, int)> callback;
    int njobs;
};

struct signature_db_priv;

class signature_db
{
private:
    signature_db_priv *p;
public:
    //open a signature database
    //if dbpath is an empty path (default), the database will reside in RAM
    //and will be automatically initialized
    //otherwise it opens the database specified by dbpath
    //if the database specified by dbpath doesn't exist, it will be created
    //and initialized
    //if the database file exists but is not a valid signature database, it
    //will be immediately closed and any subsequent calls to this signature db
    //object will do nothing. The object will be marked invalid.
    signature_db(const fs::path &dbpath = fs::path());
    ~signature_db();

    bool valid();

    //insert image signature into database
    //if id is omitted, it's assigned automatically and returned
    //if specificted, id must be unique
    //treat automatically assigned id as arbitrary opaque integers
    size_t put_signature(const fs::path &path, const signature &sig, size_t id = ~size_t(0));
    //get image signature from database
    std::pair<fs::path, signature> get_signature(size_t id);

    //place batch_put_subslice_begin() and batch_put_subslice_end() around a group of
    //put_subslice() calls to improve performance
    void batch_put_subslice_begin();
    //insert subslice into database
    //(id, slice) must be unique
    //calling put_subslice_begin() before this is NOT required, but
    //will improve performance
    void put_subslice(size_t id, size_t slice, const signature &slicesig);
    void batch_put_subslice_end();

    //same thing as put_subslice_begin()
    void batch_find_subslice_begin();
    //find identical subslices from database
    std::vector<subslice_t> find_subslice(const signature &slicesig);
    void batch_find_subslice_end();

    void put_dupe_pair(size_t ida, size_t idb, double dist);
    std::vector<dupe_t> dupe_pairs();

    void lock();
    void unlock();

    bool to_db_file(const fs::path &path);
    bool from_db_file(const fs::path &path);

    void populate(const std::vector<fs::path> &paths, const populate_cfg_t &cfg);

    //disjoint set for keeping similar images in the same group
    //some of these probably shouldn't be public. TBD...
    void ds_init();

    void batch_ds_get_parent_begin();
    size_t ds_get_parent(size_t id);
    void batch_ds_get_parent_end();

    void batch_ds_set_parent_begin();
    size_t ds_set_parent(size_t id, size_t par);
    void batch_ds_set_parent_end();

    size_t ds_find(size_t id);
    void ds_merge(size_t id1, size_t id2);

    void group_similar();
    std::vector<std::vector<size_t>> groups_get();
};

#endif
