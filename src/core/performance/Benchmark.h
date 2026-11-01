#pragma once
#include "../thermal/ThermalSolver.h"
#include <cstdint>
#include <vector>
namespace uew {
std::vector<ThermalCase> generateCases(std::size_t count,int elements,std::uint32_t seed=20261031);
std::vector<ThermalSolution> runBatch(const std::vector<ThermalCase>&,unsigned workers);
double maxBatchDifference(const std::vector<ThermalSolution>&,const std::vector<ThermalSolution>&);
struct BenchmarkRow { unsigned workers{}; std::vector<double> repetitionsMs; double medianMs{},speedup{},efficiency{}; };
std::vector<BenchmarkRow> benchmark(const std::vector<ThermalCase>&,unsigned logicalCpus,unsigned repetitions=5);
}
