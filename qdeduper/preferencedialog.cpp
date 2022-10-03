//Chris Xiong 2022
//License: MPL-2.0
#include <climits>
#include <functional>

#include <QDebug>
#include <QKeyEvent>
#include <QLabel>
#include <QTabWidget>
#include <QGridLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QAction>
#include <QTableView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QListView>
#include <QKeySequenceEdit>
#include <QGroupBox>
#include <QMessageBox>

#include "preferencedialog.hpp"
#include "settings.hpp"

enum ToolbarActionRoles
{
    action_role = Qt::ItemDataRole::UserRole + 1,
    order_role,
    hide_role
};

PreferenceDialog::PreferenceDialog(SettingsRegistry *sr, QWidget *parent) : QDialog(parent)
{
    this->sr = sr;
    tw = new QTabWidget(this);
    bb = new QDialogButtonBox(QDialogButtonBox::StandardButton::Ok | QDialogButtonBox::StandardButton::Cancel, this);
    this->setWindowTitle("Preferences");
    this->setMinimumWidth(480);
    QVBoxLayout *l = new QVBoxLayout();
    this->setLayout(l);
    l->addWidget(tw);
    l->addWidget(bb);
    QObject::connect(bb, &QDialogButtonBox::accepted, this, &PreferenceDialog::accept);
    QObject::connect(bb, &QDialogButtonBox::rejected, this, &PreferenceDialog::reject);
    setup_widgets();
}

void PreferenceDialog::open()
{
    load_widget_status();
    QDialog::open();
}

void PreferenceDialog::accept()
{
    QKeySequence bks;
    switch (verify_shortcuts(&bks))
    {
        case 0:
        break;
        case 1:
            QMessageBox::critical(this, "Ambiguous shortcut found",
                                  QString("Shortcut %1 is assigned to multiple actions.").arg(bks.toString()));
        return;
        case 2:
            QMessageBox::critical(this, "Bad shortcut assignment",
                                  QString("Shortcut %1 has more than one key and cannot be assigned to item actions.").arg(bks.toString()));
        return;
        default:
        break;
    }
    save_widget_status();
    QDialog::accept();
}

void PreferenceDialog::setup_widgets()
{
    for (auto &i : sr->tabs)
    {
        QWidget *container = new QWidget();
        QGridLayout *l = new QGridLayout(container);
        tw->addTab(container, i);
        tabs.push_back(l);
    }
    for (auto &k : sr->klist)
    {
        auto &p = sr->smap[k];
        if (!p.desc.length()) continue;
        QWidget *w = nullptr;
        QGridLayout *l = p.tab < tabs.size() ? tabs[p.tab] : nullptr;
        if (!l) continue;
        switch (p.type)
        {
            case SettingsItem::ParameterType::_int:
            {
                QSpinBox *sb = new QSpinBox();
                sb->setMinimum(p.min.value<int>());
                sb->setMaximum(p.max.value<int>());
                w = sb;
            }
            break;
            case SettingsItem::ParameterType::_bool:
            {
                QCheckBox *cb = new QCheckBox(p.desc);
                w = cb;
            }
            break;
            case SettingsItem::ParameterType::_double:
            {
                QDoubleSpinBox *sb = new QDoubleSpinBox();
                sb->setMinimum(p.min.value<double>());
                sb->setMaximum(p.max.value<double>());
                sb->setSingleStep((sb->maximum() - sb->minimum()) / 100.);
                w = sb;
            }
            break;
            case SettingsItem::ParameterType::_keyseq:
            break;
            default:
            break;
        }
        if (!w) continue;
        p.w = w;
        if (p.type == SettingsItem::ParameterType::_bool)
        {
            l->addWidget(w, l->rowCount(), 0, 1, 2);
        }
        else
        {
            QLabel *lb = new QLabel(p.desc);
            lb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            int r = l->rowCount();
            l->addWidget(lb, r, 0);
            l->addWidget(w, r, 1);
        }
    }
}

void PreferenceDialog::set_hkactions(int tab, const std::vector<std::string> &actlist, const std::map<std::string, QAction*> &actmap)
{
    this->actlist = actlist;
    this->actmap = actmap;
    this->hktv = new QTableView();
    this->hkim = new QStandardItemModel();
    this->tabs[tab]->addWidget(hktv, 0, 0);
    this->hktv->setModel(hkim);
    this->hktv->setSortingEnabled(false);
    this->hktv->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    this->hktv->setItemDelegateForColumn(1, new ShortcutEditorDelegate);

    QGroupBox *gb = new QGroupBox("Action Modifiers");
    QGridLayout *l = new QGridLayout();
    gb->setLayout(l);
    this->tabs[tab]->addWidget(gb, 1, 0);

    const QStringList llist = {"Mark/Unmark", "Mark All Except", "Maximize", "Open with System Viewer", "Open Containing Folder"};

    for (int i = 0; i < llist.size(); ++i)
    {
        auto &lt = llist[i];
        ModifierEdit *me = new ModifierEdit();
        QLabel *lb = new QLabel(lt);
        l->addWidget(lb, i, 0);
        l->addWidget(me, i, 1);
        mes.push_back(me);
    }
}

void PreferenceDialog::set_toolbaractions(int tab, const std::map<std::string, QAction*> &actmap)
{
    QGridLayout *l = this->tabs[tab];
    tbaav = new QListView();
    tbaam = new QStandardItemModel();
    tbapm = new QSortFilterProxyModel();
    tbapm->setSourceModel(tbaam);
    tbapm->setSortRole(ToolbarActionRoles::order_role);
    tbapm->setFilterRole(ToolbarActionRoles::hide_role);
    tbapm->setFilterKeyColumn(0);
    tbapm->setFilterFixedString("0");
    tbaav->setModel(tbapm);
    tbeav = new QListView();
    tbeam = new QStandardItemModel();
    tbeav->setModel(tbeam);
    tbaav->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    tbeav->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    tbaav->setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
    tbeav->setDragDropMode(QAbstractItemView::DragDropMode::InternalMove);
    QPushButton *pbadd = new QPushButton(">");
    QPushButton *pbdel = new QPushButton("<");
    pbadd->setMaximumWidth(24);
    pbdel->setMaximumWidth(24);
    QVBoxLayout *vbl = new QVBoxLayout();
    vbl->addWidget(pbadd);
    vbl->addWidget(pbdel);
    l->addWidget(new QLabel("Available Buttons"), 0, 0);
    l->addWidget(new QLabel("Enabled Buttons"), 0, 2);
    l->addWidget(tbaav, 1, 0);
    l->addLayout(vbl, 1, 1);
    l->addWidget(tbeav, 1, 2);
    l->addWidget(new QLabel("Drag to sort enabled buttons."), 2, 0, 1, 3);
    tbaam->clear();
    for (size_t i = 0; i < actlist.size(); ++i)
    {
        auto &actn = actlist[i];
        QAction *act = this->actmap[actn];
        QStandardItem *itm = new QStandardItem(act->text());
        itm->setIcon(act->icon());
        itm->setData(QString::fromStdString(actn), ToolbarActionRoles::action_role);
        itm->setData(QVariant::fromValue<size_t>(i), ToolbarActionRoles::order_role);
        itm->setData("0", ToolbarActionRoles::hide_role);
        tbaam->appendRow(itm);
    }
    QObject::connect(pbadd, &QPushButton::clicked, [this] {
        QModelIndex idx = tbaav->currentIndex();
        if (!idx.isValid()) return;
        QString actn = idx.data(ToolbarActionRoles::action_role).value<QString>();
        QAction *act = this->actmap[actn.toStdString()];
        QStandardItem *itm = new QStandardItem(act->text());
        itm->setIcon(act->icon());
        itm->setData(actn, ToolbarActionRoles::action_role);
        itm->setDropEnabled(false);
        tbeam->appendRow(itm);
        tbaam->setData(tbapm->mapToSource(idx), "1", ToolbarActionRoles::hide_role);
    });
    QObject::connect(pbdel, &QPushButton::clicked, [this] {
        QModelIndex idx = tbeav->currentIndex();
        if (!idx.isValid()) return;
        QString actn = idx.data(ToolbarActionRoles::action_role).value<QString>();
        for (int i = 0; i < tbaam->rowCount(); ++i)
        {
            const auto &idx = tbaam->index(i, 0);
            if (tbaam->data(idx, ToolbarActionRoles::action_role).value<QString>() == actn)
                tbaam->setData(idx, "0", ToolbarActionRoles::hide_role);
        }
        tbeam->removeRows(idx.row(), 1);
    });
}

void PreferenceDialog::load_widget_status()
{
    for (auto &k : sr->klist)
    {
        auto &p = sr->smap[k];
        QWidget *w = p.w;
        switch (p.type)
        {
            case SettingsItem::ParameterType::_int:
            {
                QSpinBox *sb = qobject_cast<QSpinBox*>(w);
                if (!sb) continue;
                sb->setValue(sr->get_option_int(p.key));
            }
            break;
            case SettingsItem::ParameterType::_bool:
            {
                QCheckBox *cb = qobject_cast<QCheckBox*>(w);
                cb->setChecked(sr->get_option_bool(p.key));
            }
            break;
            case SettingsItem::ParameterType::_double:
            {
                QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(w);
                sb->setValue(sr->get_option_double(p.key));
            }
            break;
            case SettingsItem::ParameterType::_keyseq:
            break;
            default:
            break;
        }
    }
    this->hkim->clear();
    this->hkim->setHorizontalHeaderLabels({"Menu Item", "Hotkey"});
    for (auto &actn : this->actlist)
    {
        QKeySequence ks = sr->get_option_keyseq("hotkey/" + actn);
        if (this->actmap.find(actn) == this->actmap.end())
            continue;
        QAction *act = this->actmap[actn];
        QStandardItem *itma = new QStandardItem(act->text());
        QStandardItem *itmk = new QStandardItem(ks.toString());
        itma->setIcon(act->icon());
        itma->setEditable(false);
        itma->setData(QString::fromStdString(actn), Qt::ItemDataRole::UserRole);
        itmk->setData(QVariant::fromValue<QKeySequence>(ks), Qt::ItemDataRole::UserRole);
        this->hkim->appendRow({itma, itmk});
    }
    this->hktv->resizeColumnsToContents();
    for (size_t i = 0; i < 16; ++i)
    {
        std::string iakt = "item_" + std::to_string(i) + "_action_key";
        int ik = sr->get_option_int("hotkey/" + iakt);
        QKeySequence ks(ik);
        QStandardItem *itma = new QStandardItem(QString("Item %1 Action Key").arg(i + 1));
        QStandardItem *itmk = new QStandardItem(ks.toString());
        itma->setEditable(false);
        itma->setData(QString::fromStdString(iakt), Qt::ItemDataRole::UserRole);
        itmk->setData(QVariant::fromValue<QKeySequence>(ks), Qt::ItemDataRole::UserRole);
        this->hkim->appendRow({itma, itmk});
    }
    for (size_t i = 0; i < 5; ++i)
    {
        std::string iamt = "hotkey/item_action_mod_" + std::to_string(i);
        int im = sr->get_option_int(iamt);
        mes[i]->set_modifier(static_cast<Qt::Modifier>(im));
    }
    for (int i = 0; i < tbaam->rowCount(); ++i)
    {
        const auto &idx = tbaam->index(i, 0);
        tbaam->setData(idx, "0", ToolbarActionRoles::hide_role);
    }
    auto tbal = sr->get_option_strlist("toolbar_actions");
    this->tbeam->clear();
    for (auto &tban : tbal)
    {
        QAction *act = this->actmap[tban.toStdString()];
        QStandardItem *itm = new QStandardItem(act->text());
        itm->setIcon(act->icon());
        itm->setData(tban, ToolbarActionRoles::action_role);
        itm->setDropEnabled(false);
        for (int i = 0; i < tbaam->rowCount(); ++i)
        {
            const auto &idx = tbaam->index(i, 0);
            if (tbaam->data(idx, ToolbarActionRoles::action_role).value<QString>() == tban)
                tbaam->setData(idx, "1", ToolbarActionRoles::hide_role);
        }
        tbeam->appendRow(itm);
    }
}

void PreferenceDialog::save_widget_status()
{
    for (auto &k : sr->klist)
    {
        auto &p = sr->smap[k];
        QWidget *w = p.w;
        switch (p.type)
        {
            case SettingsItem::ParameterType::_int:
            {
                QSpinBox *sb = qobject_cast<QSpinBox*>(w);
                if (!sb) continue;
                sr->set_option_int(p.key, sb->value());
            }
            break;
            case SettingsItem::ParameterType::_bool:
            {
                QCheckBox *cb = qobject_cast<QCheckBox*>(w);
                sr->set_option_bool(p.key, cb->isChecked());
            }
            break;
            case SettingsItem::ParameterType::_double:
            {
                QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(w);
                sr->set_option_double(p.key, sb->value());
            }
            break;
            case SettingsItem::ParameterType::_keyseq:
            break;
            default:
            break;
        }
    }
    for (int i = 0; i < hkim->rowCount(); ++i)
    {
        QString actn = hkim->item(i, 0)->data(Qt::ItemDataRole::UserRole).toString();
        QKeySequence ks = hkim->item(i, 1)->data(Qt::ItemDataRole::UserRole).value<QKeySequence>();
        if (actn.endsWith("_action_key"))
            sr->set_option_int("hotkey/" + actn.toStdString(), ks[0]);
        else sr->set_option_keyseq("hotkey/" + actn.toStdString(), ks);
    }
    for (size_t i = 0; i < 5; ++i)
    {
        std::string iamt = "hotkey/item_action_mod_" + std::to_string(i);
        sr->set_option_int(iamt, mes[i]->get_modifier());
    }
    QStringList tbal;
    for (int i = 0; i < tbeam->rowCount(); ++i)
        tbal.push_back(tbeam->item(i)->data(ToolbarActionRoles::action_role).value<QString>());
    sr->set_option_strlist("toolbar_actions", tbal);
}

int PreferenceDialog::verify_shortcuts(QKeySequence *bks)
{
    QList<QKeySequence> ksl;
    for (int i = 0; i < hkim->rowCount(); ++i)
    {
        QString actn = hkim->item(i, 0)->data(Qt::ItemDataRole::UserRole).toString();
        QKeySequence ks = hkim->item(i, 1)->data(Qt::ItemDataRole::UserRole).value<QKeySequence>();
        if (actn.endsWith("_action_key"))
        {
            if (ks.count() > 1)
            {
                *bks = ks;
                return 2;
            }
            for (int j = 0;j < 5; ++j)
                if (mes[j]->get_modifier() != INT_MAX)
                    ksl.push_back(QKeySequence(mes[j]->get_modifier() | ks[0]));
        }
        else ksl.push_back(hkim->item(i, 1)->data(Qt::ItemDataRole::UserRole).value<QKeySequence>());
    }
    for (int i = 0; i < ksl.size(); ++i)
        for (int j = i + 1; j < ksl.size(); ++j)
            if (!ksl[i].isEmpty() && ksl[i] == ksl[j])
            {
                *bks = ksl[i];
                return 1;
            }
    return 0;
}

QWidget* ShortcutEditorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    QKeySequenceEdit *kse = new QKeySequenceEdit(parent);
    QObject::connect(kse, &QKeySequenceEdit::editingFinished, [this, kse] {
        Q_EMIT const_cast<ShortcutEditorDelegate*>(this)->commitData(kse);
        Q_EMIT const_cast<ShortcutEditorDelegate*>(this)->closeEditor(kse);
    });
    return kse;
}

void ShortcutEditorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QKeySequenceEdit *kse = qobject_cast<QKeySequenceEdit*>(editor);
    kse->setKeySequence(index.data(Qt::ItemDataRole::UserRole).value<QKeySequence>());
}

void ShortcutEditorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QKeySequenceEdit *kse = qobject_cast<QKeySequenceEdit*>(editor);
    model->setData(index, QVariant::fromValue<QKeySequence>(kse->keySequence()), Qt::ItemDataRole::UserRole);
    model->setData(index, kse->keySequence().toString(), Qt::ItemDataRole::DisplayRole);
}

ModifierEdit::ModifierEdit(QWidget *par) : QPushButton(par)
{
    this->setCheckable(true);
    this->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    this->mod = static_cast<Qt::Modifier>(0);
    QObject::connect(this, &QPushButton::toggled, [this](bool c) {
        if (c)
        {
            if (int(this->mod) == INT_MAX)
                this->set_modifier(static_cast<Qt::Modifier>(0));
            this->setText("Input modifier...");
        }
        else
            this->set_modifier(this->mod);
    });
}

Qt::Modifier ModifierEdit::get_modifier()
{
    return this->mod;
}

void ModifierEdit::set_modifier(Qt::Modifier mod)
{
    this->mod = mod;
    int imod = mod;
    switch (imod)
    {
        case 0:
            if (this->isChecked())
                this->setText("Input modifier...");
            else
                this->setText("No modifier");
        break;
        case INT_MAX:
            this->setText("Disabled");
        break;
        default:
            QString kss = QKeySequence(mod).toString();
            while (kss.length() > 0 && kss.back() == '+') kss.chop(1);
            this->setText(kss);
    }
}

void ModifierEdit::keyPressEvent(QKeyEvent *e)
{
    if (!this->isChecked()) return QPushButton::keyPressEvent(e);
    switch (e->key())
    {
        case Qt::Key::Key_Enter:
        case Qt::Key::Key_Return:
        case Qt::Key::Key_Space:
            this->set_modifier(static_cast<Qt::Modifier>(Qt::KeyboardModifiers::Int(e->modifiers())));
            this->setChecked(false);
        break;
        case Qt::Key::Key_Escape:
            this->set_modifier(static_cast<Qt::Modifier>(0));
            this->setChecked(false);
        break;
        case Qt::Key::Key_Backspace:
        case Qt::Key::Key_Delete:
            this->set_modifier(static_cast<Qt::Modifier>(INT_MAX));
            this->setChecked(false);
        break;
    }
}

bool ModifierEdit::event(QEvent *e)
{
    if (e->type() == QEvent::Type::ShortcutOverride)
    {
        e->accept();
        return true;
    }
    return QPushButton::event(e);
}
