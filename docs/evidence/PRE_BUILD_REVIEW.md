# Pre-build static review

Date: 2026-11-01. Review method: source and project-file inspection only. No compiler, configure, CTest, validation executable, or desktop runtime was used. All observations are UNVERIFIED until a Windows x64 build and execution pass completes.

## Files inspected

Configuration: CMakeLists.txt, CMakePresets.json, .vscode/tasks.json, .vscode/launch.json. The Windows presets now target Visual Studio 18 2026 and require CMake 4.2 or newer.
Core: src/core/common/SystemInfo.h/.cpp; src/core/export/CsvExport.h/.cpp; src/core/geometry/GeometryTypes.h, StlParser.h/.cpp, TemperatureCsvParser.h/.cpp; src/core/hardware/MockDevices.h/.cpp; src/core/ml/Backprop.h/.cpp; src/core/performance/Benchmark.h/.cpp; src/core/thermal/ThermalSolver.h/.cpp.
Application and checks: src/main.cpp, src/validation_main.cpp, src/frontend/windows/MainWindow.cpp, src/frontend/windows/GeometryViewport.h/.cpp, tests/TestMain.cpp.
Documentation: README.md, docs/architecture/architecture.md, docs/testing/test_strategy.md, and existing evidence documents.

## Static implementation review

The source declares a C++17 core and Visual Studio 2022 x64 Debug/Release presets plus a portable Ninja preset. Thermal source assembles a 1D linear FE system, applies end temperatures, computes midpoint comparisons, and has validation code for N=10,20,40,80. Batch code provides deterministic input generation, serial and bounded parallel execution, five measured repetitions, and ordered outputs. Backpropagation and central-difference checks are implemented in source. Parser branches and test assertions cover listed malformed, empty, missing-column, NaN, and infinity cases. The Windows UI source contains five pages and a native GDI geometry/temperature viewport. CSV/SVG exporters and a validation executable are present.

These statements describe code paths, not verified behavior. No numerical value, export file, or test result has been confirmed by execution.

## Dependencies and likely integration checks

- CMake 4.2 or later for the Visual Studio 18 2026 generator.
- Visual Studio 2026 C++ workload with x64 compiler and a compatible Windows SDK for desktop presets.
- Ninja is needed only for the portable preset.
- C++17 standard library and thread support; Windows libraries include Advapi32, Comctl32, Comdlg32, User32, and Gdi32.
- No third-party numerical, plotting, or test framework is declared.

Windows SDK declarations and linkage, MSVC warnings, GDI viewport API usage, and CMake Visual Studio multi-configuration behavior have not been compiler-checked. No compilation result can be asserted.

## Required verification commands

Run from D:demo-project in a Visual Studio x64 developer environment:

cmake --preset windows-debug
cmake --build --preset windows-debug --clean-first
ctest --preset windows-debug
.\build\windows-debug\Debug\uew_validation.exe

Record actual output in BUILD_AND_TEST_STATUS.md; fix errors and rerun build and CTest before changing execution status to VERIFIED. Manually inspect build\windows-debug\Debug\UnifiedEngineeringWorkbench.exe.

## Manual desktop review after build

1. Launch the app and switch Geometry, Thermal Analysis, Performance, Hardware, and Backpropagation pages.
2. Import valid, empty, and malformed ASCII STL; inspect geometry and rotate, pan, zoom, and fit.
3. Import valid, empty, malformed, missing-column, NaN, and Infinity temperature CSV; inspect points and select a node.
4. Run thermal, performance, and training pages; confirm responsiveness and exports under results/.
5. Confirm mock-only hardware status and recovery/telemetry behavior.
6. Check exported CSV values and open SVG plots.

## Output expectations (not generated)

The validation executable is intended to write thermal mesh CSVs and plots under results/thermal, serial/parallel agreement and benchmark CSV/SVG/environment files under results/performance, and gradient/loss CSV/SVG files under results/ml. These paths are expectations based on source and have not been observed.
