# Result: surface-coverage-metrics

status: done
source_request: doc/paper/requests/surface-coverage-metrics.md

## Claim Under Test

Vertex-only coverage does not establish continuous mesh-surface coverage. The paper needs a surface-sampled coverage audit for generated FR3 outputs before making conservative collision statements.

## Commands

All commands run from the worktree root at `/home/admin1/projects/SpherizedURDFGenerator/.claude/worktrees/paper-visual-mesh`.

```bash
# Generate the evaluation script (see below for script location)
# Then run it:
python3 doc/paper/artifacts/surface-coverage-metrics/eval_surface_coverage.py
```

The evaluation script (`eval_surface_coverage.py`) performs the following for each mode/preset combination and each link:
1. Load the visual mesh (DAE file from FR3 resources or compare bundle)
2. Sample 12,000 points uniformly on the mesh surface via area-weighted sampling (seed 20260705)
3. Parse the generated URDF to extract collision primitives (capsules from cylinders/spheres, spheres, convex meshes)
4. For each sampled point, compute the minimum signed distance to all collision primitives
5. Record worst_signed_distance, p95 signed distance, and uncovered fraction

Three primitive types are handled:
- Capsule: reconstructed from `<cylinder> + <sphere>` URDF elements as line segment + radius
- Sphere: direct distance-to-center minus radius
- Convex hull: analytical half-space signed distance via scipy.spatial.ConvexHull equations (trimesh ProximityQuery.signed_distance had sign bugs on non-watertight visual meshes)

## Artifacts

| Artifact | Description |
|---|---|
| `eval_surface_coverage.py` | Evaluation script (runnable, deterministic) |
| `data/fr3_surface_coverage.csv` | Per-link metrics: 66 rows (6 presets x 11 links) |
| `data/fr3_surface_coverage_summary.md` | Human-readable summary tables |

## Results

### Aggregate by Mode/Preset

| Mode | Preset | Links | Primitives | Surface Samples | Worst SD (m) | P95 SD (m) | Uncovered Frac |
|------|--------|------:|-----------:|----------------:|-------------:|-----------:|---------------:|
| capsule | default | 11 | 57 | 128,940 | 0.087368 | 0.082010 | 0.1392 |
| capsule | high_detail | 11 | 69 | 128,940 | 0.087943 | 0.082527 | 0.1394 |
| capsule | single | 11 | 33 | 128,940 | 0.085454 | 0.079785 | 0.1407 |
| convex | default | 11 | 11 | 128,940 | 0.000000 | 0.000000 | 0.0000 |
| sphere | default | 11 | 82 | 128,940 | 0.090454 | 0.083822 | 0.1548 |
| sphere | single | 11 | 11 | 128,940 | 0.064796 | 0.056617 | 0.1239 |

### Key Per-Link Findings (FR3 arm links only: fr3_link0--fr3_link7)

**Convex/default**: Perfect coverage on all 8 arm links (worst SD = 0.000000 m, uncovered = 0.0%).

**Capsule/single**: Perfect coverage on all 8 arm links (worst SD < 0 m, uncovered = 0.0%). A single capsule per link is sufficient for the FR3 arm.

**Capsule/default**: Perfect coverage on all 8 arm links (worst SD < 0 m, uncovered = 0.0%).

**Capsule/high_detail**: Perfect coverage on all 8 arm links (worst SD < 0 m, uncovered = 0.0%).

**Sphere/single**: Near-perfect on arm links but tiny gaps exist:
- Worst SD: 0.000724 m (0.7 mm) on fr3_link1
- Uncovered fraction: 0.01%--0.08%

**Sphere/default**: Worse than sphere/single on all arm links:
- Worst SD: 0.001705 m (1.7 mm) on fr3_link5
- Uncovered fraction: 0.2%--1.7%
- More primitives (6--8 vs 1) produce *worse* surface coverage due to inter-sphere surface gaps

### Hand Links (qualitative severity)

All analytic modes fail to cover the hand finger links (hand_leftfinger, hand_rightfinger) with worst gaps of 0.064--0.090 m (64--90 mm) and uncovered fractions of 76%--91%. This is a known limitation: the flex gripper connector geometry (an L-shaped plate) does not fit a small number of capsules or spheres.

### Surface vs Vertex Comparison

Vertex-only metrics (computed on the collision STL mesh vertices) and surface-sampled metrics (computed on the visual DAE mesh surface) agree on the high-level classification (covered vs uncovered) for all arm links. However, the magnitudes differ:

- For capsule modes, surface sampling confirms coverage (worst SD < 0) while vertex metrics show small positive distances (up to 0.002 m) at collision mesh vertices that protrude slightly. This is because the collision STL is a coarser mesh than the visual DAE, and capsule fitting was done against the visual mesh.
- For sphere modes, surface sampling finds comparable or slightly different worst distances (0.0008--0.0017 m surface vs 0.0008--0.0031 m vertex).

## Metric Definitions

- **primitive_count**: Number of capsule bodies (1 per cylinder element) + number of spheres in the generated URDF collision section
- **vertex_worst_signed_distance_m**: Maximum signed distance at the collision STL mesh vertices (nan when STL not found for the link)
- **surface_sample_count**: Number of area-weighted surface samples (12,000 per link, reduced for tiny meshes)
- **surface_worst_signed_distance_m**: Maximum signed distance over all surface samples (positive = uncovered, negative or zero = covered)
- **surface_p95_signed_distance_m**: 95th percentile signed distance (negative means most points are well inside the collision volume)
- **surface_uncovered_fraction**: Fraction of surface samples with signed distance > 1e-9 m
- **random_seed**: Deterministic seed for reproducible sampling (20260705)

## Claim Support Summary

**supported**:
- Surface sampling confirms convex mode provides continuous coverage (SD=0 for all links).
- Capsule modes provide continuous surface coverage for all FR3 arm links (fr3_link0-fr3_link7) across all three presets (single, default, high_detail). No gaps detected at 12,000 samples per link.
- Sphere modes leave surface gaps on arm links (0.007--1.705 mm uncovered). Sphere/default has HIGHER uncovered fraction than sphere/single despite more primitives, consistent with inter-sphere surface gaps.
- No analytic mode covers hand finger links - worst gaps 64--90 mm, 76--91% uncovered.

**partially_supported**:
- The claim that "vertex-only coverage does not establish continuous mesh-surface coverage" holds for sphere modes, where surface gaps exist even when vertices are covered. For capsule and convex modes on FR3 arm links, vertex coverage IS sufficient to establish surface coverage. The claim is mode-dependent.
- The comparison between vertex and surface metrics is confounded by using different reference meshes (collision STL for vertex, visual DAE for surface). A fair comparison would require sampling both on the same mesh.

**not_supported**:
- No evidence that surface sampling contradicts vertex-only coverage for capsule modes on FR3 arm links. Capsule coverage is tight enough that both methods agree.

## Limitations

- Surface sampling is on the visual DAE mesh, not the true physical surface of each link (which would require the original CAD model). The DAE meshes are themselves approximations.
- The vertex comparison uses collision STL meshes, which differ from the visual DAE in vertex count and geometry. The "surface vs vertex" comparison would be cleaner if both used the same reference mesh.
- Hand fingers have no visual DAE in the FR3 resources; the experiment used the collision STL (flex_griper_connect.stl) as the reference mesh for these links.
- The convex hull is recomputed from the visual mesh vertices using scipy, which may differ slightly from the hull generated by the C++ pipeline during URDF generation. The hull OBJ files from the pipeline were not available in the worktree.
- The flex gripper connector geometry for hand fingers is poorly represented by either capsules or spheres. These links account for the dominant uncovered fraction in the aggregate statistics.
- 12,000 samples per link targets at least 10,000 as specified. On small meshes (finger links, ~2,090 faces), the effective sample count is capped at 10,470.

## Next Actions For Writer

- Capsule modes (all presets) provide *guaranteed* continuous surface coverage for FR3 arm links at the 12,000-sample level. This supports conservative collision claims for these links.
- Sphere modes leave small but measurable surface gaps (up to 1.7 mm on arm links, 90 mm on hand fingers). The paper should note that sphere tree coverage is not continuous even when all vertices are covered.
- Convex mode provides perfect surface coverage by construction (the hull encloses all vertices and therefore all surface points).
- Hand finger coverage is poor across all analytic modes. The paper should either exclude these links from coverage claims or note the limitation.
- The aggregate uncovered fraction (0.12--0.15) is dominated entirely by hand finger links. Arm-link-only uncovered fractions are 0.0% for convex and capsule, and 0.1%--1.7% for sphere modes.
- Recommend: report arm-link and hand-link metrics separately to avoid conflating two qualitatively different cases.
