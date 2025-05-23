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
    do {
        auto stream = qDebug();
        QDebugStateSaver saver(stream);
        stream.noquote()
                << "alive"
                << QString::fromUtf8(QJsonDocument(message.toObject()).toJson());
    } while (false);

    auto serial = message[QLatin1String("serialNumber")].toString();
    auto keys = message[QLatin1String("keys")].toArray();
    for (const QJsonValueRef keyRef: keys) {
        QJsonObject key = keyRef.toObject();
        auto uid = key["uid"].toInt();
        int width = key["width"].toInt();

        QPixmap pixmap(width, 60);
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
        // PluginOperation(transport).draw(serial, uid, pixmap);

        if (key["cid"] == "com.alexguo1998.flexbarplugin-qt.counter") {
            device = {serial, uid, width};
        }
    }
}

void PluginLogic::onMediaInfoUpdate(QString info, QImage cover) {
    auto &[serial, uid, width] = device;
    QPixmap pixmap(width, 60);
    pixmap.fill(Qt::black);

    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::RenderHint::Antialiasing | QPainter::RenderHint::SmoothPixmapTransform);

    QFont font("Segoe UI");
    // QFont font;
    // font.setStyleStrategy(QFont::NoSubpixelAntialias |QFont::PreferMatch);
    // font.setStyleHint(QFont::SansSerif);
    font.setPixelSize(20);
    painter.setFont(font);

    painter.setPen(Qt::NoPen);
    // painter.setBrush(Qt::white);
    painter.setBrush(QColor(0x42, 0x42, 0x42));
    painter.drawRoundedRect(pixmap.rect(), 8, 8);

    // painter.setPen(Qt::black);
    painter.setPen(QColor(0xFF, 0xFF, 0xFF));
    painter.drawText(pixmap.rect().adjusted(60, 0, 0, 0), Qt::AlignCenter, info);

    // painter.drawImage(QRect(5, 5, 50, 50), cover);
    // painter.drawPixmap(QRect(5, 5, 50, 50), QPixmap::fromImage(cover));

    auto size = cover.size().scaled(QSize(50, 50), Qt::KeepAspectRatio);
    painter.drawImage(
        QRect(
            QPoint(
                (50 - size.width()) / 2 + 5,
                (50 - size.height()) / 2 + 5),
            size),
        cover);


    painter.end();
    PluginOperation(transport).draw(serial, uid, pixmap);
}
