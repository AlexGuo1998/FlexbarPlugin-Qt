#pragma once

#include <string>
#include <vector>
#include <cstdint>

class QString;

int qt_main(std::vector<std::string>& args);
int qt_main(uint16_t port, QString uid, QString dir);
