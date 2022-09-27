#ifndef MINGUI_HPP
#define MINGUI_HPP

#include <filesystem>
#include <map>
#include <vector>
#include <string>
#include <unordered_set>

#include <QMainWindow>
#include <QList>

#include "filescanner.hpp"
#include "sigdb_qt.hpp"

class QHBoxLayout;
class QLabel;
class QStatusBar;
class QScrollArea;
class QTextEdit;
class QListView;
class QProgressDialog;
class QSplitter;
class QStandardItemModel;
class QToolBar;
class QTimer;
class FileScanner;
class ImageItemDelegate;
class SettingsRegistry;
class PreferenceDialog;

namespace fs = std::filesystem;

enum ViewMode
{
    view_normal,
    view_searchresult,
    view_marked
};

class DeduperMainWindow : public QMainWindow
{
    Q_OBJECT
private:
    QSplitter *l;
    QTextEdit *infopanel;
    QLabel *permamsg;
    QLabel *dbramusg;
    QStatusBar *sb;
    QListView *lv;
    QToolBar *tb = nullptr;
    std::map<std::string, QAction*> menuact;
    QList<QAction*> selhk;
    QStandardItemModel *im = nullptr;
    ImageItemDelegate *id = nullptr;
    QProgressDialog *pd = nullptr;
    SignatureDB *sdb = nullptr;
    FileScanner *fsc = nullptr;
    SettingsRegistry *sr = nullptr;
    PreferenceDialog *prefdlg = nullptr;
    QTimer *rampupd = nullptr;
    std::unordered_set<fs::path> marked;
    int sort_role;
    Qt::SortOrder sort_order;
    size_t curgroup;
    fs::path searched_image;
    ViewMode vm;
    bool nohotkeywarn;
    bool markschanged;

    void mark_toggle(size_t x);
    void mark_all_but(size_t x);
    void mark_all();
    void mark_none(bool msg = true);
    void marked_update(bool update_msg = true);
    fs::path::string_type common_prefix(const std::vector<fs::path> &fns);
    bool modified_check(bool quitting = true);
protected:
    void closeEvent(QCloseEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;
public:
    DeduperMainWindow();
    ~DeduperMainWindow();

    void setup_menu();
    void sort_reassign_hotkeys();
    void search_image(const fs::path &path);
    void show_images(const std::vector<size_t> &ids);
    void update_distances(const std::map<std::pair<size_t, size_t>, double> &d);
    void update_viewstatus(size_t cur, size_t size);
    void save_list();
    void load_list();

    void scan_dirs(std::vector<std::pair<fs::path, bool>> paths);
public Q_SLOTS:
    void create_new();
    void update_actions();
    void show_group(size_t gid);
    void show_marked();
    void apply_prefs();
    void update_memusg();
Q_SIGNALS:
    void next();
    void prev();
    void switch_group(size_t group);
};

#endif
