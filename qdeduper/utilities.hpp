//Chris Xiong 2022
//License: MPL-2.0
#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <filesystem>

#include <QString>

namespace fs = std::filesystem;

namespace utilities
{
    void open_containing_folder(const fs::path &path);
    QString fspath_to_qstring(const fs::path &p);
    QString fsstr_to_qstring(const fs::path::string_type &s);
    fs::path qstring_to_path(const QString &s);
};

#endif
