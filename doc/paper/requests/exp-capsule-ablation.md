# Implementation Request: exp-capsule-ablation

request_id: exp-capsule-ablation
source: writer
status: open
owner: implement

## Paper Claim
Capsule fitting parameters (NSections, MaxCapsulesPerLink, AdaptiveCircleCount, MaxRadiusBinRatio) control the trade-off between approximation tightness and primitive count.

## Task Type
experiment

## Required Outputs
- CSV or JSON table with per-link results for each ablation variant on FR3 arm links.
- The ablation variants should cover at least:
  - NSections = {2, 4, 6, 8} (with all other parameters at `default` preset)
  - MaxCapsulesPerLink = {1, 4, 8, 12, 16} (with all other parameters at `default` preset, NSections=4)
  - AdaptiveCircleCount = true vs false (with NSections=4, MaxCirclesPerSection=3)
  - MaxRadiusBinRatio = 1.45 (enabled) vs -1.0 (disabled, no splitting) (with all other parameters at `default` preset)
  - Mesh source = visual vs collision (with `default` preset)

## Metrics
For each (parameter_variant, link) combination, report:
- primitive count per link and total
- all_covered (boolean)
- capV/aabb (float)
- r/binMed (float)

## Inputs
- Repository root: /home/admin1/projects/SpherizedURDFGenerator
- Input URDF: resources/fr3/urdf/fr3.urdf
- Python API: `from urdf_approx_geom import generate, generate_capsule_multi`
- For custom configs, create temporary YAML files overriding individual parameters from the default config at `config/capsule/default.yml`.

## Acceptance Criteria
- Results exist for all ablation variants listed above.
- A clear winner or trend is visible for each parameter (e.g., increasing NSections improves tightness up to a point).
- Data is recorded in `doc/paper/artifacts/exp-capsule-ablation/RESULT.md`.

## Writer Integration Target
- Section VI (Experiments), Table 2 (capsule ablation), Figure 4 (tightness vs primitive count plot).
