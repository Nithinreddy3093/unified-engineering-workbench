#include "ThermalSolver.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace uew {
double ThermalSolver::exactTemperature(double x, const ThermalParameters& p) {
    if (!std::isfinite(x) || !std::isfinite(p.length) || p.length <= 0.0
        || !std::isfinite(p.conductivity) || p.conductivity <= 0.0
        || !std::isfinite(p.heatGeneration) || !std::isfinite(p.leftTemperature)
        || !std::isfinite(p.rightTemperature)) {
        throw std::invalid_argument("Exact thermal solution requires finite x, positive length and conductivity, and finite boundary/source values.");
    }
    const double value = p.leftTemperature
        + (p.rightTemperature - p.leftTemperature) * x / p.length
        + (p.heatGeneration / (2.0 * p.conductivity)) * x * (p.length - x);
    if (!std::isfinite(value)) {
        throw std::overflow_error("Exact thermal solution produced a non-finite temperature.");
    }
    return value;
}

ThermalSolution ThermalSolver::solve(const ThermalParameters& p, int elements) const {
    const double inputs[] = { p.length, p.area, p.conductivity, p.heatGeneration,
                              p.leftTemperature, p.rightTemperature };
    for (double value : inputs) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("Thermal solver validation failed: all parameters must be finite.");
        }
    }
    if (p.length <= 0.0) throw std::invalid_argument("Thermal solver validation failed: length must be positive.");
    if (p.area <= 0.0) throw std::invalid_argument("Thermal solver validation failed: area must be positive.");
    if (p.conductivity <= 0.0) throw std::invalid_argument("Thermal solver validation failed: conductivity k must be positive.");
    if (elements <= 0) throw std::invalid_argument("Thermal solver validation failed: number of elements must be positive.");

    const std::size_t nodeCount = static_cast<std::size_t>(elements) + 1;
    const double elementLength = p.length / static_cast<double>(elements);
    const double elementStiffness = p.conductivity * p.area / elementLength;
    const double elementLoad = p.heatGeneration * p.area * elementLength / 2.0;
    if (!std::isfinite(elementLength) || elementLength <= 0.0
        || !std::isfinite(elementStiffness) || elementStiffness <= 0.0
        || !std::isfinite(elementLoad)) {
        throw std::invalid_argument("Thermal solver validation failed: derived element values are outside the finite numeric range.");
    }

    std::vector<std::vector<double>> stiffness(
        nodeCount, std::vector<double>(nodeCount, 0.0));
    std::vector<double> load(nodeCount, 0.0);
    for (int e = 0; e < elements; ++e) {
        const auto i = static_cast<std::size_t>(e);
        stiffness[i][i] += elementStiffness;
        stiffness[i][i + 1] -= elementStiffness;
        stiffness[i + 1][i] -= elementStiffness;
        stiffness[i + 1][i + 1] += elementStiffness;
        load[i] += elementLoad;
        load[i + 1] += elementLoad;
    }

    const std::size_t fixedNodes[] = { 0, nodeCount - 1 };
    const double prescribedTemperatures[] = { p.leftTemperature, p.rightTemperature };
    for (int boundary = 0; boundary < 2; ++boundary) {
        const std::size_t fixed = fixedNodes[boundary];
        for (std::size_t row = 0; row < nodeCount; ++row) {
            if (row != fixed) {
                load[row] -= stiffness[row][fixed] * prescribedTemperatures[boundary];
                stiffness[row][fixed] = 0.0;
            }
        }
        std::fill(stiffness[fixed].begin(), stiffness[fixed].end(), 0.0);
        stiffness[fixed][fixed] = 1.0;
        load[fixed] = prescribedTemperatures[boundary];
    }

    // Partial pivoting solves the small assembled dense system robustly.
    for (std::size_t column = 0; column < nodeCount; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < nodeCount; ++row) {
            if (std::abs(stiffness[row][column]) > std::abs(stiffness[pivot][column])) {
                pivot = row;
            }
        }
        if (std::abs(stiffness[pivot][column]) < 1e-20) {
            throw std::runtime_error("Thermal solver failed: singular global stiffness matrix.");
        }
        std::swap(stiffness[pivot], stiffness[column]);
        std::swap(load[pivot], load[column]);

        const double diagonal = stiffness[column][column];
        for (std::size_t j = column; j < nodeCount; ++j) stiffness[column][j] /= diagonal;
        load[column] /= diagonal;
        for (std::size_t row = 0; row < nodeCount; ++row) {
            if (row == column) continue;
            const double factor = stiffness[row][column];
            if (factor == 0.0) continue;
            for (std::size_t j = column; j < nodeCount; ++j) {
                stiffness[row][j] -= factor * stiffness[column][j];
            }
            load[row] -= factor * load[column];
        }
    }

    ThermalSolution solution;
    solution.elements = elements;
    solution.nodalTemperature = std::move(load);
    for (double temperature : solution.nodalTemperature) {
        if (!std::isfinite(temperature)) {
            throw std::runtime_error("Thermal solver failed: computed nodal temperature is non-finite.");
        }
    }
    solution.nodeX.resize(nodeCount);
    for (std::size_t i = 0; i < nodeCount; ++i) {
        solution.nodeX[i] = elementLength * static_cast<double>(i);
    }

    solution.midpoints.reserve(static_cast<std::size_t>(elements));
    for (int e = 0; e < elements; ++e) {
        const auto i = static_cast<std::size_t>(e);
        const double x = (static_cast<double>(e) + 0.5) * elementLength;
        const double fe = 0.5 * (solution.nodalTemperature[i] + solution.nodalTemperature[i + 1]);
        const double exact = exactTemperature(x, p);
        const double error = std::abs(fe - exact);
        if (!std::isfinite(error)) {
            throw std::runtime_error("Thermal verification failed: midpoint error is non-finite.");
        }
        solution.midpoints.push_back({ x, fe, exact, error });
        solution.maxMidpointError = std::max(solution.maxMidpointError, error);
    }
    return solution;
}
} // namespace uew
