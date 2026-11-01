#include "Benchmark.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <exception>
#include <limits>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace uew {
std::vector<ThermalCase> generateCases(std::size_t count, int elements, std::uint32_t seed) {
    if (elements <= 0) throw std::invalid_argument("Batch configuration failed: mesh size must be positive.");
    if (count > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Batch configuration failed: case count exceeds supported case IDs.");
    }
    std::mt19937 generator(seed);
    std::uniform_real_distribution<double> conductivity(5.0, 25.0);
    std::uniform_real_distribution<double> generation(5000.0, 20000.0);
    std::vector<ThermalCase> cases;
    cases.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        ThermalCase item;
        item.caseId = static_cast<int>(i);
        item.parameters.conductivity = conductivity(generator);
        item.parameters.heatGeneration = generation(generator);
        item.elements = elements;
        cases.push_back(item);
    }
    return cases;
}

std::vector<ThermalSolution> runBatch(const std::vector<ThermalCase>& cases, unsigned workers) {
    const unsigned logicalLimit = std::max(1u, std::thread::hardware_concurrency());
    if (workers == 0) {
        throw std::invalid_argument("Benchmark configuration failed: worker count must be at least one.");
    }
    if (workers > logicalLimit) {
        throw std::invalid_argument("Benchmark configuration failed: requested "
            + std::to_string(workers) + " workers but only "
            + std::to_string(logicalLimit) + " logical CPUs are available.");
    }
    std::vector<ThermalSolution> output(cases.size());
    if (cases.empty()) return output;
    if (workers == 1) {
        ThermalSolver solver;
        for (std::size_t i = 0; i < cases.size(); ++i) {
            output[i] = solver.solve(cases[i].parameters, cases[i].elements);
        }
        return output;
    }

    const unsigned workerCount = std::min<unsigned>(
        workers, static_cast<unsigned>(cases.size()));
    std::atomic<std::size_t> nextCase{0};
    std::exception_ptr failure;
    std::mutex failureMutex;
    auto worker = [&] {
        ThermalSolver solver;
        for (;;) {
            const std::size_t index = nextCase.fetch_add(1, std::memory_order_relaxed);
            if (index >= cases.size()) return;
            try {
                output[index] = solver.solve(cases[index].parameters, cases[index].elements);
            } catch (...) {
                std::lock_guard<std::mutex> lock(failureMutex);
                if (!failure) failure = std::current_exception();
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(workerCount);
    try {
        for (unsigned i = 0; i < workerCount; ++i) threads.emplace_back(worker);
    } catch (...) {
        for (auto& thread : threads) if (thread.joinable()) thread.join();
        throw;
    }
    for (auto& thread : threads) thread.join();
    if (failure) std::rethrow_exception(failure);
    return output;
}

double maxBatchDifference(const std::vector<ThermalSolution>& left,
                          const std::vector<ThermalSolution>& right) {
    if (left.size() != right.size()) return std::numeric_limits<double>::infinity();
    double difference = 0.0;
    auto compare = [&](double a, double b) {
        difference = std::max(difference, std::abs(a - b));
    };
    for (std::size_t i = 0; i < left.size(); ++i) {
        const auto& a = left[i];
        const auto& b = right[i];
        if (a.elements != b.elements || a.nodeX.size() != b.nodeX.size()
            || a.nodalTemperature.size() != b.nodalTemperature.size()
            || a.midpoints.size() != b.midpoints.size()) {
            return std::numeric_limits<double>::infinity();
        }
        for (std::size_t j = 0; j < a.nodeX.size(); ++j) {
            compare(a.nodeX[j], b.nodeX[j]);
            compare(a.nodalTemperature[j], b.nodalTemperature[j]);
        }
        for (std::size_t j = 0; j < a.midpoints.size(); ++j) {
            compare(a.midpoints[j].x, b.midpoints[j].x);
            compare(a.midpoints[j].finiteElementTemperature, b.midpoints[j].finiteElementTemperature);
            compare(a.midpoints[j].exactTemperature, b.midpoints[j].exactTemperature);
            compare(a.midpoints[j].absoluteError, b.midpoints[j].absoluteError);
        }
        compare(a.maxMidpointError, b.maxMidpointError);
    }
    return difference;
}

std::vector<BenchmarkRow> benchmark(const std::vector<ThermalCase>& cases,
                                    unsigned logicalCpus, unsigned repetitions) {
    if (repetitions != 5) {
        throw std::invalid_argument("Benchmark requires exactly five measured repetitions.");
    }
    if (cases.empty()) throw std::invalid_argument("Benchmark requires at least one thermal case.");
    const unsigned detectedCpus = std::max(1u, std::thread::hardware_concurrency());
    const unsigned workerLimit = std::max(1u, std::min({4u, logicalCpus, detectedCpus}));
    std::vector<BenchmarkRow> rows;
    rows.reserve(workerLimit);
    for (unsigned workers = 1; workers <= workerLimit; ++workers) {
        (void)runBatch(cases, workers); // one warm-up excluded from the timing sample
        BenchmarkRow row;
        row.workers = workers;
        row.repetitionsMs.reserve(repetitions);
        for (unsigned sample = 0; sample < repetitions; ++sample) {
            const auto start = std::chrono::steady_clock::now();
            (void)runBatch(cases, workers);
            const auto end = std::chrono::steady_clock::now();
            row.repetitionsMs.push_back(
                std::chrono::duration<double, std::milli>(end - start).count());
        }
        auto sorted = row.repetitionsMs;
        std::sort(sorted.begin(), sorted.end());
        row.medianMs = sorted[sorted.size() / 2];
        rows.push_back(std::move(row));
    }
    const double baseline = rows.front().medianMs;
    for (auto& row : rows) {
        row.speedup = row.medianMs > 0.0
            ? baseline / row.medianMs : std::numeric_limits<double>::infinity();
        row.efficiency = row.speedup / static_cast<double>(row.workers);
    }
    return rows;
}
} // namespace uew
