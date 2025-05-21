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
    void (PluginTransport::*m_slot)(QString) = &PluginTransport::onWebsocketTextMessage;
    connect(&ws, &QWebSocket::textMessageReceived,
            this, m_slot);
    // TODO more connects
    // connect(&checkTimeoutTimer, &QTimer::timeout,
    //         this, &PluginTransport::onCheckTimeout);
}

void PluginTransport::start() {
    ws.open(QUrl("ws://localhost:" + QString::number(port)));

    checkTimeoutTimer.start(1000);
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

void PluginTransport::call(const QString &op, QJsonValue &&message, CallbackFunc callback, void *context,
                            int timeout) {
    QString msg_uuid = send(op, std::move(message));
    callbacks.insert_or_assign(
        msg_uuid,
        std::make_tuple(callback, context,
                        QDeadlineTimer(timeout ? timeout : QDeadlineTimer::Forever)));
    if (!checkTimeoutTimer.isActive()) {
        checkTimeoutTimer.start(timeout);
    }
}

void PluginTransport::test() {
    on_method<&PluginTransport::testHandler>("plugin.alive", this);
}

void PluginTransport::testHandler(const QJsonValue &message, QJsonValue &result) {
    auto stream = qDebug();
    QDebugStateSaver saver(stream);
    stream.noquote()
            << "internal alive handler"
            << QString::fromUtf8(
                (message.isArray()
                     ? QJsonDocument(message.toArray())
                     : QJsonDocument(message.toObject())
                ).toJson());
}

void PluginTransport::onWebsocketConnected() {
    std::ignore = send(
        QStringLiteral("startup"),
        QJsonObject{
            {QLatin1String("pluginID"), uuid}
        }
    );
}

void PluginTransport::onWebsocketTextMessage(QString message) {
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
        callbacks.erase(it);
        if (callbacks.empty()) checkTimeoutTimer.stop();

        auto [callback, context, deadline] = it->second;
        if (cmd.value(QLatin1String("status")) == QLatin1String("success")) {
            callback(context, true, cmd.value(QLatin1String("payload")));
        } else {
            callback(context, false, cmd.value(QLatin1String("error")));
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
#if _DEBUG
        auto stream = qDebug();
        const auto payload = cmd.value(QLatin1String("payload"));
        QDebugStateSaver saver(stream);
        stream.noquote()
                << "unhandled message" << cmd.value(QLatin1String("type")).toString() << "\n"
                << QString::fromUtf8(
                    (payload.isArray()
                         ? QJsonDocument(payload.toArray())
                         : QJsonDocument(payload.toObject())
                    ).toJson());
#endif
    }
}

QString PluginTransport::send(const QString &command, QJsonValue &&payload) {
    QString msg_uuid = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);
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
