//Chris Xiong 2022
//License: MPL-2.0
#include "filescanner.hpp"

#include <cstring>
#include <algorithm>
#include <fstream>

using std::size_t;

FileScanner::FileScanner() : QObject(nullptr), maxmnlen(0), interrpt(false)
{
}

void FileScanner::add_magic_number(const std::string &m)
{
    mn.push_back(m);
    if (m.length() > maxmnlen)
        maxmnlen = m.length();
}

void FileScanner::add_path(const fs::path &p, bool recurse)
{
    paths.emplace_back(p, recurse);
}

template <class T>
void dirit_foreach(T iter, std::function<void(const fs::directory_entry& p)> f, std::atomic<bool> *inter)
{
    for(auto &e : iter)
    {
        if (inter->load()) break;
        f(e);
    }
}

void FileScanner::scan()
{
    size_t fcnt = 0;
    auto opt = std::filesystem::directory_options::skip_permission_denied;
    auto count_files = [&fcnt](const fs::directory_entry& e){
        if (e.is_regular_file()) ++fcnt;
    };
    auto scan_file = [&fcnt, this](const fs::directory_entry &e) {
        if (!e.is_regular_file()) return;
        std::fstream fst(e.path(), std::ios::binary | std::ios::in);
        std::string buf(maxmnlen, '\0');
        fst.read(buf.data(), maxmnlen);
        buf.resize(fst.gcount());
        for (auto &magic : mn)
            if (!memcmp(magic.data(), buf.data(), magic.length()))
            {
                ret.push_back(e.path());
                break;
            }
        Q_EMIT file_scanned(e.path(), ++fcnt);
    };
    auto for_all_paths = [opt, this](std::function<void(const fs::directory_entry&)> f) {
        for (auto &pe : paths)
        {
            fs::path p;
            bool recurse;
            std::tie(p, recurse) = pe;
            if (recurse)
                dirit_foreach(fs::recursive_directory_iterator(p, opt), f, &interrpt);
            else
                dirit_foreach(fs::directory_iterator(p, opt), f, &interrpt);
        }
    };
    for_all_paths(count_files);
    if (interrpt.load()) return;
    Q_EMIT scan_done_prep(fcnt);

    fcnt = 0;
    for_all_paths(scan_file);
}

void FileScanner::interrupt()
{
    interrpt.store(true);
}

bool FileScanner::interrupted()
{
    return interrpt.load();
}

std::vector<fs::path> FileScanner::file_list()
{
    return ret;
}
