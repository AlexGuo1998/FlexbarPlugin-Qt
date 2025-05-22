#pragma once
#include <QtCore/QObject>

#include "PluginOperation.h"

class PluginTransport;

class PluginLogic : public QObject {
    Q_OBJECT

public:
    explicit PluginLogic(PluginTransport *transport);

private:
    void onDeviceStatus(const QJsonValue &message, QJsonValue &result);

    void onPluginAlive(const QJsonValue &message, QJsonValue &result);

    PluginTransport *transport;
};
