#ifndef MINGUI_HPP
#define MINGUI_HPP

#include <filesystem>
#include <vector>
#include <string>
#include <unordered_set>

#include <QWidget>

class QHBoxLayout;
class QLabel;
class QStatusBar;

namespace fs = std::filesystem;

class MinGuiWidget : public QWidget
{
    Q_OBJECT
private:
    QHBoxLayout *l;
    QLabel *infopanel;
    QLabel *permamsg;
    QWidget *imgcontainer;
    QStatusBar *sb;
    void mark_toggle(std::size_t x);
    void mark_all_but(std::size_t x);
    void mark_all();
    void mark_none();
    void mark_view_update(bool update_msg = true);
    fs::path::string_type common_prefix(const std::vector<fs::path> &fns);
    std::vector<QWidget*> imgw;
    std::vector<bool> marks;
    std::unordered_set<fs::path> marked;
    std::vector<fs::path> current_set;
protected:
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
public:
    MinGuiWidget();
    void show_images(const std::vector<std::filesystem::path> &fns);
    void update_distances(const std::map<std::pair<std::size_t, std::size_t>, double> &d);
    void update_permamsg(std::size_t cur, std::size_t size);
    void save_list();
Q_SIGNALS:
    void next();
    void prev();
};

class ImageWidget : public QWidget
{
    Q_OBJECT
private:
    QString fn;
    std::size_t idx;
    QLabel *im;
    QLabel *lb;
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
public:
    ImageWidget(fs::path f, fs::path::string_type dispfn, std::size_t _idx, int max_width, int max_height, QWidget *par);
    void set_marked(bool marked);
Q_SIGNALS:
    void clicked();
};

#endif
