# Result: exp-fr3-mode-comparison

status: done
source_request: doc/paper/requests/exp-fr3-mode-comparison.md

## Claim Under Test
Convex, sphere, and capsule approximation modes produce different trade-offs between primitive count and geometric tightness on the FR3 robot arm.

## Commands
All commands run from repository root (`/home/admin1/projects/SpherizedURDFGenerator`) against `resources/fr3/urdf/fr3.urdf`.

```bash
# Generate URDF for each mode/preset combination with runtime instrumentation
urdf-approx-geom generate --mode convex  -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_convex_default.urdf  --preset default
urdf-approx-geom generate --mode sphere  -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sphere_single.urdf   --preset single
urdf-approx-geom generate --mode sphere  -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sphere_default.urdf  --preset default
urdf-approx-geom generate --mode capsule -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_capsule_single.urdf  --preset single
urdf-approx-geom generate --mode capsule -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_capsule_default.urdf --preset default
urdf-approx-geom generate --mode capsule -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_capsule_high_detail.urdf --preset high_detail

# Validate capsule outputs for tightness metrics
urdf-approx-geom validate --mode capsule --json /tmp/fr3_capsule_single.json
urdf-approx-geom validate --mode capsule --json /tmp/fr3_capsule_default.json
urdf-approx-geom validate --mode capsule --json /tmp/fr3_capsule_high_detail.json

# Validate sphere outputs
urdf-approx-geom validate --mode sphere --json /tmp/fr3_sphere_single.json
urdf-approx-geom validate --mode sphere --json /tmp/fr3_sphere_default.json
```

## Artifacts
| Artifact | Description |
|---|---|
| `data/fr3_aggregate_summary.csv` | Aggregate summary by preset (7 columns, 6 data rows) |
| `data/fr3_comparison_table.md` | Human-readable markdown version of aggregate results |
| `figures/fr3_link0_*.png` | (intended) Qualitative overlay screenshots -- **not yet generated** |

**Missing artifacts (not yet generated):**
- `data/fr3_per_link_metrics.csv`: Per-link metrics for all 6 presets x 7 links
- `data/fr3_mode_comparison.json`: Full JSON dump of all results
- Per-link data files supporting per-link claims

## Results

### Aggregate Summary

| Mode | Preset | Primitives | All Covered | Worst Dist (m) | Worst capV/aabb | Worst r/binMed | Runtime (s) |
|---|---|---|---|---|---|---|---|
| convex | default | 8 | yes | 0.000000 | -- | -- | 0.5 |
| sphere | single | 8 | no | 0.000020 | -- | -- | 10.1 |
| sphere | default | 63 | no | 0.017608 | -- | -- | 40.7 |
| capsule | single | 8 | no | 0.006689 | 1.3684 | 1.2684 | 12.7 |
| capsule | default | 10 | no | 0.010226 | 1.4700 | 1.4966 | 33.8 |
| capsule | high_detail | 10 | no | 0.002338 | 1.3245 | 1.3225 | 21.8 |

*Note: capV/aabb and r/binMed are capsule-specific metrics; convex and sphere modes use mesh-based output rather than capsule primitives.*

### Per-Link Coverage

**Per-link data files do not exist in the artifact.** Only aggregate (worst-case across links) values were captured. Without per-link data, the following cannot be verified from this artifact alone:

- The "0.63 capV/aabb lower bound" claim in the paper (experiments.tex L103): no per-link capV/aabb values exist to confirm any link achieves capV/aabb < 1.0. All aggregate rows report **worst-case** capV/aabb values only (minimum observed worst-case: 1.3245 for capsule/high_detail).
- Per-link primitive distribution differences between `default` and `high_detail` presets (both total 10, but per-link distribution may differ).
- Which specific link(s) produce the worst-case value for each preset.

**Available aggregate insight**: The `worst_capV_aabb` column shows the single worst link's capV/aabb for each capsule preset. The minimum worst-case across capsule presets is 1.3245 (high_detail). This suggests individual links may have lower (tighter) capV/aabb values below 1.0, but the data is not available to confirm.

### Qualitative Overlays

No overlay screenshots exist in the artifact. The request specified `figures/fr3_link0_*.png` but these were not generated.

## Claim Support Summary

- **supported**: Mode trade-offs are clearly demonstrated. Convex mode is fastest (0.5 s) with perfect coverage (mesh-based, 8 primitives). Capsule modes produce analytic primitives with measurable volume overhead (worst-case capV/aabb 1.32--1.47 across presets). Sphere tree (default) produces more primitives (63) with per-link violations. Sphere/single achieves near-perfect coverage (2e-5 m worst uncovered) with only 8 primitives but does not guarantee enclosure.
- **partially_supported**: The "0.63 capV/aabb lower bound" claim cannot be verified -- no per-link data exists in the artifact to identify any link with sub-1.0 capV/aabb. The minimum worst-case capV/aabb across all capsule presets is 1.3245 (capsule/high_detail). Generate per-link metrics to confirm or correct this bound.

## Limitations
- Only FR3 arm links (link0--link7); hand links excluded
- No analytic-primitive mode achieves full coverage on all links
- Docker startup overhead may inflate runtime measurements (all runs include pipeline initialization)
- `high_detail` not strictly better than `default` on all links (identical total primitive count of 10)
- Per-link metric files (`fr3_per_link_metrics.csv`) and full JSON dump (`fr3_mode_comparison.json`) not generated -- aggregate data only
- No qualitative overlay figures generated
- capV/aabb and r/binMed metrics are reported as worst-case across all links, not per-link

## Next Actions For Writer
- Report worst-case capV/aabb values from aggregate data (range 1.32--1.47 across presets) rather than claiming a 0.63 lower bound without per-link data
- Note that sphere tree is an approximation, not an enclosure (all_covered=false for both sphere presets)
- Do not claim any mode achieves universal full coverage (no analytic mode has all_covered=true)
- Table footnote needed: capV/aabb and r/binMed are capsule-specific metrics; "--" for convex/sphere is correct
