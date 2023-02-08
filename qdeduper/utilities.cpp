//Chris Xiong 2022
//License: MPL-2.0
#include "utilities.hpp" 

#include <QDesktopServices>
#include <QProcess>
#include <QUrl>
#include <QDebug>
#ifdef HAS_QTDBUS
#include <QDBusConnection>
#include <QDBusMessage>
#endif
#ifdef _WIN32
#include <QDir>
#endif

namespace utilities
{

QString fspath_to_qstring(const fs::path &p)
{
    return fsstr_to_qstring(p.native());
}

QString fsstr_to_qstring(const fs::path::string_type &s)
{
#if PATH_VALSIZE == 2 //the degenerate platform
    return QString::fromStdWString(s);
#else
    return QString::fromStdString(s);
#endif
}

fs::path qstring_to_path(const QString &s)
{
#if PATH_VALSIZE == 2 //the degenerate platform
    return fs::path(s.toStdWString());
#else
    return fs::path(s.toStdString());
#endif
}

void open_containing_folder(const fs::path &path)
{
#ifdef _WIN32
    QProcess::startDetached("explorer", QStringList() << "/select," << QDir::toNativeSeparators(fspath_to_qstring(path)));
#else
#ifdef HAS_QTDBUS
    auto conn = QDBusConnection::sessionBus();
    auto call = QDBusMessage::createMethodCall(
        "org.freedesktop.FileManager1",
        "/org/freedesktop/FileManager1",
        "org.freedesktop.FileManager1",
        "ShowItems"
    );
    QVariantList args = {
        QStringList({fspath_to_qstring(path)}),
        QString()
    };
    call.setArguments(args);
    auto resp = conn.call(call, QDBus::CallMode::Block, 500);
    if (resp.type() != QDBusMessage::MessageType::ErrorMessage)
        return;
#endif
    auto par = (path / "../").lexically_normal();
    QDesktopServices::openUrl(QUrl::fromLocalFile(fspath_to_qstring(par)));
#endif
}

};
