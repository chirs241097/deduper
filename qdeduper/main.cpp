#include <QWidget>
#include <QApplication>
#include "mingui.hpp"

DeduperMainWindow *w = nullptr;

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    a.setAttribute(Qt::ApplicationAttribute::AA_UseHighDpiPixmaps);

    w = new DeduperMainWindow();
    w->show();

    a.exec();

    return 0;
}
