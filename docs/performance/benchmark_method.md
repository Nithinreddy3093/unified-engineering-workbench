# Performance method

Cases are generated with `std::mt19937` seed `20261031`, positive conductivity uniformly sampled from [5,25] W/(m K), and positive generation from [5000,20000] W/m³. The same ordered case vector is passed to serial and parallel runs. Parallel workers claim indices atomically; outputs retain input order.

The benchmark performs one warm-up and five measured repetitions for each available worker count from 1 through min(4, logical CPU count). Only `runBatch` execution is timed; generation and CSV output are excluded. The median of five timings is reported. `speedup=T1/Tp` and `efficiency=speedup/p`. Parallel agreement compares corresponding nodal temperatures and reports the maximum absolute difference.

Timing data is machine-, load-, compiler-, and optimization-dependent. The implementation does not claim speedup. Windows system reporting queries the OS build, CPU model, physical and logical core counts, RAM, compiler, and build type. Exact compiler optimization flags are not collected. Non-Windows CPU model, physical cores, and RAM are reported unavailable. Timings have not yet been run in this workspace.
