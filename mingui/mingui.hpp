#ifndef MINGUI_HPP
#define MINGUI_HPP

#include <filesystem>
#include <vector>
#include <string>
#include <unordered_set>

#include <QWidget>
#include <QList>

class QHBoxLayout;
class QLabel;
class QStatusBar;
class QScrollArea;
class QTextEdit;
class QListView;
class QSplitter;
class QStandardItemModel;
class ImageItemDelegate;

namespace fs = std::filesystem;

class MinGuiWidget : public QWidget
{
    Q_OBJECT
private:
    QSplitter *l;
    QTextEdit *infopanel;
    QLabel *permamsg;
    QStatusBar *sb;
    QListView *lw;
    QList<QAction*> selhk;
    QStandardItemModel *im = nullptr;
    ImageItemDelegate *id = nullptr;
    std::size_t ngroups, curgroup;
    bool nohotkeywarn;
    void mark_toggle(std::size_t x);
    void mark_all_but(std::size_t x);
    void mark_all();
    void mark_none();
    void mark_view_update(bool update_msg = true);
    fs::path::string_type common_prefix(const std::vector<fs::path> &fns);
    std::vector<bool> marks;
    std::unordered_set<fs::path> marked;
    std::vector<fs::path> current_set;
protected:
    void resizeEvent(QResizeEvent *e) override;
    void closeEvent(QCloseEvent *e) override;
public:
    MinGuiWidget();
    void show_images(const std::vector<std::filesystem::path> &fns);
    void update_distances(const std::map<std::pair<std::size_t, std::size_t>, double> &d);
    void update_viewstatus(std::size_t cur, std::size_t size);
    void save_list();
    void load_list();
Q_SIGNALS:
    void next();
    void prev();
    void switch_group(std::size_t group);
};

#endif
