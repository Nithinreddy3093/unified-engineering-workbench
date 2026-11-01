# Thermal method

The solver models steady 1D conduction in a uniform rod with constant `k`, cross-sectional area `A`, uniform volumetric generation `q`, insulated sides, and prescribed end temperatures. The equation is `-d/dx(kA dT/dx)=qA`. Defaults: `L=1 m`, `A=1e-4 m²`, `k=10 W/(m K)`, `q=10000 W/m³`, `T(0)=100 °C`, `T(L)=20 °C`.

Each two-node linear element contributes `(kA/Le)[[1,-1],[-1,1]]` and `(qA Le/2)[1,1]`. Element terms are assembled into dense global `K` and `F`. For each prescribed temperature, the solver subtracts its column contribution from the remaining load entries, zeros the constrained column and row, then sets a unit diagonal and prescribed load value. Partial-pivot Gaussian elimination solves the small system.

The analytic function is `T(x)=T_left+(T_right-T_left)x/L+[q/(2k)]x(L-x)`; for the assigned temperatures its first terms are `100-80x/L`. Element midpoint FE values are the average of the adjacent nodal values. Absolute errors and their maximum are computed only at these midpoints. The expected midpoint error scales with element length squared. Required meshes are N=10,20,40,80.

Debugging note: imposing a fixed nodal value by overwriting only its equation leaves the interior equations with an incorrect right-hand side. Column elimination and load adjustment preserve the prescribed-temperature contribution.

CSV files are exported under `results/thermal`. CSV data and a labelled SVG temperature comparison are exported.
