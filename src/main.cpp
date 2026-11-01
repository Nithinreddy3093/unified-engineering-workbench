#include "core/common/SystemInfo.h"
#include "core/export/CsvExport.h"
#include "core/geometry/StlParser.h"
#include "core/geometry/TemperatureCsvParser.h"
#include "core/hardware/MockDevices.h"
#include "core/ml/Backprop.h"
#include "core/performance/Benchmark.h"
#include <iostream>
#ifndef UEW_DESKTOP_APP
int main(){
    using namespace uew;
    try {
        ThermalParameters p; ThermalSolver solver;
        std::cout << "Unified Engineering Workbench (portable core demo)\n" << systemInformation();
        auto convergence=solver.solve(p,80); std::cout << "Thermal solve: N=80, maximum midpoint error=" << convergence.maxMidpointError << " C\n";
        auto checks=checkGradients(Parameters{}); for(auto& c:checks)std::cout<<"Gradient "<<c.parameter<<": "<<(c.pass?"PASS":"FAIL")<<" abs error="<<c.absoluteError<<"\n";
        std::cout << "Desktop interface is available on Windows builds.\n";
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;} return 0;
}
#endif
