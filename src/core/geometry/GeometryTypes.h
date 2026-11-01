#pragma once
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace uew {
struct Vec3 { double x{}, y{}, z{}; };
struct Triangle { Vec3 normal; std::array<Vec3, 3> vertices; };
struct Geometry { std::string name; std::vector<Triangle> triangles; };
struct TemperaturePoint { long long nodeId{}; Vec3 position; double temperatureC{}; };
struct ParseError { std::string message; std::size_t line{}; };
}
