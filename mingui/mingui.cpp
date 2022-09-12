#include "mingui.hpp"
#include "imageitem.hpp"

#include <cstdio>
#include <cwchar>
#include <type_traits>

#include <QDebug>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QAction>
#include <QString>
#include <QScrollArea>
#include <QListView>
#include <QStandardItemModel>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QPixmap>
#include <QFile>
#include <QScreen>
#include <QFont>
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
#ifdef _WIN32 //the degenerate platform
    return QString::fromStdWString(s);
#else
    return QString::fromStdString(s);
#endif
}

MinGuiWidget::MinGuiWidget()
{
    this->setFont(QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont));
    this->setWindowTitle("deduper minigui");
    this->setLayout(new QVBoxLayout(this));
    QWidget *everything_except_statusbar = new QWidget(this);
    sb = new QStatusBar(this);
    sb->addPermanentWidget(permamsg = new QLabel());
    QLabel *opm = new QLabel();
    opm->setText("z: previous group, m: next group, x: mark all for deletion, c: unmark all, click: toggle, shift+click: open, shift+return: save list");
    sb->addWidget(opm);
    this->layout()->addWidget(everything_except_statusbar);
    this->layout()->addWidget(sb);
    l = new QHBoxLayout(everything_except_statusbar);
    everything_except_statusbar->setLayout(l);
    infopanel = new QTextEdit(this);
    infopanel->setReadOnly(true);
    lw = new QListView(this);
    im = new QStandardItemModel(this);
    lw->setModel(im);
    id = new ImageItemDelegate();
    lw->setItemDelegate(id);
    lw->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    lw->setResizeMode(QListView::ResizeMode::Adjust);

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
    QAction *nxt = new QAction();
    nxt->setShortcut(QKeySequence(Qt::Key::Key_M));
    QObject::connect(nxt, &QAction::triggered, [this]{Q_EMIT this->next();});
    this->addAction(nxt);
    QAction *prv = new QAction();
    prv->setShortcut(QKeySequence(Qt::Key::Key_Z));
    QObject::connect(prv, &QAction::triggered, [this]{Q_EMIT this->prev();});
    this->addAction(prv);
    QAction *load = new QAction();
    load->setShortcut(QKeySequence(Qt::Key::Key_N));
    QObject::connect(load, &QAction::triggered, [this]{Q_EMIT this->load_list();});
    this->addAction(load);
    QAction *skip = new QAction();
    skip->setShortcut(QKeySequence(Qt::Key::Key_B));
    QObject::connect(skip, &QAction::triggered, [this]{
        bool ok = false;
        int g = QInputDialog::getInt(this, "Skip to group",
                                     QString("Group # (1-%1)").arg(ngroups),
                                     curgroup + 1,
                                     1, ngroups, 1, &ok);
        if (ok) Q_EMIT switch_group((size_t) g - 1);
    });
    this->addAction(skip);
    QAction *save = new QAction();
    save->setShortcut(QKeySequence(Qt::Modifier::SHIFT | Qt::Key::Key_Return));
    QObject::connect(save, &QAction::triggered, [this]{Q_EMIT this->save_list();});
    this->addAction(save);

    QObject::connect(lw, &QListView::clicked, [this](const QModelIndex &i) {
        auto cs = i.data(Qt::ItemDataRole::CheckStateRole).value<Qt::CheckState>();
        if (cs == Qt::CheckState::Checked)
            cs = Qt::CheckState::Unchecked;
        else cs = Qt::CheckState::Checked;
        this->im->setData(i, cs, Qt::ItemDataRole::CheckStateRole);
    });
    QObject::connect(lw, &QListView::doubleClicked, [this](const QModelIndex &i) {
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
    l->addWidget(lw);
    l->addWidget(infopanel);
    marked.clear();
    infopanel->setText("bleh");
    infopanel->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
    nohotkeywarn = false;
}

void MinGuiWidget::show_images(const std::vector<fs::path> &fns)
{
    current_set = fns;
    marks.clear();
    im->clear();
    int max_height = (this->screen()->size().height() / fns.size() * 0.8 - 24) * this->screen()->devicePixelRatio();
    int max_width = this->screen()->size().width() * 0.8 * this->screen()->devicePixelRatio();
    if (max_height < 64) max_height = 64;
    if (max_width < 64) max_width = 64;
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
        im->appendRow(new ImageItem(fsstr_to_qstring(f.native()), fsstr_to_qstring(f.native().substr(common_pfx.length())), keys[idx], lw->devicePixelRatioF()));
        ++idx;
    }
    mark_view_update(false);
}

void MinGuiWidget::update_distances(const std::map<std::pair<size_t, size_t>, double> &d)
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

void MinGuiWidget::update_viewstatus(std::size_t cur, std::size_t size)
{
    permamsg->setText(QString("Viewing group %1 of %2").arg(cur + 1).arg(size));
    ngroups = size;
    curgroup = cur;
}

void MinGuiWidget::save_list()
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

void MinGuiWidget::load_list()
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

void MinGuiWidget::mark_toggle(size_t x)
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

void MinGuiWidget::mark_all_but(size_t x)
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

void MinGuiWidget::mark_all()
{
    for (size_t i = 0; i < marks.size(); ++i)
    {
        marks[i] = true;
        marked.insert(current_set[i]);
    }
    mark_view_update();
}

void MinGuiWidget::mark_none()
{
    for (size_t i = 0; i < marks.size(); ++i)
    {
        marks[i] = false;
        if (marked.find(current_set[i]) != marked.end())
            marked.erase(marked.find(current_set[i]));
    }
    mark_view_update();
}

void MinGuiWidget::mark_view_update(bool update_msg)
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

fs::path::string_type MinGuiWidget::common_prefix(const std::vector<fs::path> &fns)
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

void MinGuiWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    if (!id || !im) return;
    for (int i = 0; i < im->rowCount(); ++i)
        id->resize(im->indexFromItem(im->item(i)));
}

void MinGuiWidget::closeEvent(QCloseEvent *e)
{
    if (QMessageBox::StandardButton::Yes ==
        QMessageBox::question(this, "Confirmation", "Really quit?",
                              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                              QMessageBox::StandardButton::No))
        e->accept();
    else
        e->ignore();
}
