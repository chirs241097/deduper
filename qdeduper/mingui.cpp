#include "mingui.hpp"
#include "utilities.hpp"
#include "imageitem.hpp"
#include "filescanner.hpp"
#include "pathchooser.hpp"
#include "sigdb_qt.hpp"
#include "settings.hpp"
#include "preferencedialog.hpp"

#include <climits>
#include <chrono>
#include <fstream>
#include <thread>
#include <cwchar>

#include <QDebug>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QApplication>
#include <QActionGroup>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QString>
#include <QTimer>
#include <QScrollArea>
#include <QListView>
#include <QProgressDialog>
#include <QStandardItemModel>
#include <QLabel>
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

const std::vector<int> iadefkeys = {
        Qt::Key::Key_A, Qt::Key::Key_S, Qt::Key::Key_D, Qt::Key::Key_F,
        Qt::Key::Key_G, Qt::Key::Key_H, Qt::Key::Key_J, Qt::Key::Key_K,
        Qt::Key::Key_L, Qt::Key::Key_Semicolon, Qt::Key::Key_T, Qt::Key::Key_Y,
        Qt::Key::Key_U, Qt::Key::Key_I, Qt::Key::Key_O, Qt::Key::Key_P
};
const std::map<std::string, QKeySequence> defhk = {
    {"create_db",    QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_T)},
    {"load_db",      QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_O)},
    {"save_db",      QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_W)},
    {"save_list",    QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_E)},
    {"load_list",    QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_R)},
    {"search_image", QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_Slash)},
    {"preferences",  QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_P)},
    {"exit",         QKeySequence(Qt::Modifier::CTRL | Qt::Key::Key_Q)},
    {"next_group",   QKeySequence(Qt::Key::Key_M)},
    {"prev_group",   QKeySequence(Qt::Key::Key_Z)},
    {"skip_group",   QKeySequence(Qt::Key::Key_B)},
    {"single_mode_toggle", QKeySequence()},
    {"mark_all",     QKeySequence(Qt::Key::Key_X)},
    {"mark_none",     QKeySequence(Qt::Key::Key_C)},
    {"mark_all_dir", QKeySequence()},
    {"mark_all_dir_rec", QKeySequence()},
    {"view_marked",  QKeySequence()},
};
const std::vector<int> iadefmo = {
    0,                                          //mark_toggle
    Qt::Modifier::SHIFT,                        //mark_all_except
    Qt::Modifier::CTRL | Qt::Modifier::SHIFT,   //show_only
    Qt::Modifier::ALT | Qt::Modifier::SHIFT,    //open_with_system_viewer
    INT_MAX                                     //open_containing_folder
};

Q_DECLARE_METATYPE(fs::path)

DeduperMainWindow::DeduperMainWindow()
{
    qRegisterMetaType<fs::path>();
    qApp->setWindowIcon(QIcon(":/img/deduper.png"));
    this->setWindowTitle("QDeduper");
    sb = this->statusBar();
    sb->addPermanentWidget(dbramusg = new QLabel());
    sb->addPermanentWidget(permamsg = new QLabel());
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
    id->set_scrollbar_margins(lv->verticalScrollBar()->width(),
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
    Qt::WindowFlags pdwf = pd->windowFlags();
    pdwf |= Qt::WindowType::CustomizeWindowHint;
    pdwf &= ~(Qt::WindowType::WindowCloseButtonHint | Qt::WindowType::WindowContextHelpButtonHint);
    pd->setWindowFlags(pdwf);
    QFont fnt = QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont);
    lv->setFont(fnt);
    infopanel->setFont(fnt);
    pdlb->setFont(fnt);
    rampupd = new QTimer(this);
    rampupd->setInterval(1000);
    rampupd->stop();
    QObject::connect(rampupd, &QTimer::timeout, this, &DeduperMainWindow::update_memusg);
    this->setup_menu();
    this->update_actions();
    for (size_t i = 0; i < iadefkeys.size(); ++i)
    {
        QAction *ma = new QAction();
        QObject::connect(ma, &QAction::triggered, std::bind(&DeduperMainWindow::mark_toggle, this, i));
        selhk.push_back(ma);

        QAction *sa = new QAction();
        QObject::connect(sa, &QAction::triggered, std::bind(&DeduperMainWindow::mark_all_but, this, i));
        selhk.push_back(sa);

        QAction *ca = new QAction();
        QObject::connect(ca, &QAction::triggered, std::bind(&DeduperMainWindow::show_only, this, i));
        selhk.push_back(ca);

        QAction *oa = new QAction();
        QObject::connect(oa, &QAction::triggered, std::bind(&DeduperMainWindow::open_image, this, i));
        selhk.push_back(oa);

        QAction *la = new QAction();
        QObject::connect(la, &QAction::triggered, std::bind(&DeduperMainWindow::locate_image, this, i));
        selhk.push_back(la);
    }
    this->addActions(selhk);

    sr = new SettingsRegistry(QStandardPaths::writableLocation(QStandardPaths::StandardLocation::ConfigLocation) + QString("/qdeduperrc"));
    int generalt = sr->register_tab("General");
    sr->register_int_option(generalt, "min_image_dim", "Minimal Dimension in Image View", 16, 4096, 64);
    sr->register_int_option(generalt, "thread_count", "Number of Threads (0 = Automatic)", 0, 4096, 0);
    sr->register_bool_option(generalt, "toolbar_text", "Show Text in Toolbar Buttons", true);
    sr->register_bool_option(generalt, "show_memory_usage", "Show Database Engine Memory Usage", false);
    int sigt = sr->register_tab("Signature");
    sr->register_double_option(sigt, "signature/threshold", "Distance Threshold", 0, 1, 0.3);
    int hkt = sr->register_tab("Shortcuts");
    for (auto &hkp : defhk)
    {
        sr->register_keyseq_option(hkt, "hotkey/" + hkp.first, QString(), hkp.second);
    }
    for (size_t i = 0; i < iadefmo.size(); ++i)
    {
        std::string iamt = "hotkey/item_action_mod_" + std::to_string(i);
        sr->register_int_option(hkt, iamt, QString(), INT_MIN, INT_MAX, iadefmo[i]);
    }
    for (size_t i = 0; i < iadefkeys.size(); ++i)
    {
        std::string iakt = "hotkey/item_" + std::to_string(i) + "_action_key";
        sr->register_int_option(hkt, iakt, QString(), INT_MIN, INT_MAX, iadefkeys[i]);
    }
    int tbt = sr->register_tab("Toolbar");
    sr->register_strlist_option(tbt, "toolbar_actions", "", {"prev_group", "next_group", "skip_group", "single_mode_toggle", "sort"});
    prefdlg = new PreferenceDialog(sr, this);
    prefdlg->setModal(true);
    prefdlg->close();
    prefdlg->set_hkactions(hkt, actionlist, menuact);
    prefdlg->set_toolbaractions(tbt, menuact);
    QObject::connect(menuact["preferences"], &QAction::triggered, prefdlg, &PreferenceDialog::open);
    QObject::connect(prefdlg, &PreferenceDialog::accepted, this, &DeduperMainWindow::apply_prefs);
    apply_prefs();

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
        open_image(i.row());
    });
    l->addWidget(lv);
    l->addWidget(infopanel);
    l->setStretchFactor(0, 3);
    l->setStretchFactor(1, 1);
    l->setCollapsible(0, false);
    marked.clear();
    infopanel->setText("(Difference between images)");
    infopanel->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
    nohotkeywarn = false;
    sort_role = ImageItem::ImageItemRoles::default_order_role;
    sort_order = Qt::SortOrder::AscendingOrder;
}

DeduperMainWindow::~DeduperMainWindow()
{
    delete sr;
}

void DeduperMainWindow::setup_menu()
{
    QMenu *file = this->menuBar()->addMenu("&File");
    QMenu *view = this->menuBar()->addMenu("&View");
    QMenu *mark = this->menuBar()->addMenu("&Marks");
    QMenu *help = this->menuBar()->addMenu("&Help");

    QAction *create_db = file->addAction("Create Database...");
    QObject::connect(create_db, &QAction::triggered, this, &DeduperMainWindow::create_new);
    register_action("create_db", create_db);
    QAction *load_db = file->addAction("Load Database...");
    load_db->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_DialogOpenButton));
    QObject::connect(load_db, &QAction::triggered, [this] {
        if (!modified_check(false)) return;
        QString dbpath = QFileDialog::getOpenFileName(this, "Load Database", QString(), "Signature database (*.sigdb)");
        if (!dbpath.isNull())
        {
            if (this->sdb) delete this->sdb;
            this->sdb = new SignatureDB();
            pd->setMaximum(0);
            pd->setMinimum(0);
            pd->setLabelText("Loading database...");
            pd->open();
            QPushButton *cancelbtn = pd->findChild<QPushButton*>();
            if (Q_LIKELY(cancelbtn)) cancelbtn->setVisible(false);
            auto f = QtConcurrent::run([this, dbpath]() -> bool {
                return this->sdb->load(utilities::qstring_to_path(dbpath));
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
                this->markschanged = false;
                fw->deleteLater();
            }, Qt::ConnectionType::QueuedConnection);
        }
    });
    register_action("load_db", load_db);

    QAction *save_db = file->addAction("Save Database...");
    save_db->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_DialogSaveButton));
    QObject::connect(save_db, &QAction::triggered, [this] {
        QString dbpath = QFileDialog::getSaveFileName(this, "Save Database", QString(), "Signature database (*.sigdb)");
        if (!dbpath.isNull() && this->sdb)
            this->sdb->save(utilities::qstring_to_path(dbpath));
    });
    register_action("save_db", save_db);
    file->addSeparator();

    QAction *savelist = file->addAction("Export Marked Images List...");
    QObject::connect(savelist, &QAction::triggered, [this]{Q_EMIT this->save_list();});
    register_action("save_list", savelist);

    QAction *loadlist = file->addAction("Import Marked Images List...");
    QObject::connect(loadlist, &QAction::triggered, [this]{Q_EMIT this->load_list();});
    register_action("load_list", loadlist);

    file->addSeparator();
    QAction *search_img = file->addAction("Search for Image...");
    QObject::connect(search_img, &QAction::triggered, [this] {
        QString fpath = QFileDialog::getOpenFileName(this, "Select Image", QString(), "Image file (*.*)");
        if (fpath.isNull()) return;
        searched_image = utilities::qstring_to_path(fpath);
        search_image(searched_image);
    });
    register_action("search_image", search_img);
    file->addSeparator();
    QAction *pref = file->addAction("Preferences...");
    register_action("preferences", pref);
    QAction *exita = file->addAction("Exit");
    QObject::connect(exita, &QAction::triggered, [this] {
        if (this->modified_check()) qApp->quit();
    });
    register_action("exit", exita);

    QAction *nxtgrp = view->addAction("Next Group");
    nxtgrp->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_ArrowRight));
    register_action("next_group", nxtgrp);
    QObject::connect(nxtgrp, &QAction::triggered, [this] {
        if (this->vm == ViewMode::view_searchresult) { this->show_group(curgroup); return; }
        if (this->sdb && curgroup + 1 < this->sdb->num_groups())
            this->show_group(++curgroup);
    });

    QAction *prvgrp = view->addAction("Previous Group");
    prvgrp->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_ArrowLeft));
    register_action("prev_group", prvgrp);
    QObject::connect(prvgrp, &QAction::triggered, [this] {
        if (this->vm == ViewMode::view_searchresult) { this->show_group(curgroup); return; }
        if (this->sdb && curgroup > 0)
            this->show_group(--curgroup);
    });

    QAction *skip = view->addAction("Skip to Group...");
    skip->setIcon(this->style()->standardIcon(QStyle::StandardPixmap::SP_ArrowUp));
    register_action("skip_group", skip);
    QObject::connect(skip, &QAction::triggered, [this] {
        if (!this->sdb) return;
        bool ok = false;
        int g = QInputDialog::getInt(this, "Skip to group",
                                     QString("Group # (1-%1)").arg(sdb->num_groups()),
                                     curgroup + 1,
                                     1, sdb->num_groups(), 1, &ok);
        if (ok) this->show_group((size_t) g - 1);
    });

    view->addSeparator();
    QAction *singlemode = view->addAction("Single Item Mode");
    singlemode->setIcon(QIcon(":/img/maximize.svg"));
    singlemode->setCheckable(true);
    register_action("single_mode_toggle", singlemode);
    QObject::connect(singlemode, &QAction::triggered, [this] (bool c) {
        id->set_single_item_mode(c);
    });

    view->addSeparator();
    QAction *sort = view->addAction("Sort by");
    sort->setIcon(QIcon(":/img/sort.svg"));
    QMenu *sortm = new QMenu(this);
    sort->setMenu(sortm);
    QAction *sfsz = sortm->addAction("File size");
    QAction *simd = sortm->addAction("Image dimension");
    QAction *sfpt = sortm->addAction("File path");
    QAction *snon = sortm->addAction("Default");
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
        sortm->addAction(a);
        skeyg->addAction(a);
        a->setCheckable(true);
        ImageItem::ImageItemRoles sr = skeyr[i];
        QObject::connect(a, &QAction::triggered, [this, sr] {
            this->sort_role = sr;
            switch (this->vm)
            {
                case ViewMode::view_normal:
                    this->show_group(this->curgroup);
                break;
                case ViewMode::view_searchresult:
                    this->search_image(this->searched_image);
                break;
                case ViewMode::view_marked:
                    this->show_marked();
            }
        });
    }
    snon->setChecked(true);
    skeyg->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);
    sortm->addSeparator();
    QAction *sasc = new QAction("Ascending");
    QAction *sdec = new QAction("Descending");
    QActionGroup *sordg = new QActionGroup(sort);
    QList<QAction*> sorda = {sasc, sdec};
    QList<Qt::SortOrder> sordv = {Qt::SortOrder::AscendingOrder, Qt::SortOrder::DescendingOrder};
    for (int i = 0; i < sorda.size(); ++i)
    {
        auto a = sorda[i];
        sortm->addAction(a);
        sordg->addAction(a);
        a->setCheckable(true);
        Qt::SortOrder so = sordv[i];
        QObject::connect(a, &QAction::triggered, [this, so] {
            this->sort_order = so;
            switch (this->vm)
            {
                case ViewMode::view_normal:
                    this->show_group(this->curgroup);
                    break;
                case ViewMode::view_searchresult:
                    this->search_image(this->searched_image);
                    break;
                case ViewMode::view_marked:
                    this->show_marked();
            }
        });
    }
    sasc->setChecked(true);
    sordg->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);
    register_action("sort", sort);

    QAction *mall = mark->addAction("Mark All");
    mall->setIcon(QIcon(":/img/select_all.svg"));
    QObject::connect(mall, &QAction::triggered, [this]{this->mark_all();});
    register_action("mark_all", mall);

    QAction *mnone = mark->addAction("Mark None");
    QObject::connect(mnone, &QAction::triggered, [this]{this->mark_none();});
    register_action("mark_none", mnone);

    QAction *madir = mark->addAction("Mark All within directory...");
    QObject::connect(madir, &QAction::triggered, [this] {
        QString s = QFileDialog::getExistingDirectory(this, "Open");
        if (s.isNull() || s.isEmpty()) return;
        fs::path p = utilities::qstring_to_path(s);
        for (auto &id : this->sdb->get_image_ids())
        {
            fs::path fp = this->sdb->get_image_path(id);
            if (fp.parent_path() == p)
                this->marked.insert(fp);
        }
        for (int i = 0; i < im->rowCount(); ++i)
        {
            ImageItem *itm = static_cast<ImageItem*>(im->item(i));
            fs::path fp = utilities::qstring_to_path(itm->path());
            itm->setCheckState(marked.find(fp) == marked.end() ? Qt::CheckState::Unchecked : Qt::CheckState::Checked);
        }
    });
    register_action("mark_all_dir", madir);
    QAction *madirr = mark->addAction("Mark All within directory and subdirectory...");
    QObject::connect(madirr, &QAction::triggered, [this] {
        QString s = QFileDialog::getExistingDirectory(this, "Open");
        if (s.isNull() || s.isEmpty()) return;
        fs::path p = utilities::qstring_to_path(s);
        for (auto &id : this->sdb->get_image_ids())
        {
            fs::path fp = this->sdb->get_image_path(id);
            if (!utilities::fspath_to_qstring(fp.lexically_relative(p)).startsWith("../"))
                this->marked.insert(fp);
        }
        for (int i = 0; i < im->rowCount(); ++i)
        {
            ImageItem *itm = static_cast<ImageItem*>(im->item(i));
            fs::path fp = utilities::qstring_to_path(itm->path());
            itm->setCheckState(marked.find(fp) == marked.end() ? Qt::CheckState::Unchecked : Qt::CheckState::Checked);
        }
    });
    register_action("mark_all_dir_rec", madirr);
    mark->addSeparator();
    QAction *view_marked = mark->addAction("Review Marked Images");
    QObject::connect(view_marked, &QAction::triggered, this, &DeduperMainWindow::show_marked);
    register_action("view_marked", view_marked);

    help->addAction("Help");
    help->addSeparator();
    QAction *about = help->addAction("About");
    QObject::connect(about, &QAction::triggered, [this]{QMessageBox::about(this, "About Deduper", "Deduper\nFind similar images on your local filesystem.\n\n0.0.0\nChris Xiong 2022\nMPL-2.0");});
    QAction *aboutqt = help->addAction("About Qt");
    QObject::connect(aboutqt, &QAction::triggered, [this]{QMessageBox::aboutQt(this);});

    lv->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    QObject::connect(lv, &QWidget::customContextMenuRequested, [this] (const QPoint &pos) {
        const QModelIndex &idx = lv->indexAt(pos);
        if (!idx.isValid()) return;
        Qt::CheckState cks = idx.data(Qt::ItemDataRole::CheckStateRole).value<Qt::CheckState>();
        QMenu *cm = new QMenu(this);
        QAction *ma = cm->addAction(cks == Qt::CheckState::Checked ? "Unmark" : "Mark");
        QObject::connect(ma, &QAction::triggered, std::bind(&DeduperMainWindow::mark_toggle, this, idx.row()));

        QAction *sa = cm->addAction("Mark All Except");
        QObject::connect(sa, &QAction::triggered, std::bind(&DeduperMainWindow::mark_all_but, this, idx.row()));

        QAction *ca = cm->addAction(id->is_single_item_mode() ? "Restore" : "Maximize");
        QObject::connect(ca, &QAction::triggered, std::bind(&DeduperMainWindow::show_only, this, idx.row()));

        QAction *oa = cm->addAction("Open Image");
        QObject::connect(oa, &QAction::triggered, std::bind(&DeduperMainWindow::open_image, this, idx.row()));

        QAction *la = cm->addAction("Open Containing Folder");
        QObject::connect(la, &QAction::triggered, std::bind(&DeduperMainWindow::locate_image, this, idx.row()));

        QObject::connect(cm, &QMenu::aboutToHide, [cm] {cm->deleteLater();});
        cm->popup(this->lv->mapToGlobal(pos));
    });

    tb = new QToolBar(this);
    this->addToolBar(tb);

    for (auto &ap : menuact)
        this->addAction(ap.second);
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
    if (vm != ViewMode::view_normal)
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
    this->id->set_show_hotkey(false);
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
    if (ids.size() > iadefkeys.size() && !nohotkeywarn && this->vm != ViewMode::view_marked)
        nohotkeywarn = QMessageBox::StandardButton::Ignore ==
                       QMessageBox::warning(this,
                             "Too many duplicates",
                             "Too many duplicates found. Some couldn't be assigned a hotkey. Ignore = do not show again.",
                             QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Ignore,
                             QMessageBox::StandardButton::Ok);
    for (auto &id : ids)
    {
        fs::path &f = fns[idx];
        ImageItem *imitm = new ImageItem(utilities::fspath_to_qstring(f),
                                         utilities::fsstr_to_qstring(f.native().substr(common_pfx.length())),
                                         idx < iadefkeys.size() ? iadefkeys[idx] : QKeySequence(),
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
    std::wfstream fst(utilities::qstring_to_path(fn), std::ios_base::out);
#else
    std::fstream fst(utilities::qstring_to_path(fn), std::ios_base::out);
#endif
    if (fst.fail()) return;
    for (auto &x : this->marked)
        fst << x.native() << std::endl;
    fst.close();
    this->markschanged = false;
}

void DeduperMainWindow::load_list()
{
    QString fn = QFileDialog::getOpenFileName(this, "Load list", QString(), "File List (*.txt)");
#if PATH_VALSIZE == 2
    std::wfstream fst(utilities::qstring_to_path(fn), std::ios_base::in);
#else
    std::fstream fst(utilities::qstring_to_path(fn), std::ios_base::in);
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
        fs::path p = utilities::qstring_to_path(static_cast<ImageItem*>(im->item(i))->path());
        im->item(i)->setCheckState(marked.find(p) != marked.end() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    }
    marked_update();
    this->markschanged = false;
}

void DeduperMainWindow::create_new()
{
    if (!modified_check(false)) return;
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
    QPushButton *cancelbtn = this->pd->findChild<QPushButton*>();
    if (Q_LIKELY(cancelbtn)) cancelbtn->setVisible(true);
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
                QString etxt = this->fontMetrics().elidedText(QString("Looking for files to scan: %1").arg(utilities::fspath_to_qstring(p)),
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
                QPushButton *cancelbtn = this->pd->findChild<QPushButton*>();
                if (Q_LIKELY(cancelbtn)) cancelbtn->setVisible(false);
            }
        }, Qt::ConnectionType::QueuedConnection);
        populate_cfg_t c = this->sdb->get_sig_config();
        int nthreads = this->sr->get_option_int("thread_count");
        if (!nthreads) nthreads = std::thread::hardware_concurrency();
        c.njobs = nthreads;
        c.threshold = this->sr->get_option_double("signature/threshold");
        this->sdb->set_sig_config(c);
        this->sdb->scan_files(fs->file_list());
        delete fs;
        this->fsc = nullptr;
    });
    QFutureWatcher<void> *fw = new QFutureWatcher<void>(this);
    fw->setFuture(f);
    QObject::connect(fw, &QFutureWatcher<void>::finished, this, [this, fw] {
        this->pd->reset();
        this->pd->close();
        this->markschanged = false;
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
    this->id->set_show_hotkey(true);
    auto g = sdb->get_group(gid);
    this->show_images(g);
    this->sort_reassign_hotkeys();
    this->update_distances(sdb->group_distances(gid));
    this->update_viewstatus(gid, sdb->num_groups());
    this->update_actions();
}

void DeduperMainWindow::show_marked()
{
    this->id->set_show_hotkey(false);
    this->vm = ViewMode::view_marked;
    std::vector<size_t> g;
    for (auto &m : marked)
        g.push_back(this->sdb->get_path_id(m));
    this->show_images(g);
    this->sort_reassign_hotkeys();
    this->update_distances({});
    this->update_actions();
    this->sb->showMessage("Use next group / previous group to go back.");
    this->permamsg->setText("Viewing marked images");
}

void DeduperMainWindow::apply_prefs()
{
    id->set_min_height(sr->get_option_int("min_image_dim"));
    tb->setToolButtonStyle(sr->get_option_bool("toolbar_text") ? Qt::ToolButtonStyle::ToolButtonTextBesideIcon
                                                               : Qt::ToolButtonStyle::ToolButtonIconOnly);
    if (sr->get_option_bool("show_memory_usage"))
        this->rampupd->start();
    else
    {
        this->rampupd->stop();
        this->dbramusg->setText(QString());
    }
    for (auto &hkn : actionlist)
    {
        QKeySequence ks = sr->get_option_keyseq("hotkey/" + hkn);
        menuact[hkn]->setShortcut(ks);
    }
    for (size_t i = 0; i < selhk.size(); ++i)
    {
        QAction *act = selhk[i];
        size_t actn = i / ItemActionType::ACTION_MAX;
        size_t actt = i % ItemActionType::ACTION_MAX;
        std::string iamt = "hotkey/item_action_mod_" + std::to_string(actt);
        std::string iakt = "hotkey/item_" + std::to_string(actn) + "_action_key";
        int im = sr->get_option_int(iamt);
        int ik = sr->get_option_int(iakt);
        QKeySequence ks = ~im ? QKeySequence(static_cast<Qt::Key>(ik | im)) : QKeySequence();
        act->setShortcut(ks);
    }
    auto tbal = sr->get_option_strlist("toolbar_actions");
    tb->clear();
    bool tbvisible = false;
    for (auto &tban : tbal)
    {
        if (menuact.find(tban.toStdString()) == menuact.end()) continue;
        tbvisible = true;
        QAction *act = menuact[tban.toStdString()];
        tb->addAction(act);
        if (act->menu())
        {
            QToolButton *tbb = qobject_cast<QToolButton*>(tb->widgetForAction(act));
            tbb->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        }
    }
    tb->setVisible(tbvisible);
}

void DeduperMainWindow::update_memusg()
{
    if (this->sdb)
        dbramusg->setText(QString("Database memory usage: %1").arg(QLocale::system().formattedDataSize(this->sdb->db_memory_usage())));
}

void DeduperMainWindow::register_action(const std::string &actn, QAction *act)
{
    menuact[actn] = act;
    actionlist.push_back(actn);
}

void DeduperMainWindow::sort_reassign_hotkeys()
{
    im->setSortRole(this->sort_role);
    im->sort(0, this->sort_order);
    for (int i = 0; i < im->rowCount(); ++i)
        static_cast<ImageItem*>(im->item(i))->set_hotkey(i < iadefkeys.size() ? iadefkeys[i] : QKeySequence());
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
    this->markschanged = true;
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
            im->item(i)->setCheckState(ckst);
        }
    }
    marked_update();
    this->markschanged = true;
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
    this->markschanged = true;
}

void DeduperMainWindow::mark_none(bool msg)
{
    for (int i = 0; i < im->rowCount(); ++i)
        im->item(i)->setCheckState(Qt::CheckState::Unchecked);
    marked_update(msg);
    this->markschanged = true;
}

void DeduperMainWindow::marked_update(bool update_msg)
{
    size_t m = 0;
    for (int i = 0; i < im->rowCount(); ++i)
    {
        fs::path p = utilities::qstring_to_path(static_cast<ImageItem*>(im->item(i))->path());
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

void DeduperMainWindow::show_only(size_t x)
{
    if (x >= im->rowCount()) return;
    if (id->is_single_item_mode())
        id->set_single_item_mode(false);
    else
    {
        id->set_single_item_mode(true);
        QTimer::singleShot(5, [this, x] {
        lv->scrollTo(im->index(x, 0), QAbstractItemView::ScrollHint::PositionAtTop);});
    }
    menuact["single_mode_toggle"]->setChecked(id->is_single_item_mode());
}

void DeduperMainWindow::open_image(size_t x)
{
    if (x >= im->rowCount()) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(im->item(x, 0)->data(ImageItem::ImageItemRoles::path_role).toString()));
}

void DeduperMainWindow::locate_image(size_t x)
{
    if (x >= im->rowCount()) return;
    utilities::open_containing_folder(utilities::qstring_to_path(im->item(x, 0)->data(ImageItem::ImageItemRoles::path_role).toString()));
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

bool DeduperMainWindow::modified_check(bool quitting)
{
    if (!this->sdb || !this->sdb->valid()) return true;
    if (this->markschanged || this->sdb->is_dirty())
        return QMessageBox::StandardButton::Yes ==
               QMessageBox::question(this, "Confirmation", quitting ? "You have unsaved files list or database. Really quit?"
                                                                    : "You have unsaved files list or database. Proceed?",
                                     QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                                     QMessageBox::StandardButton::No);
    return true;
}

void DeduperMainWindow::closeEvent(QCloseEvent *e)
{
    if (modified_check())
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
