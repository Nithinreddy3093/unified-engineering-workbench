#pragma once
#include "../ml/Backprop.h"
#include "../performance/Benchmark.h"
#include "../thermal/ThermalSolver.h"
#include <string>
namespace uew {
void exportThermal(const ThermalSolution&,const ThermalParameters&,const std::string& directory);
void exportConvergence(const std::string& directory);
void exportBenchmark(const std::vector<BenchmarkRow>&,const std::string& directory);
void exportTraining(const std::vector<TrainingPoint>&,const std::string& directory);
void exportGradientCheck(const std::vector<GradientCheck>&,const std::string& directory);
void exportAgreement(double maximumDifference,const std::string& directory);
void exportThermalPlot(const ThermalSolution&,const std::string& directory);
void exportConvergencePlot(const std::string& directory);
void exportBenchmarkPlots(const std::vector<BenchmarkRow>&,const std::string& directory);
void exportTrainingPlot(const std::vector<TrainingPoint>&,const std::string& directory);
void exportEnvironment(const std::string& information,const std::string& directory);
}
