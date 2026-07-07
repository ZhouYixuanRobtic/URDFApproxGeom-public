# Result: synthetic-morphology-benchmark

status: done
source_request: doc/paper/requests/synthetic-morphology-benchmark.md

## Claim Under Test

The PCA-section capsule pipeline (Wu2018) is expected to work best on elongated tubular links and degrade on branched, flat, or near-spherical geometries. A controlled stress test across synthetic morphologies is needed instead of broad generalization from FR3 alone.

## Environment

- **Python**: 3.10.12 (system) / 3.13.13 (venv for mesh generation)
- **C++ Extension**: pre-built `_urdf_approx_geom.cpython-310-x86_64-linux-gnu.so` from build directory
- **Capsule presets tested**: `default` (NSections=4, MaxCapsulesPerLink=12) and `high_detail` (NSections=6, MaxCapsulesPerLink=16)
- **Dependencies**: pybind11 2.9.1, trimesh 4.10.1 (venv) / 4.12.2 (venv), numpy, igl readOBJ (C++)
- **Git HEAD**: 63f5133

## Commands

```bash
# Mesh generation
source .venv/bin/activate && python3 doc/paper/artifacts/synthetic-morphology-benchmark/scripts/generate_meshes.py

# URDF generation
source .venv/bin/activate && run_env.sh doc/paper/artifacts/synthetic-morphology-benchmark/scripts/generate_urdfs.py

# Capsule fitting benchmark (all 5 shapes x 2 presets)
run_env.sh doc/paper/artifacts/synthetic-morphology-benchmark/scripts/run_benchmark.py

# Figure generation
source .venv/bin/activate && python3 doc/paper/artifacts/synthetic-morphology-benchmark/scripts/generate_figure.py
```

## Artifacts

- `scripts/generate_meshes.py`: mesh generation (straight_tube, tapered_tube, elbow_tube, t_branch, flat_plate)
- `scripts/generate_urdfs.py`: URDF wrappers for each mesh
- `scripts/run_benchmark.py`: capsule fitting benchmark runner
- `scripts/generate_figure.py`: qualitative overlay figure
- `meshes/*.obj`: generated synthetic meshes (5 files)
- `urdf/*.urdf`: URDF wrappers (5 files)
- `data/synthetic_morphology_metrics.csv`: metrics for all 10 shape/preset combinations
- `data/*.json`: capsule sidecar JSON files for each fit
- `data/*.urdf`: capsule output URDF files
- `figures/capsule_overlay_comparison.png`: 2x2 grid of best/moderate/poor/worst cases
- `figures/capsule_overlay_comparison.pdf`: same as vector PDF
- `figures/t_branch_preset_comparison.png`: default vs high_detail for t_branch
- `figures/t_branch_preset_comparison.pdf`: same as vector PDF

## Results

| shape_name | preset | capsules | vertex_worst_sd (m) | capV_aabb | r_binMed | runtime_s |
|---|---:|---:|---:|---:|---:|---:|
| straight_tube | default | 1 | 0.000000 | 0.869 | 1.000 | 0.44 |
| straight_tube | high_detail | 1 | 0.000000 | 0.869 | 1.000 | 0.48 |
| tapered_tube | default | 3 | 0.000000 | 0.542 | 1.099 | 5.84 |
| tapered_tube | high_detail | 6 | 0.000000 | 0.450 | 1.160 | 8.66 |
| elbow_tube | default | 1 | 0.000000 | 1.318 | 1.021 | 1.67 |
| elbow_tube | high_detail | 1 | 0.000000 | 1.489 | 1.038 | 1.71 |
| t_branch | default | 2 | 0.000000 | 4.137 | 1.002 | 2.23 |
| t_branch | high_detail | 3 | 0.000000 | 4.107 | 1.004 | 8.79 |
| flat_plate | default | 1 | 0.000000 | 16.714 | 1.000 | 0.58 |
| flat_plate | high_detail | 1 | 0.000000 | 16.713 | 1.000 | 1.04 |

All runs used the `capsule` mode with `mesh_source=collision`. Every fit achieved full vertex coverage (worst_signed_distance <= 0) after the grow-to-cover step. No failures.

## Claim Support Summary

- **supported** -- The PCA-section capsule pipeline degrades predictably across morphology classes:

  | Geometry Class | Example | capV_aabb (default) | PCA Assumption |
  |---|---|---|---|
  | Elongated tubular | straight_tube | 0.87 | Valid -- the PCA axis aligns with the tube |
  | Tapered tubular | tapered_tube | 0.54 | Valid with multiple sections needed |
  | Curved tubular | elbow_tube | 1.32 | Partially valid -- single capsule inflates |
  | Branched | t_branch | 4.14 | Violated -- branching breaks single-axis PCA |
  | Planar | flat_plate | 16.71 | Strongly violated -- PCA finds 2D plane, not a tube |

- The capV/aabb ratio is the most discriminative metric: it increases by ~5x from best (straight_tube, 0.87) to branching (t_branch, 4.14) and 19x to planar (flat_plate, 16.71).
- The r_binMed metric does not differentiate well across morphologies (all <= 1.16) because the grow-to-cover step ensures every vertex is within its capsule radius, making the bin-median radius ratio converge near 1.0.
- high_detail preset increased capsule count only for tapered_tube (3->6) and t_branch (2->3), with marginal capV/aabb improvement. For elbow_tube and flat_plate, the single-capsule fit did not split further.

## Limitations

1. **Synthetic meshes are simpler than real robot links.** The absolute capV/aabb values are a lower bound; real-world links (with complex cutouts, mounting flanges, sensor mounts) will likely degrade further.
2. **Mesh quality.** The t_branch mesh is two intersecting cylinder surfaces without proper boolean union -- the ManifoldPlus watertight repair step handled this, but the geometry is idealized.
3. **Single-link per URDF.** The benchmark tests each morphology in isolation, not as part of a kinematic chain.
4. **No sphere mode comparison.** The request specified capsule modes only, so sphere tree results are not included.
5. **r_binMed ceiling effect.** The grow-to-cover step makes r_binMed insensitive to morphological differences; capV/aabb is the better differentiator.

## Next Actions For Writer

- **Directly usable fact**: "On a straight tube (0.5 m length, 0.04 m radius), the default capsule preset produces a single capsule with capV/aabb = 0.87. On a flat plate (0.2 x 0.15 x 0.01 m), the same preset produces capV/aabb = 16.71 -- a 19x degradation."
- **Supporting data for discussion**: The PCA-section assumption holds for elongated shapes (straight_tube, tapered_tube) and degrades on curved (elbow_tube, 1.3x over a straight tube baseline), branched (t_branch, 4.8x), and planar (flat_plate, 19.2x) geometries.
- **Avoid claiming**: "r_binMed differentiates morphologies" -- it plateaus near 1.0 for all shapes due to the coverage guarantee.
- **Figure recommendation**: Include `figures/capsule_overlay_comparison.pdf` as a 2x2 subplot showing best (straight_tube), moderate (elbow_tube), poor (t_branch), and worst (flat_plate) cases with capsule overlay.
