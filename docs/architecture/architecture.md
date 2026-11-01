# Architecture

## Layers and data flow

Win32 presentation and GDI viewport (src/frontend/windows)
                       |
              UI event handlers
                       |
 C++ core: geometry | thermal | performance | hardware | ML | export
                       |
       C++17 standard library / Windows system APIs

The Windows presentation selects input files and displays summaries. uew_core owns parsing and engineering calculations. A GDI child viewport statically implements orthographic display of STL triangles or temperature points, rotate/pan/zoom, fit, a temperature legend, and point inspection. Thermal workloads are independent; the batch runner assigns indices to bounded workers and stores results in input order. Export code writes temporary files and replaces destinations. Hardware integration uses interfaces with mock-only implementations.

The portable executable is a core demo, not a replacement for the Windows desktop UI. File imports execute synchronously. Analysis actions run on a worker thread and show a running message, but have no progress or cancellation. There is no separate controller class. UI rendering and behavior remain unverified until built and run on Windows.

## Modules

- geometry: ASCII STL and temperature CSV parsing plus shared types.
- thermal: physical parameters, assembly, fixed-temperature constraints, solve, exact values, midpoint verification.
- performance: deterministic case generation, serial/parallel execution, benchmark metrics.
- hardware: device abstraction and three simulated adapters.
- ml: one-sample manual forward/backpropagation, gradient checking, training.
- export: CSV outputs and labelled SVG plots for thermal, convergence, performance, and ML results.
- common: compiler/build/host information where available.
- frontend/windows: Win32 shell and native GDI viewport.