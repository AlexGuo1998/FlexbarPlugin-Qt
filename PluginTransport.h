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


class PluginTransport : public QObject {
    Q_OBJECT

public:
    using HandlerFunc = void(*)(void *context, const QJsonValue &message, QJsonValue &result);

    explicit PluginTransport(uint16_t port, const QString &uuid);

    void start();

    void on(const QString &op, HandlerFunc handler, void *context);

    void off(const QString &op, HandlerFunc handler, void *context);

    // Generate the static handler function between on_method and off_method
    template<auto Method>
    static constexpr HandlerFunc get_handler_func() {
        using Class = typename member_function_traits<decltype(Method)>::class_type;
        const HandlerFunc handler = [](void *object, const QJsonValue &message, QJsonValue &result) {
            auto *target = static_cast<Class *>(object);
            return (target->*Method)(message, result);
        };
        return handler;
    }

    template<auto Method>
    void on_method(const QString &op, typename member_function_traits<decltype(Method)>::class_type *target) {
        static_assert(
            std::is_base_of_v<QObject, typename member_function_traits<decltype(Method)>::class_type>,
            "`target` needs to inherit from `QObject` for automatic deregistration");
        on(op, get_handler_func<Method>(), target);

        QObject::connect(
            target, &QObject::destroyed,
            this, [this, target, op=QString(op)] {
                off_method<Method>(op, target);
            }
        );
    }

    template<auto Method>
    void off_method(const QString &op, typename member_function_traits<decltype(Method)>::class_type *target) {
        off(op, get_handler_func<Method>(), target);
    }

    using CallbackFunc = void(*)(void *context, bool success, const QJsonValue &result_or_error);

    void call(const QString &op, QJsonValue &&message, CallbackFunc callback, void *context, int timeout = 5000);

    void test(); // TODO

    void testHandler(const QJsonValue &message, QJsonValue &result);

private:
    Q_SLOT void onWebsocketConnected();

    Q_SLOT void onWebsocketTextMessage(QString message);

    QString send(const QString &command, QJsonValue &&payload);

    void send_response(QJsonValue &&cmd_uuid, QJsonValue &&result);

    // type -> (handler, context)
    std::unordered_multimap<QString, std::tuple<HandlerFunc, void *> > handlers;

    // uuid -> (callback, context, deadline)
    std::unordered_map<QString, std::tuple<CallbackFunc, void *, QDeadlineTimer> > callbacks;

    QWebSocket ws{QString(), QWebSocketProtocol::VersionLatest, this};
    QTimer checkTimeoutTimer{this};
    uint16_t port;
    QString uuid;
};
