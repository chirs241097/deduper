//Chris Xiong 2022
//License: MPL-2.0
#include <QWidget>
#include <QApplication>
#include "mingui.hpp"

DeduperMainWindow *w = nullptr;

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    a.setAttribute(Qt::ApplicationAttribute::AA_UseHighDpiPixmaps);
#endif

    w = new DeduperMainWindow();
    w->show();

    a.exec();

    return 0;
}
