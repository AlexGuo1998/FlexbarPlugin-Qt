#pragma once

#include <cstdint>
#include <tuple>

#include <QtCore/QTimer>
#include <QtCore/QDeadlineTimer>
#include <QtWebSockets/QWebSocket>

class QString;


class PluginConnection : public QObject {
    Q_OBJECT

public:
    using HandlerFunc = void(*)(void *context, const QString &message, QString &result);
    using CallbackFunc = void(*)(void *context, const QString &result);

    explicit PluginConnection(uint16_t port, const QString &uuid);

    void on(const QString &op, HandlerFunc handler, void *context);

    void off(const QString &op, HandlerFunc handler, void *context);

    void call(const QString &op, const QString &message, CallbackFunc callback, void *context, int timeout = 5000);

private:
    std::unordered_multimap<QString, std::tuple<HandlerFunc, void*>> handlers;

    std::unordered_multimap<QString, std::tuple<CallbackFunc, void*, QDeadlineTimer>> callbacks;

    QWebSocket ws;
    QTimer timer;
};
