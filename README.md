# Unified Engineering Workbench

A C++17 prototype for thermal finite elements, engineering data import, CPU batch benchmarks, simulated embedded interfaces, and handwritten backpropagation. The numerical, parsing, concurrency, and export code is C++; there is no JavaScript numerical backend.

## Current status

This is a statically implemented prototype. **No C++ source has been compiled and no CTest test has been executed.** The Windows UI uses native Win32/GDI and includes an orthographic 3D viewport for STL triangles and temperature nodes, mouse rotate/pan/zoom, fit controls, a temperature legend, and point inspection. Rendering, desktop interaction, and all numerical results remain unverified until a Windows C++ build is available.

## Features

- C++17 modular core and CMake build.
- ASCII STL and temperature CSV parsers with size limits and finite-number validation.
- 1D two-node linear thermal FE model, boundary-condition elimination, pivoted dense solve, analytical reference, and element-midpoint errors for N=10,20,40,80.
- Deterministic thermal batch generator, bounded worker threads, five-repetition benchmark, median/speedup/efficiency, and nodal serial/parallel comparison.
- MCU, FPGA, and JTAG mock adapters, including a recoverable simulated FPGA timeout.
- Manual one-sample tanh-network forward/backpropagation, simultaneous gradient updates, central-difference gradient checks, and 100-step loss export.
- CSV export writes a temporary file before replacing its destination.
- `uew_validation` runs thermal meshes, serial/parallel comparison, benchmark repetitions, backpropagation checks/training, and example imports, then writes CSV/SVG evidence.
- Dependency-free C++ test executable.

## Architecture

`src/core` contains parsing, thermal, performance, hardware, ML, system information, and export modules. `src/frontend/windows` contains the Win32 UI. `src/main.cpp` is a portable core demo outside Windows. The UI invokes application operations but does not implement numerical algorithms.

## Formats and example data

- Geometry: ASCII STL only; maximum accepted file size defaults to 64 MiB.
- Temperature: CSV header must be exactly `node_id,x_m,y_m,z_m,temperature_C`; maximum accepted size defaults to 64 MiB.
- `data/thermal/example_temperature.csv` is synthetic and is not an ANSYS export.
- The example STL is self-created; see `data/provenance/geometry_provenance.md`.

## Build prerequisites

- CMake 4.2 or newer (the Windows presets use the Visual Studio 18 2026 generator; presets format version 3).
- Visual Studio 2026 C++ tools and Windows SDK for the Windows x64 presets.
- Ninja for the portable-debug preset.
- Windows SDK for the native Windows desktop target.
- VS Code users: Microsoft C/C++ extension for debugging with `cppvsdbg`; CMake Tools is optional because the included tasks invoke CMake directly.

Configure/build/test with a developer shell:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --clean-first
ctest --preset windows-debug
.\build\windows-debug\Debug\uew_validation.exe
```

The release configuration is `windows-release`; build it with `cmake --build --preset windows-release --clean-first` and test it with `ctest --preset windows-release`. The Visual Studio presets explicitly select x64. `portable-debug` is a non-UI Ninja build for the core demo and is not the Windows desktop target.

## Run

Run `build/windows-debug/Debug/Launch.exe` on Windows for the desktop shell. Run `build/windows-debug/Debug/uew_validation.exe` from the project root to create reproducible validation outputs. Results use relative `results/` paths.

The Windows desktop target is named `Launch.exe`. CPack ZIP install rules include the application, example data, project documentation, and required MSVC runtime libraries. Build the Release package with `cmake --build --preset windows-release --target package`. The resulting archive and clean-machine launch still require execution and verification; the VS Code/Visual Studio UI has also not been manually accepted.

## Thermal method

Steady 1D conduction uses `-d/dx(kA dT/dx)=qA`, constant properties, insulated sides, and fixed end temperatures 100 °C and 20 °C. The assembled system applies prescribed values by eliminating fixed columns and adjusting the load vector. Verification samples linear FE interpolation at each element midpoint and compares to `T=100-80x/L+[q/(2k)]x(L-x)`.

## Performance method

Thermal cases use fixed seed `20261031`; identical ordered cases feed serial and parallel runs. Benchmark warm-up occurs once per worker count; measured compute sections are repeated five times and summarized by median. Worker counts are capped at four and the reported logical CPU count. No speedup is promised. Results depend on machine load and build optimization. See `docs/performance/benchmark_method.md`.

## Embedded design

All devices and telemetry are software mocks. Device interfaces and wiring/power assumptions are illustrative and device-specific. No hardware is scanned or driven. See `docs/embedded/embedded_design.md`.

## Dependencies and licenses

The project uses only the C++ standard library and Windows system libraries for the desktop shell. No external test framework or plotting library is bundled. See [LICENSES.md](LICENSES.md).

## AI assistance disclosure

An AI assistant drafted project files in response to the user's request. The validation status and limitations are recorded plainly here; the developer should review, build, test, and complete `docs/evidence/AI_USE_LOG.md` before submission.

## Known limitations

- No build or test result is available; the current Codex execution environment cannot access CMake, MSVC, Ninja, MSBuild, or the Windows SDK.
- The UI has not been launched or visually inspected. GDI rendering behavior, camera controls, fit behavior, and point selection are statically implemented but unverified.
- Core exporters create CSV evidence and labelled SVG plots; there are no in-app plot widgets.
- Analysis actions run on a worker thread to keep the UI responsive but do not provide progress/cancellation. File import is synchronous.
- System reporting queries Windows OS build, CPU model, core counts, and RAM when available; non-Windows CPU model, physical cores, and RAM are reported unavailable.
- Packaging, Windows 11 clean-machine launch, and evidence screenshots are incomplete.
- The desktop UI is Windows-specific; the core is portable C++17.
