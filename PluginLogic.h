#pragma once
#include <QtCore/QObject>

#include "PluginOperation.h"

class PluginTransport;

class PluginLogic : public QObject {
    Q_OBJECT

public:
    explicit PluginLogic(PluginTransport *transport);

    Q_SLOT void onMediaInfoUpdate(QString info, QImage cover);

private:
    void onDeviceStatus(const QJsonValue &message, QJsonValue &result);

    void onPluginAlive(const QJsonValue &message, QJsonValue &result);


    PluginTransport *transport;

    // device, cid, width
    std::tuple<QString, int, int> device;
};
