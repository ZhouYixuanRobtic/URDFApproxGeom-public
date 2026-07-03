# Capsule Tightness Improvement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make capsuleized FR3 output both collision-safe and visually/geometrically tight, with COA/Lloyd configuration actually affecting the fitter.

**Architecture:** Add measurable FR3 tightness diagnostics first, then restore a controlled multi-circle cross-section path that keeps circle counts pairable across adjacent planes. Radius growth must remain coverage-safe, but it should trigger local splitting instead of inflating one long capsule across unrelated narrow regions.

**Tech Stack:** C++17, Eigen, yaml-cpp, GoogleTest, existing ManifoldPlus pipeline, Python 3 standard library + NumPy inside the `spherized-development` container.

---

## File Structure

- Modify: `scripts/check_capsule_coverage.py`
  - Fix multi-capsule coverage evaluation.
  - Add tightness metrics that do not require GUI review.
- Modify: `test/test_capsule.cpp`
  - Add unit tests proving config-driven multi-circle fitting changes output for wide shapes.
  - Add tests for local growth/splitting behavior.
- Modify: `include/CapsuleCrossSection.h`
  - Add small option/result structs only if needed to keep signatures readable.
- Modify: `src/CapsuleCrossSection.cpp`
  - Replace the hard-coded one-MEC-per-plane path with a configurable plane fitting mode.
  - Implement uniform circle-count fitting per plane and stable adjacent-plane matching.
  - Split or preserve local capsule segments when merged radius inflation exceeds threshold.
- Modify: `src/CapsuleURDFGenerator.cpp`
  - Read any new config values and pass them into the fitter.
- Modify: `config/capsule/capsuleConfig.yml`
  - Make the default behavior explicit and truthful.
- Create: `config/capsule/capsuleConfig_tight.yml`
  - Tight preset for FR3-quality output with more capsules.
- Modify: `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`
  - Update current-state claims after implementation.

## Current Evidence To Preserve

The current implementation passes the C++ tests but does not test tightness:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule'
```

Observed result before this plan: 15 tests pass.

The current fitter reads `CoaThreshold` and `MaxCirclesPerSection`, then ignores them in `src/CapsuleCrossSection.cpp`:

```cpp
(void)coa_threshold;
(void)max_circles_per_section;
```

The current diagnostic script incorrectly checks only `body["capsules"][0]`, which misreports any link that has more than one capsule.

---

### Task 1: Fix Coverage Diagnostics And Add Tightness Metrics

**Files:**
- Modify: `scripts/check_capsule_coverage.py`

- [ ] **Step 1: Write the failing diagnostic expectation**

Run the current script in the container:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_coverage.py'
```

Expected before implementation: output reports `hand_leftfinger False` and `hand_rightfinger False`, because the script checks only the first capsule per link.

- [ ] **Step 2: Replace single-capsule coverage with union-of-capsules coverage**

In `scripts/check_capsule_coverage.py`, replace the block that starts with:

```python
cp = body["capsules"][0]
p0L = np.array(cp["p0"]); p1L = np.array(cp["p1"]); r = float(cp["radius"])
```

with this exact helper and usage:

```python
def capsule_to_mesh_frame(cp, T, R):
    p0L = np.array(cp["p0"], dtype=float)
    p1L = np.array(cp["p1"], dtype=float)
    radius = float(cp["radius"])
    Rt = R.T
    return Rt @ (p0L - T), Rt @ (p1L - T), radius


def best_capsule_distance(vertex, capsules):
    best_signed = float("inf")
    best_raw = float("inf")
    best_index = -1
    for idx, (p0m, p1m, radius) in enumerate(capsules):
        raw = point_to_seg(vertex, p0m, p1m)
        signed = raw - radius
        if signed < best_signed:
            best_signed = signed
            best_raw = raw
            best_index = idx
    return best_signed, best_raw, best_index
```

Then compute coverage like this:

```python
capsules = [capsule_to_mesh_frame(cp, T, R) for cp in body["capsules"]]
signed = []
raw = []
assigned = []
for vertex in V:
    sdist, rdist, idx = best_capsule_distance(vertex, capsules)
    signed.append(sdist)
    raw.append(rdist)
    assigned.append(idx)
signed = np.array(signed, dtype=float)
raw = np.array(raw, dtype=float)
assigned = np.array(assigned, dtype=int)
worst = float(signed.max())
covered = worst <= 1e-6
maxd = float(raw.max())
```

- [ ] **Step 3: Add tightness metrics**

Add these helpers below `point_to_seg`:

```python
def point_to_seg_with_t(p, a, b):
    d = b - a
    denom = float(d @ d)
    t = 0.0 if denom < 1e-12 else float(np.clip((p - a) @ d / denom, 0.0, 1.0))
    q = a + t * d
    return float(np.linalg.norm(p - q)), t


def capsule_volume(p0, p1, radius):
    length = float(np.linalg.norm(p1 - p0))
    return math.pi * radius * radius * length + (4.0 / 3.0) * math.pi * radius ** 3


def tightness_metrics(V, capsules, assigned):
    cap_volume = sum(capsule_volume(p0, p1, radius) for p0, p1, radius in capsules)
    ext = V.max(axis=0) - V.min(axis=0)
    aabb_volume = float(np.prod(np.maximum(ext, 1e-12)))
    inflation = cap_volume / aabb_volume if aabb_volume > 1e-12 else 0.0

    worst_radius_ratio = 0.0
    for idx, (p0, p1, radius) in enumerate(capsules):
        mask = assigned == idx
        if not np.any(mask):
            continue
        bin_max = []
        for vertex in V[mask]:
            distance, t = point_to_seg_with_t(vertex, p0, p1)
            slot = min(9, max(0, int(t * 10.0)))
            while len(bin_max) <= slot:
                bin_max.append(0.0)
            bin_max[slot] = max(bin_max[slot], distance)
        nonzero = [v for v in bin_max if v > 1e-12]
        if nonzero:
            median_section_radius = float(np.median(nonzero))
            worst_radius_ratio = max(worst_radius_ratio, radius / median_section_radius)
    return inflation, worst_radius_ratio
```

Update the print header to include `caps`, `worst`, `capV/aabb`, and `r/binMed`:

```python
print(f"{'link':16} {'caps':>4} {'covered':8} {'worst':>9} {'radius':>8} "
      f"{'maxd':>8} {'capV/aabb':>9} {'r/binMed':>8} | "
      f"{'PCA axis (link)':26} {'bbox long axis':26}")
```

- [ ] **Step 4: Run the diagnostic and verify the original false negatives are gone**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_coverage.py'
```

Expected after implementation:

```text
hand_leftfinger  ... True ...
hand_rightfinger ... True ...
ALL COVERED: True
```

- [ ] **Step 5: Commit**

```bash
git add scripts/check_capsule_coverage.py
git commit -m "test: fix capsule coverage diagnostics"
```

---

### Task 2: Add Regression Tests For Config-Driven Multi-Circle Fitting

**Files:**
- Modify: `test/test_capsule.cpp`

- [ ] **Step 1: Add a wide-box mesh helper**

Add this helper near the existing synthetic mesh helpers:

```cpp
static void makeBox(double x, double y, double z, Eigen::MatrixXd& V, Eigen::MatrixXi& F) {
    V.resize(8, 3);
    V << -x/2, -y/2, -z/2,
          x/2, -y/2, -z/2,
          x/2,  y/2, -z/2,
         -x/2,  y/2, -z/2,
         -x/2, -y/2,  z/2,
          x/2, -y/2,  z/2,
          x/2,  y/2,  z/2,
         -x/2,  y/2,  z/2;
    F.resize(12, 3);
    F << 0, 1, 2, 0, 2, 3,
         4, 6, 5, 4, 7, 6,
         0, 4, 5, 0, 5, 1,
         1, 5, 6, 1, 6, 2,
         2, 6, 7, 2, 7, 3,
         3, 7, 4, 3, 4, 0;
}
```

- [ ] **Step 2: Add a failing test proving max circles changes output**

Add this test after `CapsuleXSectionFit.CylinderToOneCoveringCapsule`:

```cpp
TEST(CapsuleXSectionFit, WideBoxUsesMultipleCapsulesWhenAllowed) {
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeBox(1.0, 0.20, 0.20, V, F);

    auto sparse = fitCapsulesByCrossSection(V, F, 4, 0.005, 1, 12);
    auto tight = fitCapsulesByCrossSection(V, F, 4, 0.005, 4, 12);

    ASSERT_GE(sparse.size(), 1u);
    EXPECT_GT(tight.size(), sparse.size())
        << "MaxCirclesPerSection must affect the fitter for wide sections";

    double sparse_volume = 0.0;
    for (const auto& c : sparse) {
        const double L = (c.p1 - c.p0).norm();
        sparse_volume += M_PI * c.radius * c.radius * L + 4.0 * M_PI * c.radius * c.radius * c.radius / 3.0;
    }

    double tight_volume = 0.0;
    for (const auto& c : tight) {
        const double L = (c.p1 - c.p0).norm();
        tight_volume += M_PI * c.radius * c.radius * L + 4.0 * M_PI * c.radius * c.radius * c.radius / 3.0;
    }

    EXPECT_LT(tight_volume, 0.85 * sparse_volume)
        << "More circles should reduce over-cover volume on a wide box";
}
```

- [ ] **Step 3: Run the test and verify it fails before implementation**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_filter=CapsuleXSectionFit.WideBoxUsesMultipleCapsulesWhenAllowed'
```

Expected before implementation: FAIL because `max_circles_per_section` is ignored and `tight.size()` equals `sparse.size()`.

- [ ] **Step 4: Commit the failing test**

```bash
git add test/test_capsule.cpp
git commit -m "test: capture capsule multi-circle config behavior"
```

---

### Task 3: Implement Uniform Multi-Circle Plane Fitting

**Files:**
- Modify: `include/CapsuleCrossSection.h`
- Modify: `src/CapsuleCrossSection.cpp`
- Test: `test/test_capsule.cpp`

- [ ] **Step 1: Add a plane circle fitting helper declaration**

In `include/CapsuleCrossSection.h`, add this declaration below `fitCirclesLloyd`:

```cpp
/// Fit a fixed number of covering circles to all contours in one section plane.
/// k==1 is the MEC fallback. k>1 uses deterministic k-means seeds followed by
/// MEC refits per cluster so every sampled contour point remains covered.
std::vector<Circle2D> fitFixedCountCirclesForPlane(const std::vector<Contour2D>& contours,
                                                   int k);
```

- [ ] **Step 2: Implement fixed-count circle fitting**

In `src/CapsuleCrossSection.cpp`, add this function after `fitCirclesLloyd`:

```cpp
std::vector<Circle2D> fitFixedCountCirclesForPlane(const std::vector<Contour2D>& contours,
                                                   int k) {
    std::vector<Eigen::Vector2d> pts;
    for (const auto& contour : contours) {
        auto sampled = sampleContour(contour);
        pts.insert(pts.end(), sampled.begin(), sampled.end());
    }
    std::vector<Circle2D> circles;
    if (pts.empty() || k <= 0) return circles;
    if (k == 1 || static_cast<int>(pts.size()) < k) {
        circles.push_back(mec2d(pts));
        return circles;
    }

    std::vector<Eigen::Vector2d> seeds;
    seeds.reserve(k);
    seeds.push_back(pts.front());
    while (static_cast<int>(seeds.size()) < k) {
        int best = 0;
        double best_dist = -1.0;
        for (int i = 0; i < static_cast<int>(pts.size()); ++i) {
            double nearest = std::numeric_limits<double>::max();
            for (const auto& seed : seeds) {
                nearest = std::min(nearest, (pts[i] - seed).squaredNorm());
            }
            if (nearest > best_dist) {
                best_dist = nearest;
                best = i;
            }
        }
        seeds.push_back(pts[best]);
    }

    std::vector<std::vector<Eigen::Vector2d>> groups(k);
    for (int iter = 0; iter < 8; ++iter) {
        for (auto& group : groups) group.clear();
        for (const auto& p : pts) {
            int best = 0;
            double best_dist = std::numeric_limits<double>::max();
            for (int i = 0; i < k; ++i) {
                double d = (p - seeds[i]).squaredNorm();
                if (d < best_dist) {
                    best_dist = d;
                    best = i;
                }
            }
            groups[best].push_back(p);
        }
        for (int i = 0; i < k; ++i) {
            if (groups[i].empty()) continue;
            Eigen::Vector2d mean = Eigen::Vector2d::Zero();
            for (const auto& p : groups[i]) mean += p;
            seeds[i] = mean / double(groups[i].size());
        }
    }

    for (int i = 0; i < k; ++i) {
        if (!groups[i].empty()) circles.push_back(mec2d(groups[i]));
    }
    while (static_cast<int>(circles.size()) < k && !circles.empty()) {
        circles.push_back(circles.back());
    }
    return circles;
}
```

- [ ] **Step 3: Use config to choose circle count**

In `fitCapsulesByCrossSection`, replace:

```cpp
std::vector<double> planeT;
std::vector<Circle2D> planeCircle;
for (auto& kv : planes) {
    std::vector<Eigen::Vector2d> allpts;
    for (auto& ct : kv.second) {
        auto p = sampleContour(ct);
        allpts.insert(allpts.end(), p.begin(), p.end());
    }
    if (allpts.empty()) continue;
    planeT.push_back(kv.first);
    planeCircle.push_back(mec2d(allpts));
}
int N = static_cast<int>(planeT.size());
if (N == 0) return caps;
(void)coa_threshold;
(void)max_circles_per_section;
```

with:

```cpp
std::vector<double> planeT;
std::vector<std::vector<Circle2D>> planeCircles;
int requested_k = std::max(1, max_circles_per_section);
if (coa_threshold <= 0.0) requested_k = 1;

for (auto& kv : planes) {
    auto circles = fitFixedCountCirclesForPlane(kv.second, requested_k);
    if (circles.empty()) continue;
    planeT.push_back(kv.first);
    planeCircles.push_back(std::move(circles));
}
int N = static_cast<int>(planeT.size());
if (N == 0) return caps;
```

- [ ] **Step 4: Replace single-chain emission with matched multi-chain emission**

Replace the `emit_pair` block and the `for (int k = 0; k < N - 1; ++k)` chain emission with:

```cpp
auto emit_pair = [&](const Circle2D& a, double ta, const Circle2D& b, double tb) {
    Capsule cap;
    cap.p0 = to3d(a, ta);
    cap.p1 = to3d(b, tb);
    cap.radius = std::max(a.radius, b.radius);
    caps.push_back(cap);
};

if (N == 1) {
    for (const auto& circle : planeCircles[0]) {
        emit_pair(circle, planeT[0], circle, planeT[0]);
    }
} else {
    for (int section = 0; section < N - 1; ++section) {
        const auto& A = planeCircles[section];
        const auto& B = planeCircles[section + 1];
        std::vector<char> used(B.size(), 0);
        for (const auto& a : A) {
            int best = -1;
            double best_dist = std::numeric_limits<double>::max();
            for (int j = 0; j < static_cast<int>(B.size()); ++j) {
                if (used[j]) continue;
                double d = (a.center - B[j].center).squaredNorm();
                if (d < best_dist) {
                    best_dist = d;
                    best = j;
                }
            }
            if (best < 0) best = 0;
            used[best] = 1;
            emit_pair(a, planeT[section], B[best], planeT[section + 1]);
        }
    }
}
```

- [ ] **Step 5: Update end extension for multiple circles**

Replace:

```cpp
double t0 = planeT.front(), tN = planeT.back();
for (auto& cap : caps) {
    double a0 = (cap.p0 - origin).dot(u);
    double a1 = (cap.p1 - origin).dot(u);
    if (std::abs(a0 - t0) < 1e-9) cap.p0 += (amin - t0) * u;
    if (std::abs(a1 - tN) < 1e-9) cap.p1 += (amax - tN) * u;
}
```

with:

```cpp
double t0 = planeT.front(), tN = planeT.back();
for (auto& cap : caps) {
    double a0 = (cap.p0 - origin).dot(u);
    double a1 = (cap.p1 - origin).dot(u);
    if (std::abs(a0 - t0) < 1e-9) cap.p0 += (amin - t0) * u;
    if (std::abs(a1 - t0) < 1e-9) cap.p1 += (amin - t0) * u;
    if (std::abs(a0 - tN) < 1e-9) cap.p0 += (amax - tN) * u;
    if (std::abs(a1 - tN) < 1e-9) cap.p1 += (amax - tN) * u;
}
```

- [ ] **Step 6: Run the regression test**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_filter=CapsuleXSectionFit.WideBoxUsesMultipleCapsulesWhenAllowed'
```

Expected after implementation: PASS.

- [ ] **Step 7: Run all C++ capsule tests**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule'
```

Expected: all tests pass.

- [ ] **Step 8: Commit**

```bash
git add include/CapsuleCrossSection.h src/CapsuleCrossSection.cpp test/test_capsule.cpp
git commit -m "feat: enable config-driven multi-circle capsule fitting"
```

---

### Task 4: Prevent Radius Inflation From Over-Merging Local Segments

**Files:**
- Modify: `src/CapsuleCrossSection.cpp`
- Modify: `test/test_capsule.cpp`

- [ ] **Step 1: Add a test for local bulge preservation**

Add this helper in `test/test_capsule.cpp` near `makeBox`:

```cpp
static void makeTwoBoxLink(Eigen::MatrixXd& V, Eigen::MatrixXi& F) {
    Eigen::MatrixXd A, B;
    Eigen::MatrixXi FA, FB;
    makeBox(0.20, 0.20, 0.20, A, FA);
    makeBox(0.20, 0.60, 0.20, B, FB);
    for (int i = 0; i < A.rows(); ++i) A(i, 0) -= 0.20;
    for (int i = 0; i < B.rows(); ++i) B(i, 0) += 0.20;

    V.resize(A.rows() + B.rows(), 3);
    V << A, B;
    F.resize(FA.rows() + FB.rows(), 3);
    F.topRows(FA.rows()) = FA;
    F.bottomRows(FB.rows()) = FB.array() + A.rows();
}
```

Add this test:

```cpp
TEST(CapsuleXSectionFit, LocalBulgeDoesNotInflateWholeLink) {
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeTwoBoxLink(V, F);

    auto caps = fitCapsulesByCrossSection(V, F, 6, 0.005, 2, 12);
    ASSERT_GE(caps.size(), 2u);

    double smallest_radius = std::numeric_limits<double>::max();
    double largest_radius = 0.0;
    for (const auto& cap : caps) {
        smallest_radius = std::min(smallest_radius, cap.radius);
        largest_radius = std::max(largest_radius, cap.radius);
    }
    EXPECT_LT(smallest_radius, 0.75 * largest_radius)
        << "A narrow section should keep a smaller capsule instead of inheriting the bulge radius";
}
```

- [ ] **Step 2: Run the test and verify the current merge/grow behavior fails if it over-merges**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_filter=CapsuleXSectionFit.LocalBulgeDoesNotInflateWholeLink'
```

Expected before implementation: FAIL if `mergeCollinearCapsules` collapses local segments into one large-radius capsule.

- [ ] **Step 3: Make merge reject large local radius jumps**

In `mergeCollinearCapsules`, tighten this condition:

```cpp
if (std::abs(A.radius - B.radius) > 0.30 * std::max(A.radius, B.radius)) continue;
```

to:

```cpp
if (std::abs(A.radius - B.radius) > 0.15 * std::max(A.radius, B.radius)) continue;
```

Then add a span inflation guard before assigning `merged`:

```cpp
double merged_radius = std::max(A.radius, B.radius);
double min_radius = std::min(A.radius, B.radius);
if (min_radius > 1e-12 && merged_radius / min_radius > 1.35) continue;
Capsule merged{A.p0, B.p1, merged_radius};
```

- [ ] **Step 4: Run local bulge test**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_filter=CapsuleXSectionFit.LocalBulgeDoesNotInflateWholeLink'
```

Expected after implementation: PASS.

- [ ] **Step 5: Run all capsule tests**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule'
```

Expected: all tests pass.

- [ ] **Step 6: Commit**

```bash
git add src/CapsuleCrossSection.cpp test/test_capsule.cpp
git commit -m "fix: preserve local capsule radii during merge"
```

---

### Task 5: Make Configuration Truthful And Add Sparse/Tight Presets

**Files:**
- Modify: `src/CapsuleURDFGenerator.cpp`
- Modify: `include/CapsuleCrossSection.h`
- Modify: `config/capsule/capsuleConfig.yml`
- Create: `config/capsule/capsuleConfig_tight.yml`

- [ ] **Step 1: Update default config comments**

Replace `config/capsule/capsuleConfig.yml` with:

```yaml
# Capsule approximation config.
# Sparse default: few URDF-native primitives, collision-safe outer cover.
# Set MaxCirclesPerSection > 1 to enable tighter multi-circle sections.

NSections: 4
CoaThreshold: 0.005
MaxCirclesPerSection: 1
MaxCapsulesPerLink: 12
```

- [ ] **Step 2: Add tight preset**

Create `config/capsule/capsuleConfig_tight.yml`:

```yaml
# Tight capsule approximation preset.
# More section circles and more retained capsules reduce visual over-cover.

NSections: 6
CoaThreshold: 0.005
MaxCirclesPerSection: 4
MaxCapsulesPerLink: 24
```

- [ ] **Step 3: Confirm generator already forwards config values**

Check `src/CapsuleURDFGenerator.cpp` still contains:

```cpp
n_sections_ = doc["NSections"].as<int>(n_sections_);
coa_threshold_ = doc["CoaThreshold"].as<double>(coa_threshold_);
max_circles_per_section_ = doc["MaxCirclesPerSection"].as<int>(max_circles_per_section_);
max_capsules_ = doc["MaxCapsulesPerLink"].as<int>(max_capsules_);
```

and:

```cpp
auto caps = urdf_approx_geom::fitCapsulesByCrossSection(
    Vlf, OUT_F, n_sections_, coa_threshold_, max_circles_per_section_, max_capsules_);
```

No code change is needed in this task if those lines remain present.

- [ ] **Step 4: Generate FR3 with sparse config**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sparse_capsuleized.urdf --config config/capsule/capsuleConfig.yml'
```

Expected: command exits 0 and writes `/tmp/fr3_sparse_capsuleized.json`.

- [ ] **Step 5: Generate FR3 with tight config**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_capsuleized.urdf --config config/capsule/capsuleConfig_tight.yml'
```

Expected: command exits 0 and writes `/tmp/fr3_tight_capsuleized.json`.

- [ ] **Step 6: Compare capsule counts**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 - <<'"'"'PY'"'"'
import json
for label, path in [("sparse", "/tmp/fr3_sparse_capsuleized.json"), ("tight", "/tmp/fr3_tight_capsuleized.json")]:
    data = json.load(open(path))
    count = sum(len(body.get("capsules", [])) for body in data.values())
    print(label, count)
PY'
```

Expected: tight count is greater than sparse count and less than or equal to `24 * number_of_mesh_links`.

- [ ] **Step 7: Commit**

```bash
git add config/capsule/capsuleConfig.yml config/capsule/capsuleConfig_tight.yml
git commit -m "config: add sparse and tight capsule presets"
```

---

### Task 6: Add FR3 Tightness Regression Gate

**Files:**
- Modify: `scripts/check_capsule_coverage.py`
- Create: `scripts/check_capsule_tightness.py`

- [ ] **Step 1: Create a non-GUI tightness gate**

Create `scripts/check_capsule_tightness.py`:

```python
#!/usr/bin/env python3
import argparse
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--max-capv-aabb", type=float, default=2.10)
    parser.add_argument("--max-r-binmed", type=float, default=1.45)
    args = parser.parse_args()

    proc = subprocess.run(
        [sys.executable, "scripts/check_capsule_coverage.py"],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    print(proc.stdout)
    if proc.returncode != 0:
        return proc.returncode
    if "ALL COVERED: True" not in proc.stdout:
        print("coverage gate failed: ALL COVERED was not True", file=sys.stderr)
        return 1

    failures = []
    for line in proc.stdout.splitlines():
        parts = line.split()
        if len(parts) < 8 or parts[0] in {"link", "ALL"}:
            continue
        link = parts[0]
        try:
            capv_aabb = float(parts[6])
            r_binmed = float(parts[7])
        except ValueError:
            continue
        if capv_aabb > args.max_capv_aabb:
            failures.append(f"{link}: capV/aabb {capv_aabb:.2f} > {args.max_capv_aabb:.2f}")
        if r_binmed > args.max_r_binmed:
            failures.append(f"{link}: r/binMed {r_binmed:.2f} > {args.max_r_binmed:.2f}")

    if failures:
        print("tightness gate failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 2: Make the script executable**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && chmod +x scripts/check_capsule_tightness.py'
```

- [ ] **Step 3: Run the gate**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py'
```

Expected after algorithm improvements: PASS.

- [ ] **Step 4: Commit**

```bash
git add scripts/check_capsule_coverage.py scripts/check_capsule_tightness.py
git commit -m "test: add FR3 capsule tightness gate"
```

---

### Task 7: Update Handoff Documentation

**Files:**
- Modify: `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`

- [ ] **Step 1: Update current state**

Replace the current-state paragraph that says the active algorithm is “one MEC circle per plane” with:

```markdown
**Active algorithm:** Wu2018 cross-section decomposition with configurable uniform circle count per section plane. Sparse config uses 1 circle/section for low primitive count. Tight config uses up to 4 circles/section and stable adjacent-plane matching for better fit on flanged robot links.
```

- [ ] **Step 2: Update config section**

Replace the note saying `CoaThreshold` and `MaxCirclesPerSection` are unused with:

```markdown
- **`MaxCirclesPerSection`** controls the number of section circles. `1` gives sparse output. Values above `1` enable multi-circle fitting.
- **`CoaThreshold`** enables/disables circle splitting behavior. Values `<= 0` force sparse one-circle fitting.
- **`config/capsule/capsuleConfig.yml`** is the sparse default.
- **`config/capsule/capsuleConfig_tight.yml`** is the tighter FR3-oriented preset.
```

- [ ] **Step 3: Add verification commands**

Add:

```markdown
# Coverage and tightness gate
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py'
```

- [ ] **Step 4: Commit**

```bash
git add doc/handoffs/2026-07-02-capsule-geometry-handoff.md
git commit -m "docs: update capsule geometry handoff"
```

---

### Task 8: Final Verification

**Files:**
- No code changes.

- [ ] **Step 1: Build**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)'
```

Expected: build exits 0.

- [ ] **Step 2: Run C++ tests**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule'
```

Expected: all tests pass.

- [ ] **Step 3: Run Python package tests if the built `.so` exists**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cp build/python/urdf_approx_geom/*.so python/urdf_approx_geom/ && PYTHONPATH=/workspace/python python3 -m pytest python/tests -q'
```

Expected: tests pass. If `pytest` is missing in the container, record the exact missing dependency and do not claim Python tests pass.

- [ ] **Step 4: Regenerate FR3 tight output**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o resources/fr3/urdf/fr3_capsuleized.urdf --config config/capsule/capsuleConfig_tight.yml'
```

Expected: command exits 0 and writes `resources/fr3/urdf/fr3_capsuleized.urdf` plus `resources/fr3/urdf/fr3_capsuleized.json`.

- [ ] **Step 5: Run tightness gate**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py'
```

Expected: PASS.

- [ ] **Step 6: Check git status**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git status --short'
```

Expected: only intentional source, config, docs, script, and generated FR3 output changes remain.

---

## Self-Review

**Spec coverage:** This plan addresses the observed failures: ignored config, coverage-only validation, false-negative multi-capsule diagnostic, over-merged local radii, missing sparse/tight presets, and stale handoff docs.

**Placeholder scan:** No task uses unresolved placeholder language. Each code-changing task includes concrete paths, code snippets, commands, and expected outcomes.

**Type consistency:** New helper names are consistent across tasks: `fitFixedCountCirclesForPlane`, `point_to_seg_with_t`, `capsule_volume`, `tightness_metrics`, and `check_capsule_tightness.py`.
