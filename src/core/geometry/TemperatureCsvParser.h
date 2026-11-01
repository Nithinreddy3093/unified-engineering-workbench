#pragma once
#include "GeometryTypes.h"
#include <variant>
namespace uew {
using TemperatureCsvResult = std::variant<std::vector<TemperaturePoint>, ParseError>;
TemperatureCsvResult parseTemperatureCsv(const std::string& path, std::size_t maxBytes = 64u * 1024u * 1024u);
}
