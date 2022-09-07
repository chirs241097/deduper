#include "imageitem.hpp"

#include <algorithm>

#include <QDebug>
#include <QFileInfo>
#include <QPixmap>
#include <QListView>
#include <QScrollBar>
#include <QFontMetrics>
#include <QPainter>
#include <QLocale>

ImageItem::ImageItem(QString fn, QString dispn, QKeySequence hotkey, double pxratio)
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
}

void ImageItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect imr = option.rect;
    imr.adjust(MARGIN + BORDER, MARGIN + BORDER, -MARGIN - BORDER, -MARGIN - BORDER);
    QFont bfnt(option.font);
    bfnt.setBold(true);
    QFontMetrics bfm(bfnt);
    imr.adjust(0, 0, 0, -2 * HKPADD - bfm.height() - LINESP);

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
    painter->drawPixmap(QRect(imr.topLeft(), imd), pm);
    QPoint dtopright = QRect(imr.topLeft(),imd).bottomLeft();

    QPoint hko = dtopright + QPoint(HKPADD, HKPADD + LINESP);
    QKeySequence ks = index.data(ImageItem::ImageItemRoles::hotkey_role).value<QKeySequence>();
    QString kss = ks.toString();
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
    painter->drawText(r, kss);

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
    painter->drawText(r, infos);
    r = option.fontMetrics.boundingRect(fns);
    r.moveTopRight(QPoint(option.rect.right() - MARGIN - BORDER, ftopright.y() + (hkbg.height() - r.height()) / 2));
    painter->drawText(r, fns);
    /*
    painter->setPen(QColor(Qt::GlobalColor::red));
    painter->drawRect(QRect(imr.topLeft(), imd));
    painter->drawRect(txt);
    painter->setPen(option.widget->palette().color(QPalette::ColorRole::Text));
    */
}

QSize ImageItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QListView *lw = qobject_cast<const QListView*>(option.widget);
    QSize vpsz = lw->maximumViewportSize();
    QScrollBar *vsc = lw->verticalScrollBar();
    if (vsc->isVisible())
        vpsz.setWidth(vpsz.width() - vsc->size().width());
    QScrollBar *hsc = lw->horizontalScrollBar();
    if (hsc->isVisible())
        vpsz.setHeight(vpsz.height() - vsc->size().height());
    QPixmap pm = index.data(Qt::ItemDataRole::DecorationRole).value<QPixmap>();
    QSize onscsz = pm.size() / pm.devicePixelRatioF();
    int imh = onscsz.height();
    if (onscsz.width() > vpsz.width() - 2 * MARGIN - 2 * BORDER)
        imh = (vpsz.width() - 2 * MARGIN - 2 * BORDER) / (double)onscsz.width() * onscsz.height();

    QFont fnt(option.font);
    fnt.setBold(true);
    QFontMetrics fm(fnt);
    int extra_height = 2 * MARGIN + 2 * BORDER + LINESP + fm.height() + 2 * HKPADD + HKSHDS;
    int min_height = 64;
    int max_height = imh;

    QSize ret(vpsz);
    ret.setHeight(vpsz.height() / index.model()->rowCount() - lw->spacing());
    ret.setHeight(std::max(min_height + extra_height, ret.height()));
    ret.setHeight(std::min(max_height + extra_height, ret.height()));
    return ret;
}

void ImageItemDelegate::resize(const QModelIndex &index)
{
    Q_EMIT sizeHintChanged(index);
}