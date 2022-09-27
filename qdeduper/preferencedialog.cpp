#include <QLabel>
#include <QTabWidget>
#include <QGridLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QVBoxLayout>

#include "preferencedialog.hpp"
#include "settings.hpp"

PreferenceDialog::PreferenceDialog(SettingsRegistry *sr, QWidget *parent) : QDialog(parent)
{
    this->sr = sr;
    tw = new QTabWidget(this);
    bb = new QDialogButtonBox(QDialogButtonBox::StandardButton::Ok | QDialogButtonBox::StandardButton::Cancel, this);
    this->setWindowTitle("Preferences");
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
}
