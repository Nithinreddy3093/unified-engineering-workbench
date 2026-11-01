#include "core/common/SystemInfo.h"
#include "core/export/CsvExport.h"
#include "core/geometry/StlParser.h"
#include "core/geometry/TemperatureCsvParser.h"
#include "core/ml/Backprop.h"
#include "core/performance/Benchmark.h"
#include "core/thermal/ThermalSolver.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <thread>

namespace {
void reportError(const std::string& message) {
    std::cerr << message << '\n';
}
}

int main() {
    using namespace uew;
    try {
        std::cout << "Unified Engineering Workbench validation run\n";
        std::cout << systemInformation() << '\n';

        ThermalParameters parameters;
        ThermalSolver solver;
        std::cout << "Thermal FE midpoint verification\n";
        for (int elements : {10, 20, 40, 80}) {
            const auto solution = solver.solve(parameters, elements);
            if (std::abs(solution.nodalTemperature.front() - parameters.leftTemperature) > 1e-10
                || std::abs(solution.nodalTemperature.back() - parameters.rightTemperature) > 1e-10) {
                reportError("Thermal boundary-temperature verification failed.");
                return 1;
            }
            std::cout << "N=" << elements
                      << " max midpoint absolute error=" << solution.maxMidpointError << " C\n";
            exportThermal(solution, parameters,
                          "results/thermal/N_" + std::to_string(elements));
        }
        exportConvergence("results/thermal");

        const auto logicalCpus = std::max(1u, std::thread::hardware_concurrency());
        const auto cases = generateCases(80, 40, 20261031);
        const auto serial = runBatch(cases, 1);
        const auto parallelWorkers = std::min(4u, logicalCpus);
        const auto parallel = runBatch(cases, parallelWorkers);
        const double difference = maxBatchDifference(serial, parallel);
        exportAgreement(difference, "results/performance");
        const auto rows = benchmark(cases, logicalCpus, 5);
        exportBenchmark(rows, "results/performance");
        const std::string benchmarkEnvironment = systemInformation()
            + "Workload cases: 80\nMesh elements per case: 40\nRandom seed: 20261031\n"
            + "Warm-up: one run per worker count\nMeasured repetitions: 5\n";
        exportEnvironment(benchmarkEnvironment, "results/performance");
        std::cout << "Serial/parallel maximum result difference=" << difference << " C"
                  << (difference <= 1e-8 ? " PASS\n" : " FAIL\n");
        for (const auto& row : rows) {
            std::cout << "workers=" << row.workers
                      << " median_ms=" << row.medianMs
                      << " speedup=" << row.speedup
                      << " efficiency=" << row.efficiency << '\n';
        }

        Parameters initial;
        const auto firstPass = forward(initial);
        const auto check = checkGradients(initial, 1.0, 2.0, 1e-6, 1e-5);
        exportGradientCheck(check, "results/ml");
        for (const auto& item : check) {
            std::cout << "gradient " << item.parameter
                      << " analytical=" << item.analytical
                      << " numerical=" << item.numerical
                      << " absolute_error=" << item.absoluteError
                      << (item.pass ? " PASS\n" : " FAIL\n");
        }
        auto trained = initial;
        const auto losses = train(trained, 100, 0.1);
        exportTraining(losses, "results/ml");
        std::cout << "Initial loss=" << firstPass.loss
                  << " loss after 100 steps=" << losses.back().loss << '\n';

        const auto geometry = parseAsciiStl("data/geometry/example_geometry.stl");
        if (const auto error = std::get_if<ParseError>(&geometry)) {
            reportError(error->message);
            return 1;
        }
        const auto temperature = parseTemperatureCsv("data/thermal/example_temperature.csv");
        if (const auto error = std::get_if<ParseError>(&temperature)) {
            reportError(error->message);
            return 1;
        }
        std::cout << "Example ASCII STL and synthetic temperature CSV imports passed.\n";

        const bool gradientsPassed = std::all_of(
            check.begin(), check.end(), [](const GradientCheck& item) { return item.pass; });
        if (difference > 1e-8 || !gradientsPassed || losses.size() != 100
            || losses.back().loss >= firstPass.loss) {
            return 1;
        }
        std::cout << "Core validation completed. See results/ for CSV and SVG evidence.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
