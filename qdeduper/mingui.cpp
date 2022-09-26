#include "mingui.hpp"
#include "imageitem.hpp"
#include "filescanner.hpp"
#include "pathchooser.hpp"
#include "sigdb_qt.hpp"

#include <chrono>
#include <fstream>
#include <thread>
#include <cwchar>

#include <QDebug>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QActionGroup>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QPushButton>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
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

fs::path qstring_to_path(const QString &s)
{
#if PATH_VALSIZE == 2 //the degenerate platform
    return fs::path(s.toStdWString());
#else
    return fs::path(s.toStdString());
#endif
}

Q_DECLARE_METATYPE(fs::path)

DeduperMainWindow::DeduperMainWindow()
{
    qRegisterMetaType<fs::path>();
    this->setWindowTitle("QDeduper");
    this->setup_menu();
    this->update_actions();
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
    id->set_model(im);
    lv->setItemDelegate(id);
    lv->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    lv->setResizeMode(QListView::ResizeMode::Adjust);
    lv->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    lv->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    lv->setHorizontalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
    lv->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
    lv->setMinimumWidth(240);
    lv->installEventFilter(this);
    pd = new QProgressDialog(this);
    pd->setModal(true);
    pd->setMinimumDuration(0);
    pd->setAutoReset(false);
    pd->setAutoClose(false);
    pd->setWindowTitle("Progress");
    QLabel *pdlb = new QLabel(pd);
    pdlb->setMaximumWidth(500);
    pdlb->setAlignment(Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignVCenter);
    pd->setLabel(pdlb);
    pd->setMinimumWidth(520);
    pd->setMaximumWidth(520);
    pd->setMaximumHeight(120);
    pd->setMinimumHeight(120);
    pd->close();
    QFont fnt = QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont);
    lv->setFont(fnt);
    infopanel->setFont(fnt);
    pd->setFont(fnt);

    for (size_t i = 0; i < keys.size(); ++i)
    {
        auto &k = keys[i];
        QAction *a = new QAction();
        a->setShortcut(QKeySequence(k));
        QObject::connect(a, &QAction::triggered, std::bind(&DeduperMainWindow::mark_toggle, this, i));
        selhk.push_back(a);
        QAction *sa = new QAction();
        sa->setShortcut(QKeySequence(Qt::Modifier::SHIFT | k));
        QObject::connect(sa, &QAction::triggered, std::bind(&DeduperMainWindow::mark_all_but, this, i));
        selhk.push_back(sa);
        QAction *ca = new QAction();
        ca->setShortcut(QKeySequence(Qt::Modifier::CTRL | k));
        QObject::connect(ca, &QAction::triggered, [this, i] {
            if (i >= im->rowCount()) return;
            if (id->is_single_item_mode())
                id->set_single_item_mode(false);
            else
            {
                id->set_single_item_mode(true);
                QTimer::singleShot(5, [this, i] {
                lv->scrollTo(im->index(i, 0), QAbstractItemView::ScrollHint::PositionAtTop);});
            }
            menuact["single_mode_toggle"]->setChecked(id->is_single_item_mode());
        });
        selhk.push_back(ca);
    }
    this->addActions(selhk);

    QObject::connect(lv, &QListView::clicked, [this](const QModelIndex &i) {
        auto cs = i.data(Qt::ItemDataRole::CheckStateRole).value<Qt::CheckState>();
        if (cs == Qt::CheckState::Checked)
            cs = Qt::CheckState::Unchecked;
        else cs = Qt::CheckState::Checked;
        this->im->setData(i, cs, Qt::ItemDataRole::CheckStateRole);
        this->marked_update();
    });
    QObject::connect(lv, &QListView::doubleClicked, [this](const QModelIndex &i) {
        auto cs = i.data(Qt::ItemDataRole::CheckStateRole).value<Qt::CheckState>();
        if (cs == Qt::CheckState::Checked)
            cs = Qt::CheckState::Unchecked;
        else cs = Qt::CheckState::Checked;
        this->im->setData(i, cs, Qt::ItemDataRole::CheckStateRole);
        this->marked_update();
        QDesktopServices::openUrl(QUrl::fromLocalFile(i.data(ImageItem::ImageItemRoles::path_role).toString()));
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
    sort_role = ImageItem::ImageItemRoles::default_order_role;
    sort_order = Qt::SortOrder::AscendingOrder;
}

void DeduperMainWindow::setup_menu()
{
    QMenu *file = this->menuBar()->addMenu("&File");
    QMenu *view = this->menuBar()->addMenu("&View");
    QMenu *mark = this->menuBar()->addMenu("&Marks");
    QMenu *help = this->menuBar()->addMenu("&Help");

    QAction *create_db = file->addAction("Create Database...");
    QObject::connect(create_db, &QAction::triggered, this, &DeduperMainWindow::create_new);
    menuact["create_db"] = create_db;
    QAction *load_db = file->addAction("Load Database...");
    load_db->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_DialogOpenButton));
    QObject::connect(load_db, &QAction::triggered, [this] {
        QString dbpath = QFileDialog::getOpenFileName(this, "Load Database", QString(), "Signature database (*.sigdb)");
        if (!dbpath.isNull())
        {
            if (this->sdb) delete this->sdb;
            this->sdb = new SignatureDB();
            pd->setMaximum(0);
            pd->setMinimum(0);
            pd->setLabelText("Loading database...");
            pd->open();
            pd->setCancelButton(nullptr);
            auto f = QtConcurrent::run([this, dbpath]() -> bool {
                return this->sdb->load(qstring_to_path(dbpath));
            });
            QFutureWatcher<bool> *fw = new QFutureWatcher<bool>(this);
            fw->setFuture(f);
            QObject::connect(fw, &QFutureWatcher<bool>::finished, this, [this, fw] { 
                pd->close();
                if (!fw->result()) {
                    delete this->sdb;
                    this->sdb = nullptr;
                    QMessageBox::critical(this, "Error", "Error loading database.");
                    fw->deleteLater();
                    return;
                }
                curgroup = 0;
                show_group(0);
                fw->deleteLater();
            }, Qt::ConnectionType::QueuedConnection);
        }
    });
    menuact["load_db"] = load_db;

    QAction *save_db = file->addAction("Save Database...");
    save_db->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_DialogSaveButton));
    QObject::connect(save_db, &QAction::triggered, [this] {
        QString dbpath = QFileDialog::getSaveFileName(this, "Save Database", QString(), "Signature database (*.sigdb)");
        if (!dbpath.isNull() && this->sdb)
            this->sdb->save(qstring_to_path(dbpath));
    });
    menuact["save_db"] = save_db;
    file->addSeparator();

    QAction *savelist = file->addAction("Export Marked Images List...");
    savelist->setShortcut(QKeySequence(Qt::Modifier::SHIFT | Qt::Key::Key_Return));
    QObject::connect(savelist, &QAction::triggered, [this]{Q_EMIT this->save_list();});
    menuact["save_list"] = savelist;
    this->addAction(savelist);

    QAction *loadlist = file->addAction("Import Marked Images List...");
    loadlist->setShortcut(QKeySequence(Qt::Key::Key_N));
    QObject::connect(loadlist, &QAction::triggered, [this]{Q_EMIT this->load_list();});
    menuact["load_list"] = loadlist;
    this->addAction(loadlist);

    file->addSeparator();
    QAction *search_img = file->addAction("Search for Image...");
    QObject::connect(search_img, &QAction::triggered, [this] {
        QString fpath = QFileDialog::getOpenFileName(this, "Select Image", QString(), "Image file");
        if (fpath.isNull()) return;
        searched_image = qstring_to_path(fpath);
        search_image(searched_image);
    });
    menuact["search_image"] = search_img;
    file->addSeparator();
    file->addAction("Preferences...");
    file->addAction("Exit");

    QAction *nxtgrp = view->addAction("Next Group");
    nxtgrp->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_ArrowRight));
    menuact["next_group"] = nxtgrp;
    nxtgrp->setShortcut(QKeySequence(Qt::Key::Key_M));
    QObject::connect(nxtgrp, &QAction::triggered, [this] {
        if (this->vm == ViewMode::view_searchresult) { this->show_group(curgroup); return; }
        if (this->sdb && curgroup + 1 < this->sdb->num_groups())
            this->show_group(++curgroup);
    });
    this->addAction(nxtgrp);

    QAction *prvgrp = view->addAction("Previous Group");
    prvgrp->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_ArrowLeft));
    menuact["prev_group"] = prvgrp;
    prvgrp->setShortcut(QKeySequence(Qt::Key::Key_Z));
    QObject::connect(prvgrp, &QAction::triggered, [this] {
        if (this->vm == ViewMode::view_searchresult) { this->show_group(curgroup); return; }
        if (this->sdb && curgroup > 0)
            this->show_group(--curgroup);
    });
    this->addAction(prvgrp);

    QAction *skip = view->addAction("Skip to Group...");
    skip->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_ArrowUp));
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
    QAction *singlemode = view->addAction("Single Item Mode");
    singlemode->setCheckable(true);
    menuact["single_mode_toggle"] = singlemode;
    QObject::connect(singlemode, &QAction::triggered, [this] (bool c) {
        id->set_single_item_mode(c);
    });

    view->addSeparator();
    QMenu *sort = view->addMenu("Sort by");
    QMenu *tbsort = new QMenu(this);
    QAction *sfsz = new QAction("File size");
    QAction *simd = new QAction("Image dimension");
    QAction *sfpt = new QAction("File path");
    QAction *snon = new QAction("Default");
    QList<QAction*> skeya = {sfsz, simd, sfpt, snon};
    QList<ImageItem::ImageItemRoles> skeyr = {
        ImageItem::ImageItemRoles::file_size_role,
        ImageItem::ImageItemRoles::pixelcnt_role,
        ImageItem::ImageItemRoles::path_role,
        ImageItem::ImageItemRoles::database_id_role,
    };
    QActionGroup *skeyg = new QActionGroup(sort);
    for (int i = 0; i < skeya.size(); ++i)
    {
        auto a = skeya[i];
        sort->addAction(a);
        tbsort->addAction(a);
        skeyg->addAction(a);
        a->setCheckable(true);
        ImageItem::ImageItemRoles sr = skeyr[i];
        QObject::connect(a, &QAction::triggered, [this, sr] {
            this->sort_role = sr;
            if (this->vm == ViewMode::view_normal)
                this->show_group(this->curgroup);
            else
                this->search_image(this->searched_image);
        });
    }
    snon->setChecked(true);
    skeyg->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);
    sort->addSeparator();
    tbsort->addSeparator();
    QAction *sasc = new QAction("Ascending");
    QAction *sdec = new QAction("Descending");
    QActionGroup *sordg = new QActionGroup(sort);
    QList<QAction*> sorda = {sasc, sdec};
    QList<Qt::SortOrder> sordv = {Qt::SortOrder::AscendingOrder, Qt::SortOrder::DescendingOrder};
    for (int i = 0; i < sorda.size(); ++i)
    {
        auto a = sorda[i];
        sort->addAction(a);
        tbsort->addAction(a);
        sordg->addAction(a);
        a->setCheckable(true);
        Qt::SortOrder so = sordv[i];
        QObject::connect(a, &QAction::triggered, [this, so] {
            this->sort_order = so;
            if (this->vm == ViewMode::view_normal)
                this->show_group(this->curgroup);
            else
                this->search_image(this->searched_image);
        });
    }
    sasc->setChecked(true);
    sordg->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);

    QAction *mall = mark->addAction("Mark All");
    mall->setShortcut(QKeySequence(Qt::Key::Key_X));
    QObject::connect(mall, &QAction::triggered, [this]{this->mark_all();});
    menuact["mark_all"] = mall;
    this->addAction(mall);

    QAction *mnone = mark->addAction("Mark None");
    mnone->setShortcut(QKeySequence(Qt::Key::Key_C));
    QObject::connect(mnone, &QAction::triggered, [this]{this->mark_none();});
    menuact["mark_none"] = mnone;
    this->addAction(mnone);

    QAction *madir = mark->addAction("Mark All within directory...");
    QObject::connect(madir, &QAction::triggered, [this] {
        QString s = QFileDialog::getExistingDirectory(this, "Open");
        if (s.isNull() || s.isEmpty()) return;
        fs::path p = qstring_to_path(s);
        for (auto &id : this->sdb->get_image_ids())
        {
            fs::path fp = this->sdb->get_image_path(id);
            if (fp.parent_path() == p)
                this->marked.insert(fp);
        }
        for (int i = 0; i < im->rowCount(); ++i)
        {
            ImageItem *itm = static_cast<ImageItem*>(im->item(i));
            fs::path fp = qstring_to_path(itm->path());
            itm->setCheckState(marked.find(fp) == marked.end() ? Qt::CheckState::Unchecked : Qt::CheckState::Checked);
        }
    });
    menuact["mark_all_dir"] = madir;
    QAction *madirr = mark->addAction("Mark All within directory and subdirectory...");
    QObject::connect(madirr, &QAction::triggered, [this] {
        QString s = QFileDialog::getExistingDirectory(this, "Open");
        if (s.isNull() || s.isEmpty()) return;
        fs::path p = qstring_to_path(s);
        for (auto &id : this->sdb->get_image_ids())
        {
            fs::path fp = this->sdb->get_image_path(id);
            if (!fsstr_to_qstring(fp.lexically_relative(p)).startsWith("../"))
                this->marked.insert(fp);
        }
        for (int i = 0; i < im->rowCount(); ++i)
        {
            ImageItem *itm = static_cast<ImageItem*>(im->item(i));
            fs::path fp = qstring_to_path(itm->path());
            itm->setCheckState(marked.find(fp) == marked.end() ? Qt::CheckState::Unchecked : Qt::CheckState::Checked);
        }
    });
    menuact["mark_all_dir_rec"] = madirr;
    mark->addSeparator();
    mark->addAction("Review Marked Images");

    help->addAction("Help");
    help->addSeparator();
    QAction *about = help->addAction("About");
    QObject::connect(about, &QAction::triggered, [this]{QMessageBox::about(this, "About Deduper", "Deduper\nFind similar images on your local filesystem.\n\n0.0.0\nChris Xiong 2022\nMPL-2.0");});
    QAction *aboutqt = help->addAction("About Qt");
    QObject::connect(aboutqt, &QAction::triggered, [this]{QMessageBox::aboutQt(this);});

    tb = new QToolBar(this);
    this->addToolBar(tb);
    tb->addAction(prvgrp);
    tb->addAction(nxtgrp);
    tb->addAction(skip);
    QAction *tbsorta = tb->addAction("Sort by");
    QToolButton *tbsortb = qobject_cast<QToolButton*>(tb->widgetForAction(tbsorta));
    tbsortb->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
    tbsorta->setMenu(tbsort);
    tb->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
}
void DeduperMainWindow::update_actions()
{
    if (!sdb)
    {
        menuact["next_group"]->setEnabled(false);
        menuact["prev_group"]->setEnabled(false);
        menuact["skip_group"]->setEnabled(false);
        menuact["save_db"]->setEnabled(false);
        menuact["load_list"]->setEnabled(false);
        menuact["save_list"]->setEnabled(false);
        menuact["search_image"]->setEnabled(false);
        return;
    }
    menuact["skip_group"]->setEnabled(true);
    menuact["prev_group"]->setEnabled(curgroup > 0);
    menuact["next_group"]->setEnabled(curgroup + 1 < sdb->num_groups());
    menuact["search_image"]->setEnabled(true);
    menuact["save_db"]->setEnabled(true);
    menuact["load_list"]->setEnabled(true);
    menuact["save_list"]->setEnabled(true);
    if (vm == ViewMode::view_searchresult)
    {
        menuact["next_group"]->setEnabled(true);
        menuact["prev_group"]->setEnabled(true);
    }
}

void DeduperMainWindow::search_image(const fs::path &path)
{
    auto sim = this->sdb->search_file(path);
    if (sim.empty())
    {
        this->sb->showMessage("No similar image found.", 2000);
        return;
    }
    this->vm = ViewMode::view_searchresult;
    std::vector<size_t> ps;
    std::map<std::pair<size_t, size_t>, double> dm;
    for (auto &s : sim)
    {
        ps.push_back(s.first);
        dm[std::make_pair(0, s.first)] = s.second;
    }
    this->show_images(ps);
    this->sort_reassign_hotkeys();
    this->update_distances(dm);
    this->sb->showMessage("Use next group / previous group to go back.");
    this->permamsg->setText("Viewing image search result");
}

void DeduperMainWindow::show_images(const std::vector<size_t> &ids)
{
    im->clear();
    std::vector<fs::path> fns;
    for (auto &id : ids) fns.push_back(this->sdb->get_image_path(id));
    fs::path::string_type common_pfx = common_prefix(fns);
    size_t idx = 0;
    if (ids.size() > keys.size() && !nohotkeywarn)
        nohotkeywarn = QMessageBox::StandardButton::Ignore ==
                       QMessageBox::warning(this,
                             "Too many duplicates",
                             "Too many duplicates found. Some couldn't be assigned a hotkey. Ignore = do not show again.",
                             QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Ignore,
                             QMessageBox::StandardButton::Ok);
    for (auto &id : ids)
    {
        fs::path &f = fns[idx];
        ImageItem *imitm = new ImageItem(fsstr_to_qstring(f.native()),
                                         fsstr_to_qstring(f.native().substr(common_pfx.length())),
                                         idx < keys.size() ? keys[idx] : QKeySequence(),
                                         id, idx,
                                         lv->devicePixelRatioF());
        imitm->setCheckState(marked.find(f) == marked.end() ? Qt::CheckState::Unchecked : Qt::CheckState::Checked);
        im->appendRow(imitm);
        ++idx;
    }
    marked_update(false);
}

void DeduperMainWindow::update_distances(const std::map<std::pair<size_t, size_t>, double> &d)
{
    QString r;
    std::map<size_t, QString> idkeymap;
    for (int i = 0; i < im->rowCount(); ++i)
    {
        ImageItem *itm = static_cast<ImageItem*>(im->item(i));
        idkeymap[itm->database_id()] = itm->hotkey().isEmpty() ? "(No hotkey)" : itm->hotkey().toString();
    }
    for (auto &p : d)
    {
        QString ka = idkeymap[p.first.first];
        QString kb = idkeymap[p.first.second];
        if (vm == ViewMode::view_normal)
            r += QString("%1 <-> %2: %3\n").arg(ka).arg(kb).arg(QString::number(p.second));
        else
            r += QString("%1 : %2\n").arg(kb).arg(QString::number(p.second));;
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
    QString fn = QFileDialog::getSaveFileName(this, "Save list", QString(), "File List (*.txt)");
#if PATH_VALSIZE == 2
    std::wfstream fst(qstring_to_path(fn), std::ios_base::out);
#else
    std::fstream fst(qstring_to_path(fn), std::ios_base::out);
#endif
    if (fst.fail()) return;
    for (auto &x : this->marked)
        fst << x.native() << std::endl;
    fst.close();
}

void DeduperMainWindow::load_list()
{
    QString fn = QFileDialog::getOpenFileName(this, "Load list", QString(), "File List (*.txt)");
#if PATH_VALSIZE == 2
    std::wfstream fst(qstring_to_path(fn), std::ios_base::in);
#else
    std::fstream fst(qstring_to_path(fn), std::ios_base::in);
#endif
    if (fst.fail()) return;
    this->marked.clear();
    while(!fst.eof())
    {
        fs::path::string_type s;
        std::getline(fst, s);
        if (s.back() == fst.widen('\n')) s.pop_back();
        if (!s.empty()) this->marked.insert(s);
    }
    fst.close();
    for (int i = 0; i < im->rowCount(); ++i)
    {
        fs::path p = qstring_to_path(static_cast<ImageItem*>(im->item(i))->path());
        im->item(i)->setCheckState(marked.find(p) != marked.end() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    }
    marked_update();
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
    this->pd->setCancelButton(new QPushButton("Cancel"));
    auto f = QtConcurrent::run([this, paths] {
        FileScanner *fs = new FileScanner();
        this->fsc = fs;
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
                QString etxt = this->fontMetrics().elidedText(QString("Looking for files to scan: %1").arg(fsstr_to_qstring(p)),
                                                              Qt::TextElideMode::ElideMiddle,
                                                              475);
                this->pd->setLabelText(etxt);
                this->pd->setValue(c);
            }
        }, Qt::ConnectionType::QueuedConnection);
        fs->scan();
        if (fs->interrupted())
        {
            delete fs;
            this->fsc = nullptr;
            return;
        }
        int flsize = fs->file_list().size() - 1;
        QMetaObject::invokeMethod(this->pd, [flsize, this] {this->pd->setMaximum(flsize);}, Qt::ConnectionType::QueuedConnection);
        if (this->sdb) delete this->sdb;
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
        this->sdb->scan_files(fs->file_list(), std::thread::hardware_concurrency());
        delete fs;
        this->fsc = nullptr;
    });
    QFutureWatcher<void> *fw = new QFutureWatcher<void>(this);
    fw->setFuture(f);
    QObject::connect(fw, &QFutureWatcher<void>::finished, this, [this, fw] {
        this->pd->reset();
        this->pd->close();
        this->curgroup = 0;
        this->vm = ViewMode::view_normal;
        this->show_group(this->curgroup);
        fw->deleteLater();
    }, Qt::ConnectionType::QueuedConnection);
    QObject::connect(pd, &QProgressDialog::canceled, [this] {
        if (this->fsc) this->fsc->interrupt();
        if (this->sdb) this->sdb->interrupt_scan();
    });
}

void DeduperMainWindow::show_group(size_t gid)
{
    if (!sdb || gid >= sdb->num_groups())
        return;
    this->vm = ViewMode::view_normal;
    auto g = sdb->get_group(gid);
    this->show_images(g);
    this->sort_reassign_hotkeys();
    this->update_distances(sdb->group_distances(gid));
    this->update_viewstatus(gid, sdb->num_groups());
    this->update_actions();
}

void DeduperMainWindow::sort_reassign_hotkeys()
{
    im->setSortRole(this->sort_role);
    im->sort(0, this->sort_order);
    for (int i = 0; i < im->rowCount(); ++i)
        static_cast<ImageItem*>(im->item(i))->set_hotkey(i < keys.size() ? keys[i] : QKeySequence());
}

void DeduperMainWindow::mark_toggle(size_t x)
{
    if (vm == ViewMode::view_searchresult)
    {
        mark_none(false);
        sb->showMessage("Marking images in search result is disabled.", 2000);
        return;
    }
    if (x < im->rowCount())
    {
        Qt::CheckState ckst = im->item(x)->checkState() == Qt::CheckState::Checked ?
                              Qt::CheckState::Unchecked : Qt::CheckState::Checked;
        im->item(x)->setCheckState(ckst);
    }
    marked_update();
}

void DeduperMainWindow::mark_all_but(size_t x)
{
    if (vm == ViewMode::view_searchresult)
    {
        mark_none(false);
        sb->showMessage("Marking images in search result is disabled.", 2000);
        return;
    }
    if (x < im->rowCount())
    {
        for (int i = 0; i < im->rowCount(); ++i)
        {
            Qt::CheckState ckst = (i == x) ? Qt::CheckState::Unchecked : Qt::CheckState::Checked;
            im->item(x)->setCheckState(ckst);
        }
    }
    marked_update();
}

void DeduperMainWindow::mark_all()
{
    if (vm == ViewMode::view_searchresult)
    {
        mark_none(false);
        sb->showMessage("Marking images in search result is disabled.", 2000);
        return;
    }
    for (int i = 0; i < im->rowCount(); ++i)
        im->item(i)->setCheckState(Qt::CheckState::Checked);
    marked_update();
}

void DeduperMainWindow::mark_none(bool msg)
{
    for (int i = 0; i < im->rowCount(); ++i)
        im->item(i)->setCheckState(Qt::CheckState::Unchecked);
    marked_update(msg);
}

void DeduperMainWindow::marked_update(bool update_msg)
{
    size_t m = 0;
    for (int i = 0; i < im->rowCount(); ++i)
    {
        fs::path p = qstring_to_path(static_cast<ImageItem*>(im->item(i))->path());
        if (im->item(i)->checkState() == Qt::CheckState::Checked)
        {
            marked.insert(p);
            ++m;
        }
        else
            if (marked.find(p) != marked.end())
                marked.erase(marked.find(p));
    }
    if (update_msg)
    sb->showMessage(QString("%1 of %2 marked for deletion").arg(m).arg(im->rowCount()), 1000);
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

bool DeduperMainWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::Type::Resize)
    {
        if (id && obj == lv)
            id->resize();
        return false;
    }
    return false;
}
