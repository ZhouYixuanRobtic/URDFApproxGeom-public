# Capsule Endpoint Semantics Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix capsule endpoint semantics so `p0`/`p1` are sphere centers optimized for coverage and volume, not mesh axial extrema that add an avoidable half-sphere overhang.

**Architecture:** Preserve the existing cross-section fitter and URDF representation (`cylinder` plus two `sphere` primitives). Add an endpoint-span optimization pass that moves capsule centers inward along each capsule axis when doing so reduces total capsule volume while preserving coverage. Update tests and diagnostics so the suite catches the current overhang bug instead of only checking capsule count.

**Tech Stack:** C++17, Eigen, GoogleTest, Python 3 diagnostic scripts, yaml-cpp, Docker container `spherized-development`.

---

## File Structure

- Modify `test/test_capsule.cpp`: add a capsule-surface regression mesh, update endpoint expectations, and move integration-test output paths to `/tmp`.
- Modify `src/CapsuleCrossSection.cpp`: add endpoint-span optimization helpers and call them after merge, split, and budget pruning.
- Modify `include/CapsuleCrossSection.h`: no API change unless a helper must be exposed for tests; prefer keeping helpers private in `src/CapsuleCrossSection.cpp`.
- Modify `scripts/check_capsule_coverage.py`: report assigned axial overhang so visual over-expansion is machine-readable.
- Modify `scripts/check_capsule_tightness.py`: optionally enforce per-link axial overhang limits when provided.
- Modify `scripts/compare_capsule_presets.py`: restore a tighter `capV/aabb` ceiling after endpoint optimization.
- Modify `config/capsule/capsuleConfig_tight.yml`: keep `MaxCirclesPerSection: 1`; tune only after endpoint semantics are fixed.
- Modify `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`: document endpoint semantics and verified metrics.
- Do not intentionally commit generated `build/` files. Do not let tests write `resources/fr3/urdf/fr3_capsuleized.*`.

All verification commands must run inside the container:

```bash
docker exec spherized-development bash -lc 'cd /workspace && <command>'
```

---

### Task 1: Add a Failing Endpoint-Semantics Regression

**Files:**
- Modify: `test/test_capsule.cpp`

- [ ] **Step 1: Add a capsule-like mesh helper**

Insert this helper after `makeBox()` and before `makeTwoBoxLink()`:

```cpp
// Closed capsule surface with sphere centers at x=0 and x=center_distance.
// Mesh axial extrema are [-radius, center_distance + radius]; fitted capsule
// sphere centers should recover the center span, not the extrema.
static void makeCapsuleSurface(double radius,
                               double center_distance,
                               int radial_segments,
                               int hemi_segments,
                               Eigen::MatrixXd& V,
                               Eigen::MatrixXi& F) {
    std::vector<Eigen::Vector3d> verts;
    std::vector<std::vector<int>> rings;

    auto add_ring = [&](double x, double ring_radius) {
        std::vector<int> ring;
        ring.reserve(radial_segments);
        for (int i = 0; i < radial_segments; ++i) {
            double a = 2.0 * M_PI * i / radial_segments;
            ring.push_back(static_cast<int>(verts.size()));
            verts.emplace_back(x, ring_radius * std::cos(a), ring_radius * std::sin(a));
        }
        rings.push_back(ring);
    };

    int left_pole = static_cast<int>(verts.size());
    verts.emplace_back(-radius, 0.0, 0.0);
    for (int h = 1; h <= hemi_segments; ++h) {
        double phi = -0.5 * M_PI + h * (0.5 * M_PI / hemi_segments);
        add_ring(radius * std::sin(phi), radius * std::cos(phi));
    }
    add_ring(center_distance, radius);
    for (int h = 1; h < hemi_segments; ++h) {
        double phi = h * (0.5 * M_PI / hemi_segments);
        add_ring(center_distance + radius * std::sin(phi), radius * std::cos(phi));
    }
    int right_pole = static_cast<int>(verts.size());
    verts.emplace_back(center_distance + radius, 0.0, 0.0);

    std::vector<Eigen::Vector3i> faces;
    for (int i = 0; i < radial_segments; ++i) {
        int ni = (i + 1) % radial_segments;
        faces.push_back({left_pole, rings.front()[ni], rings.front()[i]});
    }
    for (size_t r = 0; r + 1 < rings.size(); ++r) {
        for (int i = 0; i < radial_segments; ++i) {
            int ni = (i + 1) % radial_segments;
            faces.push_back({rings[r][i], rings[r][ni], rings[r + 1][i]});
            faces.push_back({rings[r][ni], rings[r + 1][ni], rings[r + 1][i]});
        }
    }
    for (int i = 0; i < radial_segments; ++i) {
        int ni = (i + 1) % radial_segments;
        faces.push_back({rings.back()[i], rings.back()[ni], right_pole});
    }

    V.resize(static_cast<int>(verts.size()), 3);
    for (int i = 0; i < static_cast<int>(verts.size()); ++i) V.row(i) = verts[i].transpose();
    F.resize(static_cast<int>(faces.size()), 3);
    for (int i = 0; i < static_cast<int>(faces.size()); ++i) F.row(i) = faces[i];
}
```

- [ ] **Step 2: Add the failing test**

Insert this test after `CapsuleXSectionFit.CylinderToOneCoveringCapsule`:

```cpp
TEST(CapsuleXSectionFit, CapsuleSurfaceCentersDoNotUseMeshExtrema) {
    constexpr double radius = 0.05;
    constexpr double center_distance = 0.40;
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeCapsuleSurface(radius, center_distance, 32, 6, V, F);

    CapsuleFitOptions opts;
    opts.n_sections = 8;
    opts.coa_threshold = 0.005;
    opts.max_circles_per_section = 1;
    opts.max_capsules = 4;
    opts.max_radius_bin_ratio = -1.0;
    opts.max_capv_aabb_ratio = -1.0;
    opts.adaptive_circle_count = false;

    auto caps = fitCapsulesByCrossSection(V, F, opts);
    ASSERT_EQ(caps.size(), 1u);
    auto metrics = evaluateCapsuleTightness(V, caps);
    ASSERT_TRUE(metrics.covered);

    Eigen::Vector3d axis = caps[0].p1 - caps[0].p0;
    double length = axis.norm();
    ASSERT_GT(length, 1e-9);
    axis /= length;

    double mesh_min = std::numeric_limits<double>::max();
    double mesh_max = std::numeric_limits<double>::lowest();
    for (int i = 0; i < V.rows(); ++i) {
        double t = V.row(i).transpose().dot(axis);
        mesh_min = std::min(mesh_min, t);
        mesh_max = std::max(mesh_max, t);
    }
    double end0 = caps[0].p0.dot(axis);
    double end1 = caps[0].p1.dot(axis);
    double cap_min = std::min(end0, end1);
    double cap_max = std::max(end0, end1);

    EXPECT_GT(cap_min - mesh_min, 0.50 * radius)
        << "p0/p1 are sphere centers and should not sit on the mesh axial extrema";
    EXPECT_GT(mesh_max - cap_max, 0.50 * radius)
        << "p0/p1 are sphere centers and should not sit on the mesh axial extrema";
    EXPECT_NEAR(length, center_distance, 0.08);
    EXPECT_LT(metrics.capV_aabb, 1.80);
}
```

- [ ] **Step 3: Run the single test and verify it fails**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_color=no --gtest_filter=CapsuleXSectionFit.CapsuleSurfaceCentersDoNotUseMeshExtrema'
```

Expected: FAIL on the two `EXPECT_GT(..., 0.50 * radius)` checks because current code extends endpoints to mesh extrema.

- [ ] **Step 4: Commit**

```bash
git add test/test_capsule.cpp
git commit -m "test: capture capsule endpoint center semantics"
```

---

### Task 2: Stop Tests From Rewriting Tracked FR3 Assets

**Files:**
- Modify: `test/test_capsule.cpp`

- [ ] **Step 1: Change the sparse integration test output path**

In `CapsuleRun.EmitsNativeCylinderSphere`, replace:

```cpp
    const std::string out_urdf = "/workspace/resources/fr3/urdf/fr3_capsuleized.urdf";
```

with:

```cpp
    const std::string out_urdf = "/tmp/fr3_sparse_capsule_emit_test.urdf";
```

Replace:

```cpp
    std::ifstream f("/workspace/resources/fr3/urdf/fr3_capsuleized.json");
```

with:

```cpp
    std::ifstream f("/tmp/fr3_sparse_capsule_emit_test.json");
```

- [ ] **Step 2: Run the integration tests and verify tracked assets stay clean**

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule --gtest_color=no --gtest_filter=CapsuleRun.* && git status --short resources/fr3/urdf/fr3_capsuleized.json resources/fr3/urdf/fr3_capsuleized.urdf'
```

Expected after this task: both `CapsuleRun` tests pass or only the new endpoint test from Task 1 remains unrelated; `git status --short resources/...` prints no output.

- [ ] **Step 3: Commit**

```bash
git add test/test_capsule.cpp
git commit -m "test: keep capsule integration outputs in tmp"
```

---

### Task 3: Add Endpoint-Span Optimization

**Files:**
- Modify: `src/CapsuleCrossSection.cpp`

- [ ] **Step 1: Add required-radius and candidate construction helpers**

Insert these helpers after `capsuleSetVolume()` and before the anonymous namespace closes:

```cpp
static Capsule capsuleWithSpan(const Capsule& cap, double lo, double hi, double radius) {
    Eigen::Vector3d axis = cap.p1 - cap.p0;
    double length = axis.norm();
    if (length < 1e-12) return cap;
    Eigen::Vector3d u = axis / length;
    Capsule out;
    out.p0 = cap.p0 + lo * u;
    out.p1 = cap.p0 + hi * u;
    out.radius = radius;
    return out;
}

static double requiredRadiusForSpan(const Capsule& cap,
                                    const Eigen::MatrixXd& V,
                                    double lo,
                                    double hi) {
    Eigen::Vector3d axis = cap.p1 - cap.p0;
    double length = axis.norm();
    if (length < 1e-12 || lo >= hi) return std::numeric_limits<double>::infinity();
    Eigen::Vector3d u = axis / length;
    double radius = 0.0;
    for (int i = 0; i < V.rows(); ++i) {
        Eigen::Vector3d p = V.row(i).transpose();
        double t = (p - cap.p0).dot(u);
        double clamped = std::clamp(t, lo, hi);
        Eigen::Vector3d q = cap.p0 + clamped * u;
        radius = std::max(radius, (p - q).norm());
    }
    return radius;
}

static Capsule optimizeEndpointSpanForAssignedVertices(const Capsule& cap,
                                                       const Eigen::MatrixXd& V) {
    Eigen::Vector3d axis = cap.p1 - cap.p0;
    double length = axis.norm();
    if (length < 1e-12 || V.rows() == 0) return cap;
    Eigen::Vector3d u = axis / length;

    double tmin = std::numeric_limits<double>::max();
    double tmax = std::numeric_limits<double>::lowest();
    for (int i = 0; i < V.rows(); ++i) {
        double t = (V.row(i).transpose() - cap.p0).dot(u);
        tmin = std::min(tmin, t);
        tmax = std::max(tmax, t);
    }
    if (tmax - tmin < 1e-12) return cap;

    std::vector<double> lows;
    std::vector<double> highs;
    lows.push_back(0.0);
    highs.push_back(length);
    for (int k = 0; k <= 20; ++k) {
        double f = 0.35 * double(k) / 20.0;
        lows.push_back(tmin + f * (tmax - tmin));
        highs.push_back(tmax - f * (tmax - tmin));
    }

    Capsule best = cap;
    double best_volume = capsuleVolume(cap);
    for (double lo : lows) {
        for (double hi : highs) {
            if (hi - lo < 1e-9) continue;
            double radius = requiredRadiusForSpan(cap, V, lo, hi);
            if (!std::isfinite(radius) || radius <= 0.0) continue;
            Capsule candidate = capsuleWithSpan(cap, lo, hi, radius);
            double volume = capsuleVolume(candidate);
            if (volume < best_volume) {
                best = candidate;
                best_volume = volume;
            }
        }
    }
    return best;
}
```

- [ ] **Step 2: Add the vector-level shrink pass**

Insert this helper after `optimizeEndpointSpanForAssignedVertices()`:

```cpp
static bool shrinkCapsuleEndpointSpans(std::vector<Capsule>& caps,
                                       const Eigen::MatrixXd& V) {
    if (caps.empty() || V.rows() == 0) return false;
    auto before = evaluateCapsuleTightness(V, caps);
    auto assignment = assignVerticesToCapsules(V, caps);

    std::vector<Capsule> candidate = caps;
    for (int i = 0; i < static_cast<int>(caps.size()); ++i) {
        Eigen::MatrixXd local = assignedVerticesForCapsule(V, assignment, i);
        if (local.rows() == 0) continue;
        candidate[i] = optimizeEndpointSpanForAssignedVertices(caps[i], local);
    }
    growCapsulesToCover(candidate, V);
    candidate = dedupeNestedCapsules(candidate);
    auto after = evaluateCapsuleTightness(V, candidate);
    if (!after.covered) return false;
    if (after.capsule_volume >= before.capsule_volume * 0.999) return false;
    caps = std::move(candidate);
    return true;
}
```

- [ ] **Step 3: Call shrink pass after merge and grow**

Replace this block:

```cpp
    caps = mergeCollinearCapsules(caps);
    growCapsulesToCover(caps, V);
```

with:

```cpp
    caps = mergeCollinearCapsules(caps);
    growCapsulesToCover(caps, V);
    while (shrinkCapsuleEndpointSpans(caps, V)) {
    }
```

- [ ] **Step 4: Call shrink pass on split candidates**

Inside `splitMostInflatedCapsule()`, after:

```cpp
            growCapsulesToCover(candidate, V);
            candidate = dedupeNestedCapsules(candidate);
```

add:

```cpp
            while (shrinkCapsuleEndpointSpans(candidate, V)) {
            }
```

- [ ] **Step 5: Call shrink pass after split loop and final grow**

After the existing split-loop `growCapsulesToCover(caps, V);`, add:

```cpp
        while (shrinkCapsuleEndpointSpans(caps, V)) {
        }
```

At the end of `fitCapsulesByCrossSection()`, replace:

```cpp
    growCapsulesToCover(caps, V);
    caps = dedupeNestedCapsules(caps);
    return caps;
```

with:

```cpp
    growCapsulesToCover(caps, V);
    while (shrinkCapsuleEndpointSpans(caps, V)) {
    }
    caps = dedupeNestedCapsules(caps);
    return caps;
```

- [ ] **Step 6: Run the endpoint regression**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_color=no --gtest_filter=CapsuleXSectionFit.CapsuleSurfaceCentersDoNotUseMeshExtrema'
```

Expected: PASS.

- [ ] **Step 7: Run focused cross-section tests**

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule --gtest_color=no --gtest_filter=CapsuleXSectionFit.*'
```

Expected: all `CapsuleXSectionFit` tests PASS. If `CylinderToOneCoveringCapsule` fails only because segment length is now slightly shorter than `1.0`, update that assertion in Task 4 rather than weakening coverage checks here.

- [ ] **Step 8: Commit**

```bash
git add src/CapsuleCrossSection.cpp
git commit -m "fix: optimize capsule endpoint spans"
```

---

### Task 4: Correct Tests That Encoded the Old Endpoint Semantics

**Files:**
- Modify: `test/test_capsule.cpp`

- [ ] **Step 1: Update cylinder endpoint expectations if needed**

If `CapsuleXSectionFit.CylinderToOneCoveringCapsule` fails after Task 3, replace:

```cpp
    EXPECT_NEAR((caps[0].p1 - caps[0].p0).norm(), 1.0, 2e-2);
```

with:

```cpp
    double fitted_length = (caps[0].p1 - caps[0].p0).norm();
    EXPECT_GT(fitted_length, 0.90);
    EXPECT_LE(fitted_length, 1.0)
        << "p0/p1 are capsule sphere centers; they should not extend beyond the mesh just to match axial extent";
```

Keep the existing per-vertex coverage loop unchanged.

- [ ] **Step 2: Run the affected tests**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_color=no --gtest_filter=CapsuleXSectionFit.CylinderToOneCoveringCapsule:CapsuleXSectionFit.CapsuleSurfaceCentersDoNotUseMeshExtrema'
```

Expected: both tests PASS.

- [ ] **Step 3: Commit**

```bash
git add test/test_capsule.cpp
git commit -m "test: align capsule length checks with endpoint centers"
```

---

### Task 5: Remove Non-Improving Split Acceptance

**Files:**
- Modify: `src/CapsuleCrossSection.cpp`
- Modify: `config/capsule/capsuleConfig_tight.yml`
- Modify: `scripts/compare_capsule_presets.py`

- [ ] **Step 1: Remove the least-bad split fallback**

In `splitMostInflatedCapsule()`, remove these variables:

```cpp
    bool found = false;
    double best_managing_score = std::numeric_limits<double>::max();
    std::vector<Capsule> best_managing_candidate;
```

Keep:

```cpp
    bool found_improving = false;
    double best_score = std::numeric_limits<double>::max();
    std::vector<Capsule> best_candidate;
```

Replace the candidate selection block:

```cpp
            if (improves_volume || improves_ratio) {
                if (!found_improving || score < best_score) {
                    best_score = score;
                    best_candidate = std::move(candidate);
                    found_improving = true;
                    found = true;
                }
            } else if (volume_pressure &&
                       after_metrics.capV_aabb <= max_capv_aabb_ratio * 1.10) {
                // ponytail: no candidate improves capV (common for near-uniform links
                // where splitting adds spherical endcap volume). Accept the least-bad
                // split capped at max_capv_aabb_ratio*1.10 to prevent cascade splits.
                if (!found_improving && (!found || score < best_managing_score)) {
                    best_managing_score = score;
                    best_managing_candidate = std::move(candidate);
                    found = true;
                }
            }
```

with:

```cpp
            if (improves_volume || improves_ratio) {
                if (!found_improving || score < best_score) {
                    best_score = score;
                    best_candidate = std::move(candidate);
                    found_improving = true;
                }
            }
```

Replace the function tail:

```cpp
    if (found_improving) {
        caps = std::move(best_candidate);
        return true;
    }
    if (found) {
        caps = std::move(best_managing_candidate);
        return true;
    }
    return false;
```

with:

```cpp
    if (!found_improving) return false;
    caps = std::move(best_candidate);
    return true;
```

- [ ] **Step 2: Restore a tighter comparison ceiling**

In `scripts/compare_capsule_presets.py`, change:

```python
    ap.add_argument("--max-capv-aabb", type=float, default=2.50,
```

to:

```python
    ap.add_argument("--max-capv-aabb", type=float, default=2.35,
```

- [ ] **Step 3: Keep tight config pressure strict**

Keep these lines in `config/capsule/capsuleConfig_tight.yml`:

```yaml
MaxCapVAabbRatio: 2.25
MinSplitVolumeImprovement: 0.005
```

Do not loosen them in this task. If the new endpoint optimizer makes the trigger unnecessary, remove `MaxCapVAabbRatio` in a separate reviewed change after the gates pass without it.

- [ ] **Step 4: Run FR3 sparse/tight generation**

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sparse_endpoint_fix.urdf --config config/capsule/capsuleConfig.yml'
```

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_endpoint_fix.urdf --config config/capsule/capsuleConfig_tight.yml'
```

Expected: both commands exit 0.

- [ ] **Step 5: Run gates**

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_endpoint_fix.json --max-capv-aabb 2.35 --max-r-binmed 1.45 --link-min-capsules fr3_link0=2'
```

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/compare_capsule_presets.py --sparse-json /tmp/fr3_sparse_endpoint_fix.json --tight-json /tmp/fr3_tight_endpoint_fix.json'
```

Expected: both commands exit 0. If `fr3_link0` no longer has 2 capsules because non-improving splits were removed, stop and report the metrics; do not reintroduce a non-improving split just to satisfy capsule count.

- [ ] **Step 6: Commit**

```bash
git add src/CapsuleCrossSection.cpp scripts/compare_capsule_presets.py config/capsule/capsuleConfig_tight.yml
git commit -m "fix: require splits to improve capsule tightness"
```

---

### Task 6: Make Axial Overhang Visible in Diagnostics

**Files:**
- Modify: `scripts/check_capsule_coverage.py`
- Modify: `scripts/check_capsule_tightness.py`

- [ ] **Step 1: Add overhang metric helper**

In `scripts/check_capsule_coverage.py`, insert this function after `tightness_metrics()`:

```python
def axis_overhang_metrics(V, capsules, assigned):
    worst_ratio = 0.0
    worst_abs = 0.0
    for idx, (p0, p1, radius) in enumerate(capsules):
        mask = assigned == idx
        if not np.any(mask):
            continue
        axis = p1 - p0
        length = float(np.linalg.norm(axis))
        if length < 1e-12 or radius <= 1e-12:
            continue
        unit = axis / length
        vertex_projection = V[mask] @ unit
        endpoint_projection = np.array([p0 @ unit, p1 @ unit], dtype=float)
        low_gap = float(endpoint_projection.min() - vertex_projection.min())
        high_gap = float(vertex_projection.max() - endpoint_projection.max())
        low_overhang = max(0.0, radius - low_gap)
        high_overhang = max(0.0, radius - high_gap)
        capsule_worst = max(low_overhang, high_overhang)
        worst_abs = max(worst_abs, capsule_worst)
        worst_ratio = max(worst_ratio, capsule_worst / radius)
    return worst_abs, worst_ratio
```

- [ ] **Step 2: Include metrics in JSON rows**

In `evaluate_capsules()`, after:

```python
        inflation, radius_ratio = tightness_metrics(V, capsules, assigned)
```

add:

```python
        axis_overhang, axis_overhang_ratio = axis_overhang_metrics(V, capsules, assigned)
```

In the row dictionary, add:

```python
            "axis_overhang": float(axis_overhang),
            "axis_overhang_r": float(axis_overhang_ratio),
```

- [ ] **Step 3: Include metrics in human-readable output**

Replace the header fragment:

```python
          f"{'maxd':>8} {'capV/aabb':>9} {'r/binMed':>8} | "
```

with:

```python
          f"{'maxd':>8} {'capV/aabb':>9} {'r/binMed':>8} {'over/r':>8} | "
```

Replace the print fragment:

```python
              f"{row['capV_aabb']:9.4f} {row['r_binMed']:8.2f} | "
```

with:

```python
              f"{row['capV_aabb']:9.4f} {row['r_binMed']:8.2f} {row['axis_overhang_r']:8.2f} | "
```

- [ ] **Step 4: Add optional tightness enforcement**

In `scripts/check_capsule_tightness.py`, add this argument next to the other per-link arguments:

```python
    parser.add_argument("--link-max-axis-overhang-r", action="append", default=[],
                        help="per-link max assigned axial overhang divided by radius as LINK=VALUE")
```

After parsing `link_min_capsules`, add:

```python
    link_max_overhang = parse_link_limits(args.link_max_axis_overhang_r, float, "--link-max-axis-overhang-r")
```

In the loop over rows, add:

```python
        max_overhang = link_max_overhang.get(link)
        if max_overhang is not None and row.get("axis_overhang_r", 0.0) > max_overhang:
            failures.append(f"{link}: axis_overhang/r {row['axis_overhang_r']:.2f} > {max_overhang:.2f}")
```

Update the requested-link set:

```python
    requested_links = set(link_max_capv) | set(link_max_ratio) | set(link_min_capsules) | set(link_max_overhang)
```

- [ ] **Step 5: Run diagnostics on fresh tight output**

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_coverage.py --caps-json /tmp/fr3_tight_endpoint_fix.json --json'
```

Expected: JSON rows include `axis_overhang` and `axis_overhang_r` for every link with capsules.

- [ ] **Step 6: Commit**

```bash
git add scripts/check_capsule_coverage.py scripts/check_capsule_tightness.py
git commit -m "test: report capsule axial overhang"
```

---

### Task 7: Regenerate Outputs and Update Handoff

**Files:**
- Modify: `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`
- Optional modify: `resources/fr3/urdf/fr3_capsuleized.json`
- Optional modify: `resources/fr3/urdf/fr3_capsuleized.urdf`

- [ ] **Step 1: Run full C++ tests**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_color=no'
```

Expected: all C++ tests PASS.

- [ ] **Step 2: Generate final sparse and tight outputs in `/tmp`**

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sparse_endpoint_final.urdf --config config/capsule/capsuleConfig.yml'
```

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_endpoint_final.urdf --config config/capsule/capsuleConfig_tight.yml'
```

Expected: both commands exit 0.

- [ ] **Step 3: Run final gates**

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_endpoint_final.json --max-capv-aabb 2.35 --max-r-binmed 1.45 --link-min-capsules fr3_link0=2'
```

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/compare_capsule_presets.py --sparse-json /tmp/fr3_sparse_endpoint_final.json --tight-json /tmp/fr3_tight_endpoint_final.json'
```

Expected: both commands exit 0. The comparison JSON must show tight preset improves sparse on `capV/aabb`, `r/binMed`, and capsule count.

- [ ] **Step 4: Update handoff text**

In `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`, update the current-state section to include:

```markdown
**Endpoint semantics:** `p0`/`p1` are capsule sphere centers, not mesh axial extrema. The fitter now optimizes endpoint spans after merge/split and grows radii only as needed for coverage, avoiding the previous extra half-sphere overhang at link ends.
```

Update the metrics JSON with the exact output from:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/compare_capsule_presets.py --sparse-json /tmp/fr3_sparse_endpoint_final.json --tight-json /tmp/fr3_tight_endpoint_final.json'
```

- [ ] **Step 5: Decide generated asset policy**

If this branch is supposed to commit regenerated FR3 examples, run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o resources/fr3/urdf/fr3_capsuleized.urdf --config config/capsule/capsuleConfig_tight.yml'
```

Then commit:

```bash
git add resources/fr3/urdf/fr3_capsuleized.json resources/fr3/urdf/fr3_capsuleized.urdf
git commit -m "assets: regenerate FR3 capsule output with endpoint centers"
```

If this branch should avoid generated assets, do not modify `resources/fr3/urdf/fr3_capsuleized.*` in this task; leave them out of commits and state that examples are generated via the commands above.

- [ ] **Step 6: Commit docs**

```bash
git add doc/handoffs/2026-07-02-capsule-geometry-handoff.md
git commit -m "docs: document capsule endpoint center semantics"
```

---

## Self-Review Notes

- Spec coverage: the plan directly addresses the visual issue by testing true capsule geometry, optimizing `p0/p1` as sphere centers, removing non-improving split acceptance, and adding overhang diagnostics.
- Placeholder scan: every code-edit step includes exact snippets and exact commands; there are no deferred implementation slots.
- Type consistency: C++ helpers remain private in `src/CapsuleCrossSection.cpp`; Python metric keys are `axis_overhang` and `axis_overhang_r`; CLI option is `--link-max-axis-overhang-r`.
- Risk: flat-ended cylinders cannot have zero axial overhang without radius growth. The plan tests a capsule-surface mesh for endpoint semantics and preserves coverage checks for flat cylinders, so it does not confuse unavoidable geometry with the current endpoint bug.
