# Implementation Request: exp-fr3-mode-comparison

request_id: exp-fr3-mode-comparison
source: writer
status: open
owner: implement

## Paper Claim
Convex, sphere, and capsule approximation modes produce different trade-offs between primitive count and geometric tightness on the FR3 robot arm.

## Task Type
experiment

## Required Outputs
- CSV or JSON table with per-link and aggregate results for each mode/preset combination on FR3 arm links (link0--link7).
- The table should cover: convex/default, sphere/single, sphere/default, capsule/single, capsule/default, capsule/high_detail.
- Qualitative screenshots or render paths showing at least one representative link (e.g., link0 or link3) with visual mesh wireframe overlaid with each approximation mode.
- Pipeline runtime for each mode.

## Metrics
For each (mode, preset, link) combination, report:
- primitive count (total and per-link)
- all_covered (boolean)
- worst_signed_distance (float)
- capV/aabb (float) — for capsule mode only
- r/binMed (float) — for capsule mode only
- pipeline runtime in seconds (wall-clock)

## Inputs
- Repository root: /home/admin1/projects/SpherizedURDFGenerator
- Input URDF: resources/fr3/urdf/fr3.urdf
- CLI tool: `urdf-approx-geom generate --mode <mode> -i <input> -o <output> --preset <preset>`
- Validation: `urdf-approx-geom validate --mode capsule --json <sidecar>`
- Use Python API for structured access: `from urdf_approx_geom import generate, generate_all`

## Acceptance Criteria
- Data exists for all 7 FR3 arm links (link0--link7) across all 6 mode/preset combinations listed above.
- Aggregate table summarizes total primitive count, min/max coverage, worst capV/aabb, worst r/binMed, and runtime.
- Screenshots or render paths exist in `doc/paper/artifacts/exp-fr3-mode-comparison/`.
- Data is recorded in `doc/paper/artifacts/exp-fr3-mode-comparison/RESULT.md`.

## Writer Integration Target
- Section VI (Experiments), Table 1 (mode comparison), Figure 2 (qualitative comparison), Figure 4 (trade-off plot).
