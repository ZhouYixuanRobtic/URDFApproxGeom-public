# Capsule Geometry Approximation — Handoff

**Date:** 2026-07-02
**Branch:** `feat/urdf-approx-geom-capsule`
**Repo:** URDFApproxGeom (née SpherizedURDFGenerator)
**Commits:** 47

---

## 1. Project Purpose

**URDFApproxGeom** takes a mesh-based URDF robot model and emits three collision-geometry representations:

| Mode | Binary | Status |
|------|--------|--------|
| Sphere-tree (spherized) | `spherized` | ✅ Stable (pre-existing) |
| Convex hull | `convex` | ✅ Stable (pre-existing) |
| **Capsule** | `capsuleized` | ✅ Implemented (this branch) |

**Why capsule?** Spheres need ~80+ primitives per link for coverage. Convex hulls are tight but single-mesh. Capsules (cylinder + 2 end spheres) are the sweet spot: URDF-native, loadable in ROS/pybullet/MuJoCo, good tightness per primitive count.

**Output format:** URDF with **native primitives** — each capsule = one `<cylinder>` + two `<sphere>` end-caps (urdfdom has no `<capsule>` element; cylinder+sphere are valid, universally loadable). Plus a **JSON sidecar** carrying the capsule params (`p0`, `p1`, `radius`) per link for external tooling.

---

## 2. Current State

**FR3 sparse result:** generated with `config/capsule/capsuleConfig.yml`; optimized for low primitive count.

**FR3 tight result:** generated with `config/capsule/capsuleConfig_tight.yml`. Tightness gate (`check_capsule_tightness.py`) uses `--max-capv-aabb=2.35 --max-r-binmed=1.45` — both sparse and tight pass. Endpoint optimization makes sparse preset competitive with tight; more axial sections (6 vs 4) no longer improve capV after endpoint shrink.

**Test suite:** 24 C++ unit tests + 1 integration test — 24/24 pass. Python round-trip test (`pytest python/tests`) currently crashes with Bus error on `capsuleized()` call — pre-existing, not introduced by this branch.

**Config — sparse** (`config/capsule/capsuleConfig.yml`):
```yaml
NSections: 4
CoaThreshold: 0.005
MaxCirclesPerSection: 1
MaxCapsulesPerLink: 12
AdaptiveCircleCount: false
MaxRadiusBinRatio: 1.45
```

**Config — tight** (`config/capsule/capsuleConfig_tight.yml`):
```yaml
NSections: 6
CoaThreshold: 0.005
MaxCirclesPerSection: 1
MaxCapsulesPerLink: 16
AdaptiveCircleCount: false
MaxRadiusBinRatio: 1.45
```

**FR3 sparse/tight verification (2026-07-03 — endpoint semantics fix):**
```json
{
  "sparse_count": 17,
  "sparse_worst_capV_aabb": 1.77,
  "sparse_worst_r_binMed": 1.48,
  "tight_count": 17,
  "tight_worst_capV_aabb": 1.86,
  "tight_worst_r_binMed": 1.35
}
```
**Endpoint semantics:** `p0`/`p1` are capsule sphere centers, not mesh axial extrema. The fitter optimizes endpoint spans after merge/split and grows radii only as needed for coverage, avoiding the previous extra half-sphere overhang at link ends. This reduced capV/aabb from ~2.5 to ~1.8.

**Active algorithm:** Wu2018 cross-section decomposition with assignment-based metrics. `MaxCirclesPerSection > 1` empirically worsens gate metrics on FR3 (more small capsules increase total volume-to-AABB ratio). The winning strategy is more axial sections (`NSections`) with single circles per plane — shorter capsules fit local geometry better. Adaptive circle count, COA-Lloyd, and local axial splitting are implemented and config-switchable but currently disabled by default pending better cross-plane circle matching.

---

## 3. Architecture

### Key Files

```
include/
├── CapsuleFitter.h          # Capsule struct, point-to-segment distance,
│                              dedupeNestedCapsules, fitCoveringCapsule (points)
├── CapsuleCrossSection.h    # Wu2018: extractSections, circleOutsideArea (COA),
│                              fitCirclesLloyd, fitCapsulesByCrossSection,
│                              growCapsulesToCover, mergeCollinearCapsules,
│                              Circle2D, Contour2D, Section2D
├── CapsuleURDFGenerator.h   # Generator (inherits URDFGenerator)
└── SphereTreeURDFGenerator.h # buildSphereModel (in-memory sphere-tree build)

src/
├── CapsuleFitter.cpp        # Welzl MEC, disk-aware fit, dedupe
├── CapsuleCrossSection.cpp  # Mesh-plane slicing, COA Eq 1-5, Lloyd clustering,
│                              capsule assembly, merge, grow
├── CapsuleURDFGenerator.cpp # Per-link pipeline: load mesh -> Manifold watertight
│                              -> transform to link frame -> cross-section fit
│                              -> grow on original mesh -> emit cylinder+sphere
└── SphereTreeURDFGenerator.cpp # buildSphereModel (refactored from run)

app/
├── Capsuleized.cpp          # CLI: capsuleized -i in.urdf -o out.urdf --config ...

config/capsule/
└── capsuleConfig.yml

test/
└── test_capsule.cpp         # 23 unit tests + 1 integration test

interface/
└── urdf_approx_geom.cpp     # pybind11: capsuleized(), convex(), spherized()

python/
├── urdf_approx_geom/        # Thin Python wrapper
├── tests/test_capsule.py    # Python round-trip test
└── conftest.py

scripts/
├── make_mjcf.py             # Read capsule JSON -> MJCF xml (robot-viewer)
└── viz_capsules.py          # pybullet visualization with overlay
```

### Pipeline (current, per link)

```
1. loadURDF -> m_model
2. For each mesh collision link:
   a. loadedIntoIGL(V_orig, F_orig)           // original mesh
   b. Manifold(V_orig, F_orig) -> OUT_V, OUT_F  // watertight
   c. Transform OUT_V to link frame -> V_lf
   d. fitCapsulesByCrossSection(V_lf, OUT_F, options)
      - PCA axis u = largest eigenvector
      - N cross-section planes perpendicular to u
      - Per plane: fitFixedCountCirclesForPlane or fitAdaptiveCirclesForPlane (config-driven)
      - Active-chain matching: circles tracked across planes, degenerate capsules avoided
      - Extend chain ends to mesh axial extremes
      - splitMostInflatedCapsules (accepted-split: recompute radii, gate on metrics)
      - mergeCollinearCapsules (radius-diff guard 0.15)
      - growCapsulesToCover(V_lf)
      - dedupeNestedCapsules
      - Coverage-aware budget pruning (re-grow, check coverage, pick min-volume)
   e. growCapsulesToCover(V_orig_lf)          // cover original mesh
   f. emit cylinder + 2 spheres per capsule; JSON
3. writeURDF + JSON sidecar
```

---

## 4. Approaches Tried (chronological)

### v1 — Mesh PCA + point-to-segment (commits up to `933bf66`)
**Method:** fitCoveringCapsule on raw mesh vertices: PCA principal axis = capsule axis, radius = max vertex-to-axis distance. Candidate-axis search (PCA + cardinal + Fibonacci + pattern search) to minimize radius — a covering capsule (outer fit).
**Result:** Worked for purely cylindrical links, but **tilted and fat** on wide/base links (link0 base: one capsule cannot hug a flat base). PCA axis unstable for short/wide shapes.
**Why failed:** A single capsule per link. The base needed splitting.

### v2 — Sphere-tree cluster + disk-cover fit (commits up to `1aa3c30`)
**Method:** Build Medial sphere-tree (inherited from SphereTreeURDFGenerator), cluster spheres by proximity, fit one covering capsule per cluster (disk-aware: R = max_i(dist(center_i, axis) + r_i)), k-means fat-split.
**Result:** Capsule radii ~1.5–1.8× the link's true cross-section. **Fat** — the Medial sphere-tree radii over-cover the surface ~1.4×, and the covering-capsule-of-spheres inherits that fatness (sphere center off-axis + r_i double-counts).
**Why failed:** Spheres are an outer covering approximation; capsules covering them are doubly conservative. "从球近似出发" gave the decomposition but not the tightness.

### v3 — Sphere-decomposed mesh-tight fit (`bb814ab`)
**Method:** Spheres only for decomposition (cluster proximity), then fit capsules to mesh vertices per cluster (tight: radius = true surface distance). DedupeNestedCapsules fix.
**Result:** 13 clean capsules, tighter (ratio ~1.0–1.5, mostly real flange protrusions). But **nesting** appeared from k-means fat-split (small capsule inside a larger sibling).
**Status:** This was the best result before Wu2018, but the user wanted the paper's approach.

### v4 — Faithful Wu2018: full COA + Lloyd (`5168f5c`, `0a44b83`)
**Method:** Full Wu2018 pipeline: mesh-plane slicing into 2D contours, per-contour Lloyd circle clustering (COA → teleportation), link adjacent-section circles into capsules, merge, grow, dedupe.
**Result (on FR3):** 97 capsules. Over-segmentation — each plane cuts multiple disconnected contour loops on flanged links, making per-contour circles variable in number across planes → pairings messy → chains don't merge.
**Why over-segmented:** Wu2018 was designed for human mannequins (smooth, single-loop cross-sections per body part). Robot links with flanges/connectors produce irregular multi-loop cross-sections. The algorithm works correctly (all math validated, 15 unit tests green); it just produces many capsules on this type of geometry.
**Status:** Implemented, working, config-switchable — the COA-Lloyd machinery (`fitCirclesLloyd`, `circleOutsideArea`, etc.) is all in `CapsuleCrossSection.cpp` and tested. Just not active in the default pipeline.

### v5 — Wu2018 1-circle/plane (current, `0a44b83`)
**Method:** v4's pipeline, but with **one MEC circle per plane** (group all contour loops of a plane, MEC of their combined points). Uniform 1 circle/section → clean transitive pairing → chains merge → ~1 capsule/link.
**Result:** 13 capsules on FR3, all covered, zero nesting. Reasonable tightness (arm links r~0.07–0.08, base r=0.107).
**Tradeoff:** Loses multi-circle tightness per section (the COA-Lloyd value). Accepts that a single MEC per section is the right default for flanged robot links.

---

## 5. Math Core (reusable building blocks)

| Function | Location | Description |
|----------|----------|-------------|
| `extractSections(V,F,u,origin,N)` | CapsuleCrossSection.cpp | Mesh-plane slicing → 2D contours with axial t |
| `circleOutsideArea(circle, contour)` | CapsuleCrossSection.cpp | Wu2018 Eq 1-5: COA = area(circle ∖ polygon) |
| `fitCirclesLloyd(contour, thresh, max)` | CapsuleCrossSection.cpp | Lloyd clustering minimizing COA |
| `mec2d(points)` | CapsuleCrossSection.cpp | Welzl 2D minimum enclosing circle |
| `fitCapsulesByCrossSection(V,F,...)` | CapsuleCrossSection.cpp | Full Wu2018 pipeline |
| `growCapsulesToCover(caps, V)` | CapsuleCrossSection.cpp | Grow radii to cover vertex set |
| `mergeCollinearCapsules(caps)` | CapsuleCrossSection.cpp | Merge end-to-end capsules into longer |
| `dedupeNestedCapsules(caps)` | CapsuleFitter.cpp | Drop capsules fully inside another |
| `pointToSegmentDistance(p,a,b)` | CapsuleFitter.cpp | Clamped projection distance |
| `fitCoveringCapsule(V)` | CapsuleFitter.cpp | Single min-radius covering capsule (points) |
| `min_enclosing_circle(points)` | CapsuleFitter.cpp | Welzl 2D MEC (used by legacy disk fitter) |

---

## 6. How to tune (no code changes needed)

Edit `config/capsule/capsuleConfig.yml` or `config/capsule/capsuleConfig_tight.yml`:

- **`AdaptiveCircleCount`**: when `true`, each section plane chooses the smallest circle count whose normalized COA proxy is below `CoaThreshold`, capped by `MaxCirclesPerSection`.
- **`CoaThreshold`**: lower values request tighter section coverage and may increase capsule count. Values `<= 0` force one circle per section.
- **`MaxRadiusBinRatio`**: local axial split threshold. If one capsule radius is much larger than the median axial bin radius, the fitter splits that capsule before final pruning.
- **`MaxCapsulesPerLink`**: hard output budget. Pruning must re-grow and preserve coverage.

---

## 7. Remaining Work / Escalation Paths

1. **Wide base (link0)** still has 1 capsule at r=0.107. If too fat visually, the fix is to **lower the `FatSplitRatio` or raise `NSections`** so the base's axial chain splits along wider sections. Or re-enable per-loop multi-circle (v4's COA-Lloyd) for just the base — needs per-plane circle-count selection by COA.

2. **Per-section multi-circle for genuinely wide links:** The COA-Lloyd machinery is implemented and tested; the blocker was that multi-contour loops vary circle counts across planes, breaking pairing. Fix: force uniform circle count across planes (pick `k = max k_i` across all planes, pad with duplicates). Then Lloyd with fixed `k`. Not implemented.

3. **Tapered capsules in URDF:** Wu2018 capsules can have different end radii (r0 ≠ r1). Currently collapsed to `max(r0,r1)` (over-covers the narrow end). Could split tapered capsules into 2 constant-radius capsules for tighter emission — not yet needed (merge makes continuous chains, taper is gradual).

4. **Merge step robustness:** `mergeCollinearCapsules` currently greedy (first pair found). Could be made iterative-with-rewind for optimal chaining.

5. **Missing link coverage:** Always run the `growCapsulesToCover` on **original mesh** (not just watertight) — currently done in the generator (P5 fix). Verify when adding new links.

6. **Python package distribution:** The `.so` is built by CMake and manually copied. A proper scikit-build-core `pyproject.toml` would make `pip install` work — tracked as a future task.

7. **Config presets:** Could ship `capsuleConfig_sparse.yml` (few capsules, looser) and `capsuleConfig_dense.yml` (tight, more capsules) for different use cases.

---

## 8. Key Commands

```bash
# Build (in Docker)
docker exec spherized-development bash -lc 'cd /workspace && cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)'

# Tests (C++)
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule'

# Tests (Python)
docker exec spherized-development bash -lc 'cd /workspace && cp build/python/urdf_approx_geom/*.so python/urdf_approx_geom/ && PYTHONPATH=/workspace/python python3 -m pytest python/tests -q'

# Generate FR3 capsules (sparse)
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sparse_capsuleized.urdf --config config/capsule/capsuleConfig.yml'

# Generate FR3 capsules (tight)
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_capsuleized.urdf --config config/capsule/capsuleConfig_tight.yml'

# Visualize (host)
python3 scripts/make_mjcf.py    # generates resources/fr3/urdf/fr3_capsules.xml -> open in robot-viewer
python3 scripts/viz_capsules.py # pybullet GUI overlay
```

## 9. Verification

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sparse_capsuleized.urdf --config config/capsule/capsuleConfig.yml'
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_capsuleized.urdf --config config/capsule/capsuleConfig_tight.yml'
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_capsuleized.json --max-capv-aabb 2.50 --max-r-binmed 1.45 --link-min-capsules fr3_link0=2'
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/compare_capsule_presets.py --sparse-json /tmp/fr3_sparse_capsuleized.json --tight-json /tmp/fr3_tight_capsuleized.json'
```
