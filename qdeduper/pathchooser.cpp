//Chris Xiong 2022
//License: MPL-2.0
#include "pathchooser.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QDebug>
#include <QFileDialog>
#include <QTableView>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

PathChooser::PathChooser(QWidget *parent) : QDialog(parent)
{
    QVBoxLayout *l = new QVBoxLayout();
    this->setLayout(l);
    bb = new QDialogButtonBox(QDialogButtonBox::StandardButton::Ok | QDialogButtonBox::StandardButton::Cancel, this);
    bb->button(QDialogButtonBox::StandardButton::Ok)->setText("Scan");

    QPushButton *pbbrowse = new QPushButton();
    pbbrowse->setText("Browse...");
    pbbrowse->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_DirOpenIcon));
    bb->addButton(pbbrowse, QDialogButtonBox::ButtonRole::ActionRole);
    QObject::connect(pbbrowse, &QPushButton::pressed, this, &PathChooser::add_new);

    QPushButton *pbdelete = new QPushButton();
    pbdelete->setText("Delete");
    pbdelete->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_DialogDiscardButton));
    bb->addButton(pbdelete, QDialogButtonBox::ButtonRole::ActionRole);
    QObject::connect(pbdelete, &QPushButton::pressed, this, &PathChooser::delete_selected);
    QObject::connect(bb, &QDialogButtonBox::accepted, this, &PathChooser::accept);
    QObject::connect(bb, &QDialogButtonBox::rejected, this, &PathChooser::reject);
    im = new QStandardItemModel(this);
    tv = new QTableView();
    tv->setModel(im);
    tv->setSortingEnabled(false);
    tv->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    im->setHorizontalHeaderLabels({"Path", "Recursive?"});

    QLabel *lb = new QLabel("Choose directories to scan");
    l->addWidget(lb);
    l->addWidget(tv);
    l->addWidget(bb);
}

std::vector<std::pair<fs::path, bool>> PathChooser::get_dirs()
{
    std::vector<std::pair<fs::path, bool>> ret;
    for (int i = 0; i < im->rowCount(); ++i)
    {
#if PATH_VALSIZE == 2
        fs::path p(im->item(i, 0)->text().toStdWString());
#else
        fs::path p(im->item(i, 0)->text().toStdString());
#endif
        ret.emplace_back(p, (im->item(i, 1)->checkState() == Qt::CheckState::Checked));
    }
    return ret;
}

void PathChooser::add_new()
{
    QString s = QFileDialog::getExistingDirectory(this, "Open");
    if (s.isNull() || s.isEmpty()) return;
    QStandardItem *it = new QStandardItem(s);
    QStandardItem *ck = new QStandardItem();
    it->setEditable(false);
    ck->setCheckable(true);
    im->appendRow({it, ck});
    tv->resizeColumnsToContents();
}

void PathChooser::delete_selected()
{
    im->removeRow(tv->currentIndex().row());
}
