# Parameter Tuning

Each mode is driven by a YAML config. The CLI ships named presets (`presets`
lists them); pass a custom config with `--config` to override any field without
forking the preset.

```bash
# start from a preset, then edit a copy
cp config/capsule/default.yml /tmp/my_capsule.yml
# edit NSections / MaxCapsulesPerLink / ...
urdf-approx-geom generate --mode capsule -i robot.urdf -o out.urdf --config /tmp/my_capsule.yml
```

Tune, then close the loop with `validate` (worst `capV/aabb`, `r/binMed`) or
`compare` against a baseline sidecar.

## Capsule

Capsule fitting decomposes each link into axial sections (planes perpendicular
to the principal axis), fits circles per section, then chains circles into
capsules (Wu2018 cross-section decomposition).

| Field | Default | Effect |
|-------|--------:|--------|
| `NSections` | 4 | Axial slices per link. Raise for long, varying-section links (cost: runtime + capsule count). `high_detail` uses 6. |
| `MaxCapsulesPerLink` | 12 | Capsule budget per link. The fitter only accepts splits that improve coverage, so a higher cap does not force more capsules. `high_detail` allows 16. |
| `CoaThreshold` | 0.005 | Coverage-of-area acceptance per section; lower = tighter fit, more capsules. |
| `MaxRadiusBinRatio` | 1.45 | Max radius spread allowed within one capsule before it splits. Lower = more, thinner capsules. |
| `MinSplitVolumeImprovement` | 0.005 | A split is rejected unless it removes at least this fraction of excess volume. Raise to prune marginal splits. |
| `MaxCapVAabbRatio` | -1.0 | Hard ceiling on `capsuleVolume / aabbVolume` per link; `-1` = unset. `high_detail` sets `2.25`. |
| `AdaptiveCircleCount` | false | Let circles-per-section grow with section complexity. |
| `MaxCirclesPerSection` | 1 | Cap on that. |

**Levers by goal:**
- *Fewer capsules:* lower `MaxCapsulesPerLink`, raise `MinSplitVolumeImprovement`.
- *Tighter fit:* lower `CoaThreshold`, lower `MaxRadiusBinRatio`, raise `NSections`.
- *Bounded looseness:* set `MaxCapVAabbRatio` to fail fast on bad links.

## Sphere

Sphere mode builds a sphere tree. `Method` selects the algorithm:

| `Method` | Name | Character |
|---------:|------|-----------|
| 0 | Grid | uniform voxel-grid seeding |
| 1 | Hubbard | bounding-sphere hierarchy |
| 2 | Medial | medial-axis seeding (default; tightest) |
| 3 | Spawn | surface-point spawning |
| 4 | Octree | octree decomposition |

Key fields:

| Field | Effect |
|-------|--------|
| `SingleSphere` | One conservative sphere per link (`single` preset). |
| `SimplifyRatio` | Post-fit decimation fraction (default `0.02`). Raise to prune more spheres. |
| `<Method>.NumCoverPts` | Cover-point samples driving the fit. Raise for dense meshes. |
| `<Method>.Depth` / `Branch` | Tree depth / branching. |
| `Medial.InitSpheres` / `SpheresPerNode` | Medial-axis seeding density. |

**Levers by goal:**
- *Fewer spheres:* raise `SimplifyRatio`, switch to `SingleSphere: true`.
- *Tighter coverage:* raise `NumCoverPts`, lower `SimplifyRatio`, keep `Method: 2`.

## Convex

Convex mode has no runtime knobs — it computes the CGAL convex hull of each
link. Pick it when you want the tightest mesh-based collision and your simulator
handles convex meshes natively.
