#include "plugin_connection.h"


PluginConnection::PluginConnection(uint16_t port, const QString &uuid)
    :timer(this)
{
    TODO_IMPLEMENT_ME();
}

void PluginConnection::on(const QString &op, HandlerFunc handler, void *context) {
    TODO_IMPLEMENT_ME();
}

void PluginConnection::off(const QString &op, HandlerFunc handler, void *context) {
    TODO_IMPLEMENT_ME();
}

void PluginConnection::call(const QString &op, const QString &message, CallbackFunc callback, void *context,
    int timeout) {
    TODO_IMPLEMENT_ME();
}
