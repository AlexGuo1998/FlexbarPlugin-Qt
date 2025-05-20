#pragma once

#include <cstdint>
#include <tuple>

#include <QtCore/QTimer>
#include <QtCore/QDeadlineTimer>
#include <QtWebSockets/QWebSocket>

class QString;
class QJsonValue;


class PluginConnection : public QObject {
    Q_OBJECT

public:
    using HandlerFunc = void(*)(void *context, const QJsonValue &message, QJsonValue &result);
    using CallbackFunc = void(*)(void *context, bool success, const QJsonValue &result_or_error);

    explicit PluginConnection(uint16_t port, const QString &uuid);

    void start();

    void on(const QString &op, HandlerFunc handler, void *context);

    void off(const QString &op, HandlerFunc handler, void *context);

    void call(const QString &op, QJsonValue &&message, CallbackFunc callback, void *context, int timeout = 5000);

private:
    Q_SLOT void onWebsocketConnected();

    Q_SLOT void onWebsocketTextMessage(QString message);

    QString send(const QString &command, QJsonValue &&payload);

    void send_response(QJsonValue &&cmd_uuid, QJsonValue &&result);

    // type -> (handler, context)
    std::unordered_multimap<QString, std::tuple<HandlerFunc, void *>> handlers;

    // uuid -> (callback, context, deadline)
    std::unordered_map<QString, std::tuple<CallbackFunc, void *, QDeadlineTimer>> callbacks;

    QWebSocket ws{QString(), QWebSocketProtocol::VersionLatest, this};
    QTimer checkTimeoutTimer{this};
    uint16_t port;
    QString uuid;
};
