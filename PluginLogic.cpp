#include "PluginLogic.h"

#include "PluginTransport.h"

PluginLogic::PluginLogic(PluginTransport *transport) : transport(transport) {
    // transport->on_method<&PluginLogic::onxxx>(QStringLiteral("onxxx"), this);
}
