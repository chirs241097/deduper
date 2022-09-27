#ifndef PREFERENCEDIALOG_HPP
#define PREFERENCEDIALOG_HPP

#include <vector>

#include <QDialog>
#include <QGridLayout>

#include "settings.hpp"

class QTabWidget;
class QGridLayout;
class QDialogButtonBox;

class PreferenceDialog : public QDialog
{
    Q_OBJECT
public:
    PreferenceDialog(SettingsRegistry *sr, QWidget *parent = nullptr);
    void setup_widgets();
    void load_widget_status();
    void save_widget_status();
    void open() override;
    void accept() override;
private:
    SettingsRegistry *sr;
    QTabWidget *tw;
    std::vector<QGridLayout*> tabs;
    QDialogButtonBox *bb;
};

#endif
