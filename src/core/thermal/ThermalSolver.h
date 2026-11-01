#pragma once
#include <string>
#include <vector>
namespace uew {
struct ThermalParameters {
    double length{1.0};
    double area{1e-4};
    double conductivity{10.0};
    double heatGeneration{10000.0};
    double leftTemperature{100.0};
    double rightTemperature{20.0};
};

struct MidpointResult {
    double x{};
    double finiteElementTemperature{};
    double exactTemperature{};
    double absoluteError{};
};

struct ThermalSolution {
    int elements{};
    std::vector<double> nodeX;
    std::vector<double> nodalTemperature;
    std::vector<MidpointResult> midpoints;
    double maxMidpointError{};
};

class ThermalSolver {
public:
    ThermalSolution solve(const ThermalParameters& parameters, int elements) const;
    static double exactTemperature(double x, const ThermalParameters& parameters);
};

struct ThermalCase {
    int caseId{};
    ThermalParameters parameters;
    int elements{};
};
}
