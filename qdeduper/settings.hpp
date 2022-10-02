#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <QKeySequence>
#include <QSettings>

struct SettingsItem
{
    enum ParameterType
    {
        _int,
        _bool,
        _double,
        _keyseq,
        _str,
        _strlist
    };
    ParameterType type;
    int tab;
    std::string key;
    QString desc;
    QVariant min, max;
    QVariant defaultv;
    QWidget *w;
};

class SettingsRegistry
{
public:
    SettingsRegistry(QString path);
    ~SettingsRegistry();
    int register_tab(QString tab_name);
    void register_int_option(int tab, std::string key, QString desc, int min, int max, int defaultval);
    int get_option_int(std::string key);
    void set_option_int(std::string key, int val);
    void register_bool_option(int tab, std::string key, QString desc, bool defaultval);
    bool get_option_bool(std::string key);
    void set_option_bool(std::string key, bool val);
    void register_double_option(int tab, std::string key, QString desc, double min, double max, double defaultval);
    double get_option_double(std::string key);
    void set_option_double(std::string key, double val);
    void register_keyseq_option(int tab, std::string key, QString desc, QKeySequence defaultval);
    QKeySequence get_option_keyseq(std::string key);
    void set_option_keyseq(std::string key, QKeySequence ks);
    void register_str_option(int tab, std::string key, QString desc, QString defaultval);
    QString get_option_str(std::string key);
    void set_option_str(std::string key, QString str);
    void register_strlist_option(int tab, std::string key, QString desc, QStringList defaultval);
    QStringList get_option_strlist(std::string key);
    void set_option_strlist(std::string key, QStringList str);
private:
    QSettings *s;
    QStringList tabs;
    std::map<std::string, SettingsItem> smap;
    std::vector<std::string> klist;
    friend class PreferenceDialog;
};

#endif
