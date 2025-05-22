#pragma once

#include <cstdint>
#include <tuple>

#include <QtCore/QTimer>
#include <QtCore/QDeadlineTimer>
#include <QtWebSockets/QWebSocket>

class QString;
class QJsonValue;


// For deducing class type
template<typename T>
struct member_function_traits;

template<typename ClassType, typename... Args>
struct member_function_traits<void (ClassType::*)(Args...)> {
    using class_type = ClassType;
};


class PluginTransport final : public QObject {
    Q_OBJECT

public:
    using HandlerFunc = void(*)(void *context, const QJsonValue &message, QJsonValue &result);
    using CallbackFunc = void(*)(void *context, bool success, const QJsonValue &result_or_error);

    explicit PluginTransport(uint16_t port, const QString &uuid);

    void start();

    void on(const QString &op, HandlerFunc handler, void *context);

    void off(const QString &op, HandlerFunc handler, void *context);

    // Usage: methodOn<&MyObject::myHandler>("op", myObjectInstance);
    template<auto Method>
    void methodOn(const QString &op, typename member_function_traits<decltype(Method)>::class_type *target) {
        on(op, methodHandlerWrapper<Method>, target);

        static_assert(
            std::is_base_of_v<QObject, typename member_function_traits<decltype(Method)>::class_type>,
            "`target` needs to inherit from `QObject` for automatic deregistration on destroy");
        QObject::connect(
            target, &QObject::destroyed,
            this, [this, target, op=QString(op)] {
                methodOff<Method>(op, target);
            }
        );
    }

    template<auto Method>
    void methodOff(const QString &op, typename member_function_traits<decltype(Method)>::class_type *target) {
        off(op, methodHandlerWrapper<Method>, target);
    }

    void call(const QLatin1String &op, QJsonValue &&message,
              CallbackFunc callback = nullptr, void *context = nullptr,
              int timeoutMs = 5000);

    // void test(); // TODO
    //
    // void testHandler(const QJsonValue &message, QJsonValue &result);

private:
    template<auto Method>
    static void methodHandlerWrapper(void *object, const QJsonValue &message, QJsonValue &result) {
        using Class = typename member_function_traits<decltype(Method)>::class_type;
        return (static_cast<Class *>(object)->*Method)(message, result);
    }

    Q_SLOT void onWebsocketConnected();

    Q_SLOT void onWebsocketDisconnected();

    Q_SLOT void onWebsocketError(QAbstractSocket::SocketError error);

    Q_SLOT void onWebsocketTextMessage(const QString &message);

    Q_SLOT void onTimeoutCheck();

    QString send(const QLatin1String &command, QJsonValue &&payload);

    void send_response(QJsonValue &&cmd_uuid, QJsonValue &&result);

    QWebSocket ws{QString(), QWebSocketProtocol::VersionLatest, this};
    QTimer checkTimeoutTimer{this};
    uint16_t port;
    QString uuid;

    // type -> (handler, context)
    std::unordered_multimap<QString, std::tuple<HandlerFunc, void *> > handlers;

    // uuid -> (callback, context, deadline)
    std::unordered_map<QString, std::tuple<CallbackFunc, void *, QDeadlineTimer> > callbacks;
};
