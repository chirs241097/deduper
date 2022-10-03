//Chris Xiong 2022
//License: MPL-2.0
#ifndef SIGDB_QT_HPP
#define SIGDB_QT_HPP

#include <filesystem>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

#include <QObject>

#include "signature_db.hpp"

namespace fs = std::filesystem;

class SignatureDB : public QObject
{
    Q_OBJECT
private:
    signature_db *sdb;
    std::unordered_map<size_t, fs::path> fmap;
    std::unordered_map<fs::path, size_t> frmap;
    std::map<std::pair<size_t, size_t>, double> distmap;
    std::vector<std::vector<size_t>> groups;
    populate_cfg_t cfg;

    void create_priv_struct();
public:
    SignatureDB();
    SignatureDB(const fs::path &dbpath);
    ~SignatureDB();

    bool valid();
    bool is_dirty();

    populate_cfg_t get_sig_config();
    void set_sig_config(populate_cfg_t cfg);

    void scan_files(const std::vector<fs::path> &files);
    void interrupt_scan();

    std::vector<std::pair<size_t, double>> search_file(const fs::path &files);

    size_t num_groups();
    std::vector<size_t> get_group(size_t gid);
    std::map<std::pair<size_t, size_t>, double> group_distances(size_t gid);

    std::vector<size_t> get_image_ids();
    fs::path get_image_path(size_t id);
    size_t get_path_id(const fs::path &p);

    bool load(const fs::path &p);
    bool save(const fs::path &p);

    int64_t db_memory_usage();
Q_SIGNALS:
    void image_scanned(size_t n);
};

#endif
