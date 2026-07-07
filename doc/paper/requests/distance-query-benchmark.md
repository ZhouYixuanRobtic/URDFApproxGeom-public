# Implementation Request: distance-query-benchmark

request_id: distance-query-benchmark
source: gatekeeper-review
status: open
owner: implement

## Paper Claim
Analytic sphere and capsule outputs are useful for downstream distance-query workloads. The paper needs timing evidence rather than only asserting constant-time primitive distance formulas.

## Task Type
experiment

## Required Outputs
- `doc/paper/artifacts/distance-query-benchmark/RESULT.md`
- `doc/paper/artifacts/distance-query-benchmark/data/distance_query_timing.csv`
- Benchmark script under `doc/paper/artifacts/distance-query-benchmark/scripts/`

## Metrics
- mode
- preset
- link
- primitive_count
- query_count
- total_time_ms
- ns_per_point_query
- backend
- random_seed

## Inputs
- `doc/paper/artifacts/exp-fr3-mode-comparison/work/*.json`
- `doc/paper/artifacts/exp-fr3-mode-comparison/work/*.urdf`
- `resources/fr3/urdf/fr3.urdf`

## Acceptance Criteria
- Use a deterministic set of at least 100,000 query points sampled around each link AABB.
- Benchmark analytic point-to-sphere and point-to-capsule signed-distance evaluation from the sidecar JSON.
- Include a mesh-distance baseline only if a reliable local backend is available; otherwise mark mesh baseline as `blocked` and still report analytic primitive timing.
- Report warm-up policy, number of repeats, median, and p95 timing.
- RESULT.md clearly states that primitive query timing does not by itself prove simulator-level collision-checking speedup.

## Writer Integration Target
- `doc/paper/sections/experiments.tex`
- `doc/paper/sections/discussion.tex`
