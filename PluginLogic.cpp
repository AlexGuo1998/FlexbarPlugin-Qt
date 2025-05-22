#include "PluginLogic.h"

#include <QtGui/QPainter>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

#include "PluginTransport.h"

PluginLogic::PluginLogic(PluginTransport *transport) : transport(transport) {
    transport->methodOn<&PluginLogic::onDeviceStatus>(QStringLiteral("device.status"), this);
    transport->methodOn<&PluginLogic::onPluginAlive>(QStringLiteral("plugin.alive"), this);
}

void PluginLogic::onDeviceStatus(const QJsonValue &message, QJsonValue &result) {
    // PluginOperation(transport).showSnackbarMessage(
    //     "success", "deviceStatusChanged (from FlexbarPlugin-Qt)");
}

void PluginLogic::onPluginAlive(const QJsonValue &message, QJsonValue &result) {
    // do {
    //     auto stream = qDebug();
    //     QDebugStateSaver saver(stream);
    //     stream.noquote()
    //             << "alive"
    //             << QString::fromUtf8(QJsonDocument(message.toObject()).toJson());
    // } while (false);

    auto serial = message[QLatin1String("serialNumber")].toString();
    auto keys = message[QLatin1String("keys")].toArray();
    for (const QJsonValueRef key: keys) {
        auto uid = key.toObject()["uid"].toInt();

        QPixmap pixmap(key.toObject()["width"].toInt(), 60);
        pixmap.fill(Qt::black);

        QPainter painter(&pixmap);
        painter.setRenderHints(QPainter::RenderHint::Antialiasing | QPainter::RenderHint::SmoothPixmapTransform);

        QFont font("Segoe UI");
        font.setPixelSize(40);
        painter.setFont(font);

        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        painter.drawRoundedRect(pixmap.rect(), 8, 8);

        painter.setPen(Qt::black);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "Hello Qt!");
        painter.end();
        PluginOperation(transport).draw(serial, uid, pixmap);
    }
}
