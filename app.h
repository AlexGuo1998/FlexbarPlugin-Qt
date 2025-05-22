#pragma once

#include <string>
#include <vector>
#include <cstdint>

class QString;

int qt_main(std::vector<std::string>& args);
int qt_main(uint16_t port, const QString& uid, const QString &dir);
