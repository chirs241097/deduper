#ifndef IMAGEITEM_HPP
#define IMAGEITEM_HPP

#include <QStandardItem>
#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
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
        hotkey_role,
        database_id_role,
        default_order_role,
        pixelcnt_role
    };
    ImageItem(QString fn, QString dispn, QKeySequence hotkey, size_t dbid, size_t ord, double pxratio = 1.0);

    QString path() const;
    size_t database_id() const;
    size_t default_order() const;
    QKeySequence hotkey() const;
    void set_hotkey(QKeySequence hk);
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
    int vw = -1;
    int hh = -1;
    int min_height = 64;
    bool singlemode = false;
    bool show_hotkey = true;
    QAbstractItemModel *im = nullptr;
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void resize();
    void set_scrollbar_margins(int vw, int hh);
    void set_min_height(int mh);

    void set_single_item_mode(bool enabled);
    bool is_single_item_mode();

    void set_show_hotkey(bool show);

    void set_model(QAbstractItemModel *m);
Q_SIGNALS:
    void sizeHintChanged(const QModelIndex &index);
};

#endif
