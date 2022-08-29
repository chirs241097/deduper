#include "mingui.hpp"

#include <cstdio>
#include <filesystem>

#include <QDebug>
#include <QKeyEvent>
#include <QString>
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
#include <QMessageBox>
#include <QDesktopServices>

using std::size_t;
const std::vector<int> keys = {
        Qt::Key::Key_A, Qt::Key::Key_S, Qt::Key::Key_D, Qt::Key::Key_F,
        Qt::Key::Key_G, Qt::Key::Key_H, Qt::Key::Key_J, Qt::Key::Key_K,
        Qt::Key::Key_L, Qt::Key::Key_Semicolon, Qt::Key::Key_T, Qt::Key::Key_Y,
        Qt::Key::Key_U, Qt::Key::Key_I, Qt::Key::Key_O, Qt::Key::Key_P
};

MinGuiWidget::MinGuiWidget()
{
    this->setFont(QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont));
    this->setWindowTitle("deduper minigui");
    this->setLayout(new QVBoxLayout(this));
    QWidget *c = new QWidget(this);
    sb = new QStatusBar(this);
    sb->addPermanentWidget(permamsg = new QLabel());
    QLabel *opm = new QLabel();
    opm->setText("z: previous group, m: next group, x: mark all for deletion, c: unmark all, click: toggle, shift+click: open, shift+return: save list");
    sb->addWidget(opm);
    this->layout()->addWidget(c);
    this->layout()->addWidget(sb);
    l = new QHBoxLayout(c);
    c->setLayout(l);
    infopanel = new QLabel(this);
    imgcontainer = new QWidget(this);
    l->addWidget(imgcontainer);
    l->addWidget(infopanel);
    marked.clear();
    infopanel->setText("bleh");
    infopanel->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
}

void MinGuiWidget::show_images(const std::vector<std::string> &fns)
{
    current_set = fns;
    marks.clear();
    imgw.clear();
    qDeleteAll(imgcontainer->children());
    imgcontainer->setLayout(new QVBoxLayout(imgcontainer));
    int max_height = (this->screen()->size().height() / fns.size() * 0.75 - 24) * this->screen()->devicePixelRatio();
    int max_width = this->screen()->size().width() * 0.8 * this->screen()->devicePixelRatio();
    std::string common_pfx = common_prefix(fns);
    size_t idx = 0;
    if (fns.size() > keys.size())
        QMessageBox::warning(this, "Too many duplicates", "Too many duplicates found. Some couldn't be assigned a hotkey.");
    for (auto &f : fns)
    {
        marks.push_back(marked.find(f) != marked.end());
        ImageWidget *tw = new ImageWidget(f, f.substr(common_pfx.length()), idx, max_width, max_height, this);
        QObject::connect(tw, &ImageWidget::clicked, [this, idx] { this->mark_toggle(idx); });
        imgw.push_back(tw);
        imgcontainer->layout()->addWidget(tw);
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

void MinGuiWidget::update_permamsg(std::size_t cur, std::size_t size)
{
    permamsg->setText(QString("Viewing group %1 of %2").arg(cur + 1).arg(size));
}

void MinGuiWidget::save_list()
{
    QString fn = QFileDialog::getSaveFileName(this, "Save list", QString(), "*.txt");
    FILE *f = fopen(fn.toStdString().c_str(), "w");
    for (auto &x : this->marked)
        fprintf(f, "%s\n", x.c_str());
    fclose(f);
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
        QPalette p = imgw[i]->palette();
        if (marks[i])
        {
            p.setColor(QPalette::ColorRole::Window, Qt::GlobalColor::red);
            ++m;
        }
        else
            p.setColor(QPalette::ColorRole::Window, this->palette().color(QPalette::ColorRole::Window));
        imgw[i]->setBackgroundRole(QPalette::ColorRole::Window);
        imgw[i]->setAutoFillBackground(true);
        imgw[i]->setPalette(p);
    }
    if (update_msg)
    sb->showMessage(QString("%1 of %2 marked for deletion").arg(m).arg(current_set.size()), 1000);
}

std::string MinGuiWidget::common_prefix(const std::vector<std::string> &fns)
{
    std::string ret;
    std::string shortest = *std::min_element(fns.begin(), fns.end(), [](auto &a, auto &b){return a.length() < b.length();});
    for (size_t i = 0; i < shortest.length(); ++i)
    {
        char c = shortest[i];
        bool t = true;
        for (auto &s : fns) if (s[i] != c) {t = false; break;}
        if (!t) break;
        ret.push_back(c);
    }
    if (!ret.empty())
    {
        auto p = ret.rfind((char)std::filesystem::path::preferred_separator);
        if (p != std::string::npos)
            return ret.substr(0, p + 1);
    }
    return ret;
}

void MinGuiWidget::keyPressEvent(QKeyEvent *e)
{
    for (auto &k : keys)
    if (e->key() == k)
    {
        if (e->modifiers() & Qt::KeyboardModifier::ShiftModifier)
            this->mark_all_but(&k - &keys[0]);
        else this->mark_toggle(&k - &keys[0]);
    }
    switch (e->key())
    {
        case Qt::Key::Key_X: this->mark_all(); break;
        case Qt::Key::Key_C: this->mark_none(); break;
    }
}

void MinGuiWidget::keyReleaseEvent(QKeyEvent *e)
{
    switch (e->key())
    {
        case Qt::Key::Key_M: Q_EMIT next(); break;
        case Qt::Key::Key_Z: Q_EMIT prev(); break;
        case Qt::Key::Key_Return: if (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) save_list(); break;
    }
}

ImageWidget::ImageWidget(std::string f, std::string dispf, size_t _idx, int max_width, int max_height, QWidget *par)
    : QWidget(par), fn(QString::fromStdString(f)), idx(_idx)
{
    this->setLayout(new QVBoxLayout(this));
    this->layout()->setMargin(10);
    im = new QLabel(this);
    this->layout()->addWidget(im);
    QFile imgf(QString::fromStdString(f));
    QPixmap pm(QString::fromStdString(f));
    int imw = pm.width();
    int imh = pm.height();
    pm.setDevicePixelRatio(this->screen()->devicePixelRatio());
    if (pm.width() > max_width || pm.height() > max_height)
        pm = pm.scaled(max_width, max_height, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
    im->setPixmap(pm);
    im->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    lb = new QLabel(this);
    this->layout()->addWidget(lb);
    QString s = QString("<%1>: %2, %3 x %4, %5")
                .arg(idx < keys.size() ? QKeySequence(keys[idx]).toString(): QString("(No hotkey available)"))
                .arg(QString::fromStdString(dispf/*f.substr(common_pfx.length())*/))
                .arg(imw).arg(imh)
                .arg(QLocale::system().formattedDataSize(imgf.size(), 3));
    lb->setTextFormat(Qt::TextFormat::PlainText);
    lb->setText(s);
    lb->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
}

void ImageWidget::set_marked(bool marked)
{
    QPalette p = this->palette();
    if (marked)
        p.setColor(QPalette::ColorRole::Window, Qt::GlobalColor::red);
    else
        p.setColor(QPalette::ColorRole::Window, qobject_cast<QWidget*>(parent())->palette().color(QPalette::ColorRole::Window));
    this->setBackgroundRole(QPalette::ColorRole::Window);
    this->setAutoFillBackground(true);
    this->setPalette(p);
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (event->modifiers() & Qt::KeyboardModifier::ShiftModifier)
            QDesktopServices::openUrl(QUrl::fromLocalFile(fn));
        else
            Q_EMIT clicked();
    }
}
