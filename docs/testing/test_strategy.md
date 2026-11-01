# Test strategy

tests/TestMain.cpp is a dependency-free C++ executable registered with CTest. Current source checks thermal prescribed end temperatures, midpoint interpolation against the exact solution, mesh convergence, invalid thermal inputs, deterministic generation and ordering, serial/parallel results and worker limits, initial neural-network values, simultaneous parameter updates, central finite-difference gradients, 100-step loss reduction, mock device discovery/recovery/telemetry, valid/empty/malformed/non-finite ASCII STL, valid/empty/missing-column/malformed/non-finite temperature CSV, and CSV/SVG export output.

uew_validation independently runs meshes N=10,20,40,80, serial/parallel comparison on fixed generated cases, five-repeat benchmarking up to four workers, gradient checks at epsilon 1e-6, 100 training steps, example imports, and CSV/SVG export.

## Execution status

The test and validation sources have not been compiled or executed. Status: NOT VERIFIED — TOOLCHAIN UNAVAILABLE. Do not report tests as passing until CMake build, CTest, and the validation executable have succeeded. The desktop UI also needs a manual Windows run for rendering, interaction, and responsiveness checks.