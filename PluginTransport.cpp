#include "PluginTransport.h"

#include <QtCore/QtDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QUuid>


PluginTransport::PluginTransport(uint16_t port, const QString &uuid)
    : port(port), uuid(uuid) {
    checkTimeoutTimer.setInterval(5000);
    checkTimeoutTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);

    connect(&ws, &QWebSocket::connected,
            this, &PluginTransport::onWebsocketConnected);
    connect(&ws, &QWebSocket::disconnected,
            this, &PluginTransport::onWebsocketDisconnected);
    connect(&ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &PluginTransport::onWebsocketError);
    connect(&ws, &QWebSocket::textMessageReceived,
            this, &PluginTransport::onWebsocketTextMessage);

    connect(&checkTimeoutTimer, &QTimer::timeout,
            this, &PluginTransport::onTimeoutCheck);
}

void PluginTransport::start() {
    ws.open(QUrl("ws://localhost:" + QString::number(port)));
}

void PluginTransport::on(const QString &op, HandlerFunc handler, void *context) {
    handlers.emplace(op, std::make_tuple(handler, context));
}

void PluginTransport::off(const QString &op, HandlerFunc handler, void *context) {
    auto range = handlers.equal_range(op);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == std::make_tuple(handler, context)) {
            handlers.erase(it);
            break;
        }
    }
}

void PluginTransport::call(
    const QLatin1String &op, QJsonValue &&message,
    CallbackFunc callback, void *context,
    int timeoutMs
) {
    QString msg_uuid = send(op, std::move(message));
    callbacks.insert_or_assign(
        msg_uuid,
        std::make_tuple(
            callback, context,
            QDeadlineTimer(timeoutMs ? timeoutMs : QDeadlineTimer::Forever,
                           Qt::TimerType::VeryCoarseTimer)));
    if (!checkTimeoutTimer.isActive()) {
        checkTimeoutTimer.start();
    }
}

void PluginTransport::onWebsocketConnected() {
    std::ignore = send(
        QLatin1String("startup"),
        QJsonObject{
            {QStringLiteral("pluginID"), uuid}
        }
    );
}

void PluginTransport::onWebsocketDisconnected() {
    QTimer::singleShot(5000, this, &PluginTransport::start);
}

void PluginTransport::onWebsocketError(QAbstractSocket::SocketError error) {
    qCritical() << "Websocket error:" << error;
    ws.close(QWebSocketProtocol::CloseCodeProtocolError);
}

void PluginTransport::onWebsocketTextMessage(const QString &message) {
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (doc.isNull()) {
        qCritical() << "Failed to parse incoming message" << message << " : " << error.errorString();
        return;
    }
    const QJsonObject cmd = doc.object();

    // If it's a response to a call()
    if (const auto it = callbacks.find(cmd.value(QLatin1String("uuid")).toString());
        it != callbacks.end()
    ) {
        CallbackFunc callback = std::get<0>(it->second);
        void *context = std::get<1>(it->second);
        callbacks.erase(it);
        if (callbacks.empty()) checkTimeoutTimer.stop();

        const bool isSuccess = cmd.value(QLatin1String("status")) == QLatin1String("success");
        if (callback != nullptr) {
            callback(context, isSuccess,
                     isSuccess
                         ? cmd.value(QLatin1String("payload"))
                         : cmd.value(QLatin1String("error"))
            );
        } else {
            if (!isSuccess)
                qWarning() << "Unhandled call failure:" << message;
        }
        return;
    }

    // If it's a broadcast or direct send
    const auto range = handlers.equal_range(cmd.value(QLatin1String("type")).toString());
    if (range.first != range.second) {
        const auto payload = cmd.value(QLatin1String("payload"));
        QJsonValue result(QJsonValue::Null);
        for (auto it = range.first; it != range.second; ++it) {
            auto [handler, context] = it->second;
            handler(context, payload, result);
        }
        send_response(cmd.value(QLatin1String("uuid")), std::move(result));
    } else {
#ifdef _DEBUG
        auto stream = qDebug();
        const auto payload = cmd.value(QLatin1String("payload"));
        QDebugStateSaver saver(stream);
        stream.noquote()
                << "Unhandled message" << cmd.value(QLatin1String("type")).toString() << "\n"
                << QString::fromUtf8(
                    (payload.isArray()
                         ? QJsonDocument(payload.toArray())
                         : QJsonDocument(payload.toObject())
                    ).toJson());
#endif
    }
}

void PluginTransport::onTimeoutCheck() {
    auto it = callbacks.begin();
    while (it != callbacks.end()) {
        const QDeadlineTimer &deadline = std::get<2>(it->second);
        if (deadline.hasExpired()) {
            CallbackFunc callback = std::get<0>(it->second);
            void *context = std::get<1>(it->second);
            if (callback != nullptr) {
                callback(context, false, QStringLiteral("Request timed out"));
            } else {
                qWarning() << "Unhandled request timeout";
            }

            it = callbacks.erase(it);
        } else {
            ++it;
        }
    }
    if (callbacks.empty()) checkTimeoutTimer.stop();
}

QString PluginTransport::send(const QLatin1String &command, QJsonValue &&payload) {
    QString msg_uuid = QUuid::createUuid().toString(QUuid::StringFormat::Id128);
    const QJsonDocument doc(QJsonObject{
        {QStringLiteral("pluginID"), uuid},
        {QStringLiteral("type"), command},
        {QStringLiteral("payload"), payload},
        {QStringLiteral("timestamp"), QDateTime::currentMSecsSinceEpoch()},
        {QStringLiteral("uuid"), msg_uuid},
    });
    ws.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    return msg_uuid;
}

void PluginTransport::send_response(QJsonValue &&cmd_uuid, QJsonValue &&result) {
    const QJsonDocument doc(QJsonObject{
        {QStringLiteral("pluginID"), uuid},
        {QStringLiteral("type"), QLatin1String("response")},
        {QStringLiteral("payload"), result},
        {QStringLiteral("timestamp"), QDateTime::currentMSecsSinceEpoch()},
        {QStringLiteral("uuid"), cmd_uuid},
        {QStringLiteral("status"), QLatin1String("success")},
    });
    ws.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}
