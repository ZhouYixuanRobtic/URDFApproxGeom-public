# Capsule Ablation Summary

Each row varies one parameter from the default config (NSections=4, MaxCapsulesPerLink=12, AdaptiveCircleCount=false, MaxRadiusBinRatio=1.45, mesh_source=visual).

| Variant | Changed Param | Primitives | All Covered | Worst Dist | capV/aabb | r/binMed | Runtime (s) |
|---|---|---:|---|---:|---:|---:|---:|
| nsections_2 | NSections=2 | 8 | no | 0.006689 | 1.3684 | 1.2684 | 13.8 |
| nsections_4 | (default) | 10 | no | 0.010226 | 1.4700 | 1.4966 | 33.0 |
| nsections_6 | NSections=6 | 10 | no | 0.002338 | 1.3245 | 1.3225 | 21.0 |
| nsections_8 | NSections=8 | 11 | no | 0.001201 | 1.5558 | 1.4597 | 21.2 |
| mcpl_1 | MaxCapsulesPerLink=1 | 8 | no | 0.010214 | 1.3344 | 1.3080 | 14.7 |
| mcpl_4 | MaxCapsulesPerLink=4 | 10 | no | 0.010226 | 1.4700 | 1.4966 | 32.6 |
| mcpl_8 | MaxCapsulesPerLink=8 | 10 | no | 0.010226 | 1.4700 | 1.4966 | 32.4 |
| mcpl_16 | MaxCapsulesPerLink=16 | 10 | no | 0.010226 | 1.4700 | 1.4966 | 32.5 |
| adaptive_true | AdaptiveCircleCount=true | 39 | no | 0.001346 | 1.4071 | 2.0462 | 56.8 |
| mrbr_disabled | MaxRadiusBinRatio=-1.0 | 10 | no | 0.010226 | 1.4700 | 1.4966 | 12.6 |
| mesh_collision | mesh=collision | 10 | no | 0.010226 | 1.4700 | 1.4966 | 32.5 |
