#include <limits>

#include "settings.hpp"

SettingsRegistry::SettingsRegistry(QString path)
{
    s = new QSettings(path, QSettings::Format::IniFormat);
}
SettingsRegistry::~SettingsRegistry()
{
    delete s;
}
int SettingsRegistry::register_tab(QString tab_name)
{
    tabs.push_back(tab_name);
    return tabs.size() - 1;
}

void SettingsRegistry::register_int_option(int tab, std::string key, QString desc, int min, int max, int defaultval)
{
    klist.push_back(key);
    smap[key] = {
        SettingsItem::ParameterType::_int,
        tab,
        key,
        desc,
        QVariant::fromValue<int>(min),
        QVariant::fromValue<int>(max),
        QVariant::fromValue<int>(defaultval),
        nullptr};
}

int SettingsRegistry::get_option_int(std::string key)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_int)
        return -1;
    return s->value(QString::fromStdString(key), smap[key].defaultv).value<int>();
}

void SettingsRegistry::set_option_int(std::string key, int val)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_int)
        return;
    s->setValue(QString::fromStdString(key), QVariant::fromValue<int>(val));
}

void SettingsRegistry::register_bool_option(int tab, std::string key, QString desc, bool defaultval)
{
    klist.push_back(key);
    smap[key] = {
        SettingsItem::ParameterType::_bool,
        tab,
        key,
        desc,
        QVariant(),
        QVariant(),
        QVariant::fromValue<bool>(defaultval),
        nullptr};
}

bool SettingsRegistry::get_option_bool(std::string key)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_bool)
        return false;
    return s->value(QString::fromStdString(key), smap[key].defaultv).value<bool>();
}

void SettingsRegistry::set_option_bool(std::string key, bool val)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_bool)
        return;
    s->setValue(QString::fromStdString(key), QVariant::fromValue<bool>(val));
}

void SettingsRegistry::register_double_option(int tab, std::string key, QString desc, double min, double max, double defaultval)
{
    klist.push_back(key);
    smap[key] = {
        SettingsItem::ParameterType::_double,
        tab,
        key,
        desc,
        QVariant::fromValue<double>(min),
        QVariant::fromValue<double>(max),
        QVariant::fromValue<double>(defaultval),
        nullptr};
}

double SettingsRegistry::get_option_double(std::string key)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_double)
        return std::numeric_limits<double>::quiet_NaN();
    return s->value(QString::fromStdString(key), smap[key].defaultv).value<double>();
}

void SettingsRegistry::set_option_double(std::string key, double val)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_double)
        return;
    s->setValue(QString::fromStdString(key), QVariant::fromValue<double>(val));
}

void SettingsRegistry::register_keyseq_option(int tab, std::string key, QString desc, QKeySequence defaultval)
{
    klist.push_back(key);
    smap[key] = {
        SettingsItem::ParameterType::_keyseq,
        tab,
        key,
        desc,
        QVariant(),
        QVariant(),
        QVariant::fromValue<QKeySequence>(defaultval),
        nullptr};
}

QKeySequence SettingsRegistry::get_option_keyseq(std::string key)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_keyseq)
        return QKeySequence();
    return s->value(QString::fromStdString(key), smap[key].defaultv).value<QKeySequence>();
}

void SettingsRegistry::set_option_keyseq(std::string key, QKeySequence ks)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_keyseq)
        return;
    s->setValue(QString::fromStdString(key), QVariant::fromValue<QKeySequence>(ks));
}

void SettingsRegistry::register_str_option(int tab, std::string key, QString desc, QString defaultval)
{
    klist.push_back(key);
    smap[key] = {
        SettingsItem::ParameterType::_str,
        tab,
        key,
        desc,
        QVariant(),
        QVariant(),
        QVariant::fromValue<QString>(defaultval),
        nullptr};
}

QString SettingsRegistry::get_option_str(std::string key)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_str)
        return QString();
    return s->value(QString::fromStdString(key), smap[key].defaultv).value<QString>();
}

void SettingsRegistry::set_option_str(std::string key, QString str)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_str)
        return;
    s->setValue(QString::fromStdString(key), QVariant::fromValue<QString>(str));
}

void SettingsRegistry::register_strlist_option(int tab, std::string key, QString desc, QStringList defaultval)
{
    klist.push_back(key);
    smap[key] = {
        SettingsItem::ParameterType::_strlist,
        tab,
        key,
        desc,
        QVariant(),
        QVariant(),
        QVariant::fromValue<QStringList>(defaultval),
        nullptr};
}

QStringList SettingsRegistry::get_option_strlist(std::string key)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_strlist)
        return QStringList();
    return s->value(QString::fromStdString(key), smap[key].defaultv).value<QStringList>();
}

void SettingsRegistry::set_option_strlist(std::string key, QStringList str)
{
    if (smap.find(key) == smap.end() || smap[key].type != SettingsItem::ParameterType::_strlist)
        return;
    s->setValue(QString::fromStdString(key), QVariant::fromValue<QStringList>(str));
}
