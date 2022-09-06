#ifndef IMAGEITEM_HPP
#define IMAGEITEM_HPP

#include <QStandardItem>
#include <QAbstractItemDelegate>
#include <QStyleOptionViewItem>
#include <QModelIndex>

class ImageItem : public QStandardItem
{
public:
    enum ImageItemRoles
    {
        path_role = Qt::ItemDataRole::UserRole + 0x1000,
        dimension_role,
        file_size_role,
        hotkey_role
    };
    ImageItem(QString fn, QString dispn, QKeySequence hotkey, double pxratio = 1.0);
};

class ImageItemDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
private:
    const static int MARGIN = 3;
    const static int BORDER = 3;
    const static int HKPADD = 4;
    const static int LINESP = 4;
    const static int HKSHDS = 2;
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void resize(const QModelIndex &index);
Q_SIGNALS:
    void sizeHintChanged(const QModelIndex &index);
};

#endif
