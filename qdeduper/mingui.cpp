#include "mingui.hpp"
#include "imageitem.hpp"
#include "filescanner.hpp"
#include "pathchooser.hpp"
#include "sigdb_qt.hpp"

#include <cstdio>
#include <chrono>
#include <cwchar>

#include <QDebug>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QToolBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QString>
#include <QScrollArea>
#include <QListView>
#include <QProgressDialog>
#include <QStandardItemModel>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QPixmap>
#include <QFile>
#include <QScreen>
#include <QFontDatabase>
#include <QFileDialog>
#include <QKeySequence>
#include <QTextEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>

using std::size_t;
const std::vector<int> keys = {
        Qt::Key::Key_A, Qt::Key::Key_S, Qt::Key::Key_D, Qt::Key::Key_F,
        Qt::Key::Key_G, Qt::Key::Key_H, Qt::Key::Key_J, Qt::Key::Key_K,
        Qt::Key::Key_L, Qt::Key::Key_Semicolon, Qt::Key::Key_T, Qt::Key::Key_Y,
        Qt::Key::Key_U, Qt::Key::Key_I, Qt::Key::Key_O, Qt::Key::Key_P
};

QString fsstr_to_qstring(const fs::path::string_type &s)
{
#if PATH_VALSIZE == 2 //the degenerate platform
    return QString::fromStdWString(s);
#else
    return QString::fromStdString(s);
#endif
}

Q_DECLARE_METATYPE(fs::path)

DeduperMainWindow::DeduperMainWindow()
{
    qRegisterMetaType<fs::path>();
    this->setFont(QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont));
    this->setWindowTitle("deduper");
    this->setup_menu();
    sb = this->statusBar();
    sb->addPermanentWidget(permamsg = new QLabel());
    QLabel *opm = new QLabel();
    opm->setText("placeholder status bar text");
    sb->addWidget(opm);
    l = new QSplitter(Qt::Orientation::Horizontal, this);
    l->setContentsMargins(6, 6, 6, 6);
    l->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setCentralWidget(l);
    infopanel = new QTextEdit(this);
    infopanel->setReadOnly(true);
    infopanel->setMinimumWidth(80);
    lv = new QListView(this);
    im = new QStandardItemModel(this);
    lv->setModel(im);
    lv->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
    lv->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
    id = new ImageItemDelegate();
    id->setScrollbarMargins(lv->verticalScrollBar()->width(),
                            lv->horizontalScrollBar()->height());
    lv->setItemDelegate(id);
    lv->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    lv->setResizeMode(QListView::ResizeMode::Adjust);
    lv->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    lv->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    lv->setHorizontalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
    lv->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
    lv->setMinimumWidth(240);
    pd = new QProgressDialog(this);
    pd->setModal(true);
    pd->setMinimumDuration(0);
    pd->setAutoReset(false);
    pd->setAutoClose(false);
    pd->close();

    for (size_t i = 0; i < keys.size(); ++i)
    {
        auto &k = keys[i];
        QAction *a = new QAction();
        a->setShortcut(QKeySequence(k));
        QObject::connect(a, &QAction::triggered, [this, i](){this->mark_toggle(i);});
        selhk.push_back(a);
        QAction *sa = new QAction();
        sa->setShortcut(QKeySequence(Qt::Modifier::SHIFT | k));
        QObject::connect(sa, &QAction::triggered, [this, i](){this->mark_all_but(i);});
        selhk.push_back(a);
    }
    this->addActions(selhk);
    QAction *mall = new QAction();
    mall->setShortcut(QKeySequence(Qt::Key::Key_X));
    QObject::connect(mall, &QAction::triggered, [this]{this->mark_all();});
    this->addAction(mall);
    QAction *mnone = new QAction();
    mnone->setShortcut(QKeySequence(Qt::Key::Key_C));
    QObject::connect(mnone, &QAction::triggered, [this]{this->mark_none();});
    this->addAction(mnone);
    QAction *load = new QAction();
    load->setShortcut(QKeySequence(Qt::Key::Key_N));
    QObject::connect(load, &QAction::triggered, [this]{Q_EMIT this->load_list();});
    this->addAction(load);
    QAction *save = new QAction();
    save->setShortcut(QKeySequence(Qt::Modifier::SHIFT | Qt::Key::Key_Return));
    QObject::connect(save, &QAction::triggered, [this]{Q_EMIT this->save_list();});
    this->addAction(save);

    QObject::connect(lv, &QListView::clicked, [this](const QModelIndex &i) {
        auto cs = i.data(Qt::ItemDataRole::CheckStateRole).value<Qt::CheckState>();
        if (cs == Qt::CheckState::Checked)
            cs = Qt::CheckState::Unchecked;
        else cs = Qt::CheckState::Checked;
        this->im->setData(i, cs, Qt::ItemDataRole::CheckStateRole);
    });
    QObject::connect(lv, &QListView::doubleClicked, [this](const QModelIndex &i) {
        auto cs = i.data(Qt::ItemDataRole::CheckStateRole).value<Qt::CheckState>();
        if (cs == Qt::CheckState::Checked)
            cs = Qt::CheckState::Unchecked;
        else cs = Qt::CheckState::Checked;
        this->im->setData(i, cs, Qt::ItemDataRole::CheckStateRole);
        QDesktopServices::openUrl(QUrl::fromLocalFile(i.data(ImageItem::ImageItemRoles::path_role).toString()));
    });
    QObject::connect(im, &QStandardItemModel::itemChanged, [this](QStandardItem *i) {
        ImageItem *itm = static_cast<ImageItem*>(i);
        QModelIndex idx = itm->index();
        bool checked = itm->data(Qt::ItemDataRole::CheckStateRole) == Qt::CheckState::Checked;
        if (checked != marks[idx.row()])
            this->mark_toggle(idx.row());
    });
    l->addWidget(lv);
    l->addWidget(infopanel);
    l->setStretchFactor(0, 3);
    l->setStretchFactor(1, 1);
    l->setCollapsible(0, false);
    marked.clear();
    infopanel->setText("bleh");
    infopanel->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
    nohotkeywarn = false;
}

void DeduperMainWindow::setup_menu()
{
    QMenu *file = this->menuBar()->addMenu("File");
    QMenu *view = this->menuBar()->addMenu("View");
    QMenu *mark = this->menuBar()->addMenu("Marks");
    QMenu *help = this->menuBar()->addMenu("Help");

    QAction *create_db = file->addAction("Create Database...");
    QObject::connect(create_db, &QAction::triggered, this, &DeduperMainWindow::create_new);
    menuact["create_db"] = create_db;
    file->addAction("Load Database...");
    file->addAction("Save Database...");
    file->addSeparator();
    file->addAction("Search for Image...");
    file->addSeparator();
    file->addAction("Preferences...");
    file->addAction("Exit");

    QAction *nxtgrp = view->addAction("Next Group");
    nxtgrp->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_ArrowRight));
    menuact["next_group"] = nxtgrp;
    nxtgrp->setShortcut(QKeySequence(Qt::Key::Key_M));
    QObject::connect(nxtgrp, &QAction::triggered, [this] {
        if (this->sdb && curgroup + 1 < this->sdb->num_groups())
            this->show_group(++curgroup);
    });
    this->addAction(nxtgrp);

    QAction *prvgrp = view->addAction("Previous Group");
    prvgrp->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_ArrowLeft));
    menuact["prev_group"] = prvgrp;
    prvgrp->setShortcut(QKeySequence(Qt::Key::Key_Z));
    QObject::connect(prvgrp, &QAction::triggered, [this] {
        if (this->sdb && curgroup > 1)
            this->show_group(--curgroup);
    });
    this->addAction(prvgrp);

    QAction *skip = view->addAction("Skip to Group...");
    menuact["skip_group"] = skip;
    skip->setShortcut(QKeySequence(Qt::Key::Key_B));
    QObject::connect(skip, &QAction::triggered, [this] {
        if (!this->sdb) return;
        bool ok = false;
        int g = QInputDialog::getInt(this, "Skip to group",
                                     QString("Group # (1-%1)").arg(sdb->num_groups()),
                                     curgroup + 1,
                                     1, sdb->num_groups(), 1, &ok);
        if (ok) this->show_group((size_t) g - 1);
    });
    this->addAction(skip);

    view->addSeparator();
    QMenu *sort = view->addMenu("Sort by");
    sort->addAction("File size");
    sort->addAction("Image dimension");
    sort->addAction("File path");

    mark->addAction("Mark All");
    mark->addAction("Mark None");
    mark->addAction("Mark All within...");
    mark->addAction("Review Marked Imagess");

    help->addAction("View Documentation");
    help->addAction("About");

    tb = new QToolBar(this);
    this->addToolBar(tb);
    tb->addAction(prvgrp);
    tb->addAction(nxtgrp);
    tb->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
}
void DeduperMainWindow::update_actions()
{
    if (!sdb)
    {
        menuact["next_group"]->setEnabled(false);
        menuact["prev_group"]->setEnabled(false);
        return;
    }
}

void DeduperMainWindow::show_images(const std::vector<fs::path> &fns)
{
    current_set = fns;
    marks.clear();
    im->clear();
    fs::path::string_type common_pfx = common_prefix(fns);
    size_t idx = 0;
    if (fns.size() > keys.size() && !nohotkeywarn)
        nohotkeywarn = QMessageBox::StandardButton::Ignore ==
                       QMessageBox::warning(this,
                             "Too many duplicates",
                             "Too many duplicates found. Some couldn't be assigned a hotkey. Ignore = do not show again.",
                             QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Ignore,
                             QMessageBox::StandardButton::Ok);
    for (auto &f : fns)
    {
        marks.push_back(marked.find(f) != marked.end());
        im->appendRow(new ImageItem(fsstr_to_qstring(f.native()), fsstr_to_qstring(f.native().substr(common_pfx.length())), keys[idx], lv->devicePixelRatioF()));
        ++idx;
    }
    mark_view_update(false);
}

void DeduperMainWindow::update_distances(const std::map<std::pair<size_t, size_t>, double> &d)
{
    QString r;
    for (auto &p : d)
    {
        QString ka = "(No hotkey)";
        QString kb = "(No hotkey)";
        if (p.first.first < keys.size())
            ka = QKeySequence(keys[p.first.first]).toString();
        if (p.first.second < keys.size())
            kb = QKeySequence(keys[p.first.second]).toString();
        r += QString("%1 <-> %2: %3\n").arg(ka).arg(kb).arg(QString::number(p.second));
    }
    infopanel->setText(r);
}

void DeduperMainWindow::update_viewstatus(std::size_t cur, std::size_t size)
{
    permamsg->setText(QString("Viewing group %1 of %2").arg(cur + 1).arg(size));
    curgroup = cur;
}

void DeduperMainWindow::save_list()
{
    QString fn = QFileDialog::getSaveFileName(this, "Save list", QString(), "*.txt");
    FILE *f = fopen(fn.toStdString().c_str(), "w");
    if (!f) return;
    for (auto &x : this->marked)
#ifdef _WIN32
        fwprintf(f, L"%ls\n", x.c_str());
#else
        fprintf(f, "%s\n", x.c_str());
#endif
    fclose(f);
}

void DeduperMainWindow::load_list()
{
    QString fn = QFileDialog::getOpenFileName(this, "Load list", QString(), "*.txt");
    FILE *f = fopen(fn.toStdString().c_str(), "r");
    if (!f) return;
    this->marked.clear();
    while(!feof(f))
    {
#ifdef _WIN32
        wchar_t buf[32768];
        fgetws(buf, 32768, f);
        std::wstring ws(buf);
        if (ws.back() == L'\n') ws.pop_back();
        if (!ws.empty()) this->marked.insert(ws);
#else
        char buf[32768];
        fgets(buf, 32768, f);
        std::string s(buf);
        if (s.back() == '\n') s.pop_back();
        if (!s.empty()) this->marked.insert(s);
#endif
    }
    fclose(f);
    for (size_t i = 0; i < marks.size(); ++i)
        marks[i] = marked.find(current_set[i]) != marked.end();
    mark_view_update();
}

void DeduperMainWindow::create_new()
{
    PathChooser *pc = new PathChooser(this);
    pc->setModal(true);
    if (pc->exec() == QDialog::DialogCode::Accepted)
        this->scan_dirs(pc->get_dirs());
    pc->deleteLater();
}

void DeduperMainWindow::scan_dirs(std::vector<std::pair<fs::path, bool>> paths)
{
    this->pd->open();
    this->pd->setLabelText("Preparing for database creation...");
    this->pd->setMinimum(0);
    this->pd->setMaximum(0);
    auto f = QtConcurrent::run([this, paths] {
        FileScanner *fs = new FileScanner();
        std::for_each(paths.begin(), paths.end(), [fs](auto p){fs->add_path(p.first, p.second);});
        fs->add_magic_number("\x89PNG\r\n");
        fs->add_magic_number("\xff\xd8\xff");
        QObject::connect(fs, &FileScanner::scan_done_prep, this, [this](auto n) {
            this->pd->setMaximum(n - 1);
        }, Qt::ConnectionType::QueuedConnection);
        QObject::connect(fs, &FileScanner::file_scanned, this, [this](const fs::path &p, size_t c) {
            static auto lt = std::chrono::steady_clock::now();
            using namespace std::literals;
            if (std::chrono::steady_clock::now() - lt > 100ms)
            {
                lt = std::chrono::steady_clock::now();
                this->pd->setLabelText(QString("Looking for files to scan: %1").arg(fsstr_to_qstring(p)));
                this->pd->setValue(c);
            }
        }, Qt::ConnectionType::QueuedConnection);
        fs->scan();
        this->pd->setMaximum(fs->file_list().size() - 1);
        this->pd->setLabelText("Scanning...");
        this->sdb = new SignatureDB();
        QObject::connect(this->sdb, &SignatureDB::image_scanned, this, [this](size_t n) {
            static auto lt = std::chrono::steady_clock::now();
            using namespace std::literals;
            if (std::chrono::steady_clock::now() - lt > 100ms)
            {
                lt = std::chrono::steady_clock::now();
                this->pd->setLabelText(QString("Scanning %1 / %2").arg(n + 1).arg(this->pd->maximum() + 1));
                this->pd->setValue(n);
            }
            if (!~n)
            {
                this->pd->setMaximum(0);
                this->pd->setLabelText("Finalizing...");
            }
        }, Qt::ConnectionType::QueuedConnection);
        this->sdb->scan_files(fs->file_list(), 8);
        delete fs;
    });
    QFutureWatcher<void> *fw = new QFutureWatcher<void>(this);
    fw->setFuture(f);
    QObject::connect(fw, &QFutureWatcher<void>::finished, this, [this] {
        this->pd->reset();
        this->pd->close();
        this->curgroup = 0;
        this->show_group(this->curgroup);
    }, Qt::ConnectionType::QueuedConnection);
}

void DeduperMainWindow::show_group(size_t gid)
{
    if (!sdb || gid >= sdb->num_groups())
        return;
    auto g = sdb->get_group(gid);
    current_set.clear();
    std::for_each(g.begin(), g.end(), [this](auto id){current_set.push_back(sdb->get_image_path(id));});
    this->show_images(current_set);
    this->update_distances(sdb->group_distances(gid));
    this->update_viewstatus(gid, sdb->num_groups());
}

void DeduperMainWindow::mark_toggle(size_t x)
{
    if (x < marks.size())
    {
        marks[x] = !marks[x];
        if (marks[x])
            marked.insert(current_set[x]);
        else
            if (marked.find(current_set[x]) != marked.end())
                marked.erase(marked.find(current_set[x]));
    }
    mark_view_update();
}

void DeduperMainWindow::mark_all_but(size_t x)
{
    if (x < marks.size())
    {
        for (size_t i = 0; i < marks.size(); ++i)
        {
            marks[i] = (i != x);
            if (marks[i])
                marked.insert(current_set[i]);
            else
                if (marked.find(current_set[i]) != marked.end())
                    marked.erase(marked.find(current_set[i]));
        }
    }
    mark_view_update();
}

void DeduperMainWindow::mark_all()
{
    for (size_t i = 0; i < marks.size(); ++i)
    {
        marks[i] = true;
        marked.insert(current_set[i]);
    }
    mark_view_update();
}

void DeduperMainWindow::mark_none()
{
    for (size_t i = 0; i < marks.size(); ++i)
    {
        marks[i] = false;
        if (marked.find(current_set[i]) != marked.end())
            marked.erase(marked.find(current_set[i]));
    }
    mark_view_update();
}

void DeduperMainWindow::mark_view_update(bool update_msg)
{
    size_t m = 0;
    for (size_t i = 0; i < current_set.size(); ++i)
    {
        if (marks[i])
        {
            im->item(i)->setCheckState(Qt::CheckState::Checked);
            ++m;
        }
        else
        {
            im->item(i)->setCheckState(Qt::CheckState::Unchecked);
        }
    }
    if (update_msg)
    sb->showMessage(QString("%1 of %2 marked for deletion").arg(m).arg(current_set.size()), 1000);
}

fs::path::string_type DeduperMainWindow::common_prefix(const std::vector<fs::path> &fns)
{
    using fsstr = fs::path::string_type;
    fsstr ret;
    fsstr shortest = *std::min_element(fns.begin(), fns.end(), [](auto &a, auto &b){return a.native().length() < b.native().length();});
    for (size_t i = 0; i < shortest.length(); ++i)
    {
        fs::path::value_type c = shortest[i];
        bool t = true;
        for (auto &s : fns) if (s.c_str()[i] != c) {t = false; break;}
        if (!t) break;
        ret.push_back(c);
    }
    if (!ret.empty())
    {
        auto p = ret.rfind(std::filesystem::path::preferred_separator);
        if (p != fsstr::npos)
            return fs::path(ret.substr(0, p + 1));
    }
    return ret;
}

void DeduperMainWindow::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    if (!id || !im) return;
    for (int i = 0; i < im->rowCount(); ++i)
        id->resize(im->indexFromItem(im->item(i)));
}

void DeduperMainWindow::closeEvent(QCloseEvent *e)
{
    if (QMessageBox::StandardButton::Yes ==
        QMessageBox::question(this, "Confirmation", "Really quit?",
                              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                              QMessageBox::StandardButton::No))
        e->accept();
    else
        e->ignore();
}