#ifndef PATHCHOOSER_HPP
#define PATHCHOOSER_HPP

#include <filesystem>
#include <utility>
#include <vector>

#include <QDialog>

namespace fs = std::filesystem;

class QDialogButtonBox;
class QTableView;
class QStandardItemModel;

class PathChooser : public QDialog
{
    Q_OBJECT
private:
    QTableView *tv;
    QStandardItemModel *im;
    QDialogButtonBox *bb;
public:
    PathChooser(QWidget *parent = nullptr);
    std::vector<std::pair<fs::path, bool>> get_dirs();
public Q_SLOTS:
    void add_new();
    void delete_selected();
};

#endif
