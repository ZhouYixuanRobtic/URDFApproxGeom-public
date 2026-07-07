# Implementation Request: synthetic-morphology-benchmark

request_id: synthetic-morphology-benchmark
source: gatekeeper-review
status: open
owner: implement

## Paper Claim
The PCA-section capsule pipeline is expected to work best on elongated tubular links and degrade on branched, flat, or near-spherical geometries. The paper needs a controlled stress test instead of broad generalization from FR3 alone.

## Task Type
experiment

## Required Outputs
- `doc/paper/artifacts/synthetic-morphology-benchmark/RESULT.md`
- `doc/paper/artifacts/synthetic-morphology-benchmark/data/synthetic_morphology_metrics.csv`
- Generated meshes and URDF wrappers for five shapes:
  - straight_tube
  - tapered_tube
  - elbow_tube
  - t_branch
  - flat_plate
- One qualitative figure comparing capsule overlays for the best and worst synthetic cases.

## Metrics
- shape_name
- mode
- preset
- primitive_count
- vertex_worst_signed_distance_m
- surface_worst_signed_distance_m
- surface_uncovered_fraction
- capV_aabb
- r_binMed
- runtime_s

## Inputs
- Existing package CLI/API in `python/urdf_approx_geom`
- Existing validation helpers in `python/urdf_approx_geom/validation.py`
- Trimesh or repository mesh utilities if available in the environment

## Acceptance Criteria
- Mesh generation scripts are saved under `doc/paper/artifacts/synthetic-morphology-benchmark/scripts/`.
- Every generated mesh and URDF wrapper is saved under the artifact directory.
- Metrics are reported for capsule default and capsule high_detail for all five shapes.
- RESULT.md states which geometry classes support the PCA-sectioning assumption and which violate it.
- Failed shapes are reported with failure logs and still included in the summary table.

## Writer Integration Target
- `doc/paper/sections/experiments.tex`
- `doc/paper/sections/discussion.tex`
- `doc/paper/sections/conclusion.tex`
