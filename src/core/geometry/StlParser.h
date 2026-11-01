#pragma once
#include "GeometryTypes.h"
#include <string>
#include <variant>

namespace uew {
using GeometryResult = std::variant<Geometry, ParseError>;
GeometryResult parseAsciiStl(const std::string& path, std::size_t maxBytes = 64u * 1024u * 1024u);
}
