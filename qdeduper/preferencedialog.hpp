#ifndef PREFERENCEDIALOG_HPP
#define PREFERENCEDIALOG_HPP

#include <vector>
#include <map>
#include <string>

#include <QPushButton>
#include <QDialog>
#include <QGridLayout>
#include <QStyledItemDelegate>

#include "settings.hpp"

class QTabWidget;
class QGridLayout;
class QDialogButtonBox;
class QTableView;
class QStandardItemModel;

class ModifierEdit : public QPushButton
{
    Q_OBJECT
public:
    ModifierEdit(QWidget *par = nullptr);
    Qt::Modifier get_modifier();
    void set_modifier(Qt::Modifier mod);
    bool event(QEvent *e) override;
protected:
    void keyPressEvent(QKeyEvent *e) override;
private:
    Qt::Modifier mod;
};

class PreferenceDialog : public QDialog
{
    Q_OBJECT
public:
    PreferenceDialog(SettingsRegistry *sr, QWidget *parent = nullptr);
    void setup_widgets();
    void set_hkactions(int tab, std::map<std::string, QKeySequence> defmap, std::map<std::string, QAction*> actmap);
    void load_widget_status();
    void save_widget_status();

public Q_SLOTS:
    void open() override;
    void accept() override;
private:
    int verify_shortcuts(QKeySequence *bks);

    SettingsRegistry *sr;
    QTabWidget *tw;
    std::vector<QGridLayout*> tabs;
    QDialogButtonBox *bb;
    QTableView *hktv = nullptr;
    QStandardItemModel *hkim = nullptr;
    std::map<std::string, QKeySequence> defmap;
    std::map<std::string, QAction*> actmap;
    std::vector<ModifierEdit*> mes;
};

class ShortcutEditorDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
};

#endif
