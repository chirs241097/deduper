//Chris Xiong 2022
//License: MPL-2.0
#ifndef FILESCANNER_HPP
#define FILESCANNER_HPP

#include <atomic>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include <QObject>

namespace fs = std::filesystem;

class FileScanner : public QObject
{
    Q_OBJECT
    std::vector<std::string> mn;
    std::vector<std::pair<fs::path, bool>> paths;
    std::vector<fs::path> ret;
    std::size_t maxmnlen;
    std::atomic<bool> interrpt;
public:
    FileScanner();
    void add_magic_number(const std::string &m);
    void add_path(const fs::path &p, bool recurse = false);
    void scan();
    void interrupt();
    bool interrupted();
    std::vector<fs::path> file_list();
Q_SIGNALS:
    void scan_done_prep(std::size_t nfiles);
    void file_scanned(const fs::path &p, std::size_t n);
};

#endif
