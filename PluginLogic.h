#pragma once
#include <QObject>

class PluginTransport;

class PluginLogic : public QObject {
    Q_OBJECT

public:
    explicit PluginLogic(PluginTransport *transport);

private:
    void onConnect(PluginTransport *transport);
    PluginTransport *transport;
};
