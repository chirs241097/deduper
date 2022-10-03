//Chris Xiong 2022
//License: MPL-2.0
#include "imageitem.hpp"

#include <algorithm>

#include <QDebug>
#include <QFileInfo>
#include <QPixmap>
#include <QListView>
#include <QFontMetrics>
#include <QPainter>
#include <QLocale>

#define DEBUGPAINT 0

ImageItem::ImageItem(QString fn, QString dispn, QKeySequence hotkey, size_t dbid, size_t ord, double pxratio)
{
    this->setText(dispn);
    this->setData(fn, ImageItemRoles::path_role);
    this->setData(QFileInfo(fn).size(), ImageItemRoles::file_size_role);
    this->setCheckable(true);
    QPixmap pm(fn);
    pm.setDevicePixelRatio(pxratio);
    this->setData(pm.size(), ImageItemRoles::dimension_role);
    this->setData(hotkey, ImageItemRoles::hotkey_role);
    this->setData(pm, Qt::ItemDataRole::DecorationRole);
    this->setData(QVariant::fromValue<size_t>(dbid), ImageItemRoles::database_id_role);
    this->setData(QVariant::fromValue<size_t>(ord), ImageItemRoles::default_order_role);
    this->setData(QVariant::fromValue<quint64>(1ULL * pm.size().width() * pm.size().height()), ImageItemRoles::pixelcnt_role);
}

QString ImageItem::path() const
{
    return this->data(ImageItemRoles::path_role).toString();
}

size_t ImageItem::database_id() const
{
    return this->data(ImageItemRoles::database_id_role).value<size_t>();
}

size_t ImageItem::default_order() const
{
    return this->data(ImageItemRoles::default_order_role).value<size_t>();
}

QKeySequence ImageItem::hotkey() const
{
    return this->data(ImageItemRoles::hotkey_role).value<QKeySequence>();
}

void ImageItem::set_hotkey(QKeySequence hk)
{
    this->setData(hk, ImageItemRoles::hotkey_role);
}

void ImageItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect imr = option.rect;
    imr.adjust(MARGIN + BORDER, MARGIN + BORDER, -MARGIN - BORDER, -MARGIN - BORDER);
    QFont bfnt(option.font);
    bfnt.setBold(true);
    QFontMetrics bfm(bfnt);
    imr.adjust(0, 0, 0, -2 * HKPADD - bfm.height() - LINESP);
    QTextOption topt;
    topt.setWrapMode(QTextOption::WrapMode::NoWrap);
    topt.setAlignment(Qt::AlignmentFlag::AlignHCenter);

    QRect selr = option.rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);
    QStyleOptionViewItem so(option);
    so.rect = selr;
    if (index.data(Qt::ItemDataRole::CheckStateRole).value<Qt::CheckState>() == Qt::CheckState::Checked)
        so.state |= QStyle::StateFlag::State_Selected;
    so.state |= QStyle::StateFlag::State_Active;
    option.widget->style()->drawPrimitive(QStyle::PrimitiveElement::PE_PanelItemViewItem, &so, painter, option.widget);

    QPixmap pm = index.data(Qt::ItemDataRole::DecorationRole).value<QPixmap>();
    QSize imd = pm.size().scaled(imr.size(), Qt::AspectRatioMode::KeepAspectRatio);
    painter->setRenderHint(QPainter::RenderHint::SmoothPixmapTransform);
    if (pm.size().width() > imd.width() * pm.devicePixelRatioF() || pm.size().height() > imd.height() * pm.devicePixelRatioF())
    {
        QPixmap pms = pm.scaled(imd * pm.devicePixelRatioF(), Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
        painter->drawPixmap(QRect(imr.topLeft(), imd), pms);
    }
    else
        painter->drawPixmap(QRect(imr.topLeft(), imd), pm);
    QPoint dtopright = QRect(imr.topLeft(),imd).bottomLeft();

    QPoint hko = dtopright + QPoint(HKPADD, HKPADD + LINESP);
    QKeySequence ks = index.data(ImageItem::ImageItemRoles::hotkey_role).value<QKeySequence>();
    QString kss = ks.isEmpty() ? "(-)" : ks.toString();
    if (!show_hotkey)
        kss = QString::number(index.row() + 1);
    QRect r = bfm.boundingRect(kss);
    r.moveTopLeft(hko);
    QRect hkbg = r.adjusted(-HKPADD, -HKPADD, HKPADD, HKPADD);
    if (hkbg.width() < hkbg.height())
    {
        int shift = (hkbg.height() - hkbg.width()) / 2;
        r.adjust(shift, 0, shift, 0);
        hkbg.setWidth(hkbg.height());
    }
    painter->setPen(QPen(QBrush(), 0));
    painter->setBrush(option.widget->palette().color(QPalette::ColorGroup::Normal, QPalette::ColorRole::Light));
    painter->drawRoundedRect(hkbg.adjusted(HKSHDS, HKSHDS, HKSHDS, HKSHDS), 4, 4);
    painter->setBrush(option.widget->palette().color(QPalette::ColorGroup::Normal, QPalette::ColorRole::WindowText));
    painter->drawRoundedRect(hkbg, 4, 4);
    painter->setPen(option.widget->palette().color(QPalette::ColorGroup::Normal, QPalette::ColorRole::Window));
    painter->setBrush(QBrush());
    painter->setFont(bfnt);
    painter->drawText(r, kss, topt);
#if DEBUGPAINT
    painter->setPen(QPen(Qt::GlobalColor::red));
    painter->drawRect(r);
    painter->drawRect(hkbg);
#endif

    QPoint ftopright = hkbg.topRight() + QPoint(LINESP + HKSHDS, 0);
    QSize dim = index.data(ImageItem::ImageItemRoles::dimension_role).value<QSize>();
    qint64 fsz = index.data(ImageItem::ImageItemRoles::file_size_role).value<qint64>();
    QString infos = QString("%1 x %2, %3")
                    .arg(dim.width()).arg(dim.height())
                    .arg(QLocale::system().formattedDataSize(fsz, 3));
    QString fns = index.data(Qt::ItemDataRole::DisplayRole).toString();
    r = option.fontMetrics.boundingRect(infos);
    r.moveTopLeft(ftopright + QPoint(0, (hkbg.height() - r.height()) / 2));
    painter->setFont(option.font);
    painter->setPen(option.widget->palette().color(QPalette::ColorGroup::Normal, QPalette::ColorRole::Text));
    painter->drawText(r, infos, topt);
    topt.setAlignment(Qt::AlignmentFlag::AlignRight);
    r.setLeft(r.right());
    r.setRight(imr.right());
    QString efns = option.fontMetrics.elidedText(fns, Qt::TextElideMode::ElideMiddle, r.width());
    painter->drawText(r, efns, topt);
#if DEBUGPAINT
    painter->setPen(QPen(Qt::GlobalColor::red));
    painter->drawRect(so.rect);
    painter->drawRect(option.rect);
    qDebug() << "paint" << index.row() << option.rect;
#endif
}

QSize ImageItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QListView *lw = qobject_cast<const QListView*>(option.widget);
    QSize vpsz = lw->maximumViewportSize();
    vpsz.setWidth(vpsz.width() - vw);
    vpsz.setHeight(vpsz.height() - hh);
    QPixmap pm = index.data(Qt::ItemDataRole::DecorationRole).value<QPixmap>();
    QSize onscsz = pm.size() / pm.devicePixelRatioF();
    int imh = onscsz.height();
    if (onscsz.width() > vpsz.width() - 2 * MARGIN - 2 * BORDER)
        imh = (vpsz.width() - 2 * MARGIN - 2 * BORDER) / (double)onscsz.width() * onscsz.height();

    QFont fnt(option.font);
    fnt.setBold(true);
    QFontMetrics fm(fnt);
    int extra_height = 2 * MARGIN + 2 * BORDER + LINESP + fm.height() + 2 * HKPADD + HKSHDS;
    int max_height = imh;

    QSize dim = index.data(ImageItem::ImageItemRoles::dimension_role).value<QSize>();
    qint64 fsz = index.data(ImageItem::ImageItemRoles::file_size_role).value<qint64>();
    QString infos = QString("%1 x %2, %3")
                    .arg(dim.width()).arg(dim.height())
                    .arg(QLocale::system().formattedDataSize(fsz, 3));
    int textw = option.fontMetrics.boundingRect(infos).width() + fm.height() + 2 * HKPADD + 48;

    QSize ret(vpsz);
    if (textw > vpsz.width()) ret.setWidth(textw);
    ret.setHeight(vpsz.height() / index.model()->rowCount() - lw->spacing());
    ret.setHeight(std::max(min_height + extra_height, ret.height()));
    ret.setHeight(std::min(max_height + extra_height, ret.height()));
    if (singlemode) ret.setHeight(vpsz.height());
#if DEBUGPAINT
    qDebug() << "sizehint" << index.row() << ret;
#endif
    return ret;
}

void ImageItemDelegate::resize()
{
    if (im)
        for (int i = 0; i < im->rowCount(); ++i)
            Q_EMIT sizeHintChanged(im->index(i, 0));
}

void ImageItemDelegate::set_scrollbar_margins(int vw, int hh)
{
    this->vw = vw;
    this->hh = hh;
}

void ImageItemDelegate::set_min_height(int mh)
{
    if (mh != this->min_height)
    {
        this->min_height = mh;
        resize();
    }
}

void ImageItemDelegate::set_single_item_mode(bool enabled)
{
    if (enabled == singlemode) return;
    singlemode = enabled;
    resize();
}

bool ImageItemDelegate::is_single_item_mode()
{
    return singlemode;
}

void ImageItemDelegate::set_show_hotkey(bool show)
{
    show_hotkey = show;
}

void ImageItemDelegate::set_model(QAbstractItemModel *m)
{
    im = m;
}
