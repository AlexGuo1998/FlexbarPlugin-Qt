#include "app.h"

#include <iostream>
#include <QtCore/QRegularExpression>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include "PluginTransport.h"

int qt_main(std::vector<std::string> &args) {
    // Parse the args
    if (args.size() <= 1) return 1;

    const QRegularExpression p(QLatin1String("^--port=(\\d+)$"));
    const QRegularExpression u(QLatin1String("^--uid=(.+)$"));
    const QRegularExpression d(QLatin1String("^--dir=(.+)$"));

    uint16_t port = 0;
    QString uid;
    QString dir;
    bool bp = false, bu = false, bd = false;

    for (auto it = args.cbegin() + 1; it != args.cend(); ++it) {
        const std::string &arg = *it;
        auto qArg = QString::fromStdString(arg);

        auto match = p.match(qArg);
        if (match.hasMatch()) {
            port = match.captured(1).toUInt();
            bp = true;
            continue;
        }
        match = u.match(qArg);
        if (match.hasMatch()) {
            uid = match.captured(1);
            bu = true;
            continue;
        }
        match = d.match(qArg);
        if (match.hasMatch()) {
            dir = match.captured(1);
            bd = true;
            continue;
        }
    }

    if (!(bp && bu && bd)) return 1;

    return qt_main(port, uid, dir);
}

void deviceStatusChanged(void *context, const QJsonValue &message, QJsonValue &result) {
    auto stream = qDebug();
    QDebugStateSaver saver(stream);
    stream.noquote()
            << "deviceStatusChanged"
            << QString::fromUtf8(
                (message.isArray()
                     ? QJsonDocument(message.toArray())
                     : QJsonDocument(message.toObject())
                ).toJson());
}

void deviceAlive(void *context, const QJsonValue &message, QJsonValue &result) {
    auto stream = qDebug();
    QDebugStateSaver saver(stream);
    stream.noquote()
            << "external alive"
            << QString::fromUtf8(
                (message.isArray()
                     ? QJsonDocument(message.toArray())
                     : QJsonDocument(message.toObject())
                ).toJson());
}

int qt_main(uint16_t port, QString uid, QString dir) {
    // TODO config qInstallMessageHandler
    // Create the app
    QCoreApplication::addLibraryPath(dir + "/backend");

    int argc = 1;
    char arg[] = "app";
    char *argv[1] = {arg};
    QApplication app(argc, argv);

    PluginTransport conn(port, uid);
    conn.on(QStringLiteral("device.status"), deviceStatusChanged, nullptr);
    conn.on(QStringLiteral("plugin.alive"), deviceAlive, nullptr);
    conn.test();
    conn.start();

    // Your logic here
    QWidget widget;
    widget.setWindowTitle("Hello World");
    widget.show();

    return app.exec();
}
