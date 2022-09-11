//Chris Xiong 2022
//License: MPL-2.0
#ifndef SIGNATURE_DB_HPP
#define SIGNATURE_DB_HPP

#include <filesystem>
#include <vector>

#include "signature.hpp"

namespace fs = std::filesystem;

struct subslice_t {size_t id; size_t slice;};

struct dupe_t {size_t id1, id2; double distance;};

struct signature_db_priv;

class signature_db
{
private:
    signature_db_priv *p;
public:
    signature_db();
    ~signature_db();

    //insert image signature into database
    //id must be unique
    void put_signature(size_t id, const fs::path &path, const signature &sig);
    //get image signature from database
    std::pair<fs::path, signature> get_signature(size_t id);

    //place batch_put_subslice_begin() and batch_end() around a group of
    //put_subslice() calls to improve performance
    void batch_put_subslice_begin();
    //insert subslice into database
    //(id, slice) must be unique
    //calling put_subslice_begin() before this is NOT required, but
    //will improve performance
    void put_subslice(size_t id, size_t slice, const signature &slicesig);

    //same thing as put_subslice_begin()
    void batch_find_subslice_begin();
    //find identical subslices from database
    std::vector<subslice_t> find_subslice(const signature &slicesig);

    //call this to finish a batch
    void batch_end();

    void put_dupe_pair(size_t ida, size_t idb, double dist);
    std::vector<dupe_t> dupe_pairs();

    void lock();
    void unlock();

    bool to_db_file(const fs::path &path);
    bool from_db_file(const fs::path &path);
};

#endif
