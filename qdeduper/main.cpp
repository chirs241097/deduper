#include <cstdio>
#include <algorithm>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

#include <QWidget>
#include <QApplication>
#include "mingui.hpp"

using std::size_t;

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
