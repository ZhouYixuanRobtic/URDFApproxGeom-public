# Capsule Tightness Review Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix the reviewed capsule-tightness implementation so the generated FR3 tight preset is explicitly evaluated, coverage-safe, and passes the tightness gate without weakening the gate to match a bad result.

**Architecture:** Stabilize diagnostics first so every later algorithm change has a trustworthy target. Then replace fixed-`k` multi-circle fitting with adaptive per-plane fitting, add local axial splitting for inflated capsules, and make budget trimming coverage-aware. Finally regenerate and document sparse/tight outputs with commands that read the intended JSON file.

**Tech Stack:** C++17, Eigen, yaml-cpp, GoogleTest, existing ManifoldPlus pipeline, Python 3 scripts run inside `spherized-development`.

---

## Review Findings This Plan Fixes

- `scripts/check_capsule_tightness.py` fails on current `resources/fr3/urdf/fr3_capsuleized.json`.
- Tight preset output is currently worse under the same metrics: many links exceed `capV/aabb` and `r/binMed` thresholds.
- The tightness gate hardcodes `resources/fr3/urdf/fr3_capsuleized.json`, so it can accidentally evaluate sparse output after C++ integration tests rewrite that file.
- `CoaThreshold` only disables fitting when `<= 0`; positive values do not drive circle-count choice.
- The wide-box test was relaxed from `0.85` to `0.915`, which codifies the weak result instead of catching it.
- Greedy adjacent matching is only local and fixed-`k` creates unnecessary capsules on narrow planes.
- `MaxCapsulesPerLink` pruning deletes shortest capsules after grow without re-growing or re-checking coverage.
- Handoff docs still contain stale 13-capsule / unused-config statements.

## File Structure

- Modify: `scripts/check_capsule_coverage.py`
  - Add `--caps-json`, `--urdf`, and `--json` arguments.
  - Return machine-readable metrics instead of forcing callers to parse aligned text.
- Modify: `scripts/check_capsule_tightness.py`
  - Accept `--caps-json` and call coverage diagnostics in JSON mode.
  - Fail fast if the requested JSON does not exist.
- Modify: `include/CapsuleCrossSection.h`
  - Add `CapsuleFitOptions` and `CapsuleFitStats`.
  - Keep the existing `fitCapsulesByCrossSection(...)` overload as a wrapper.
- Modify: `src/CapsuleCrossSection.cpp`
  - Add adaptive per-plane circle fitting using `CoaThreshold`.
  - Add variable-count adjacent matching.
  - Add local axial splitting for inflated capsules.
  - Make max-capsule pruning coverage-aware.
- Modify: `include/CapsuleURDFGenerator.h`
  - Add config fields for adaptive fitting and local split ratio.
- Modify: `src/CapsuleURDFGenerator.cpp`
  - Read new config keys and pass `CapsuleFitOptions`.
- Modify: `test/test_capsule.cpp`
  - Add regression tests for threshold behavior, local split quality, and coverage after pruning.
  - Restore the wide-box volume threshold to `0.85`.
- Modify: `config/capsule/capsuleConfig.yml`
  - Keep sparse default explicit.
- Modify: `config/capsule/capsuleConfig_tight.yml`
  - Use adaptive circle count and local split parameters.
- Modify: `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`
  - Remove stale statements and record actual sparse/tight verification commands.

Do not commit generated build-system files under `build/`, `scripts/__pycache__/`, or temporary `/tmp` outputs.

---

### Task 1: Make Diagnostics Path-Aware And Machine-Readable

**Files:**
- Modify: `scripts/check_capsule_coverage.py`
- Modify: `scripts/check_capsule_tightness.py`

- [ ] **Step 1: Prove the current tightness gate evaluates the wrong fixed path**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_capsuleized.json'
```

Expected before implementation:

```text
usage: check_capsule_tightness.py ...
error: unrecognized arguments: --caps-json /tmp/fr3_tight_capsuleized.json
```

- [ ] **Step 2: Add argparse and an evaluatable API to `check_capsule_coverage.py`**

Replace the constants and `main()` shape with this structure. Keep the existing helper functions (`rpy_to_R`, `parse_collisions`, `point_to_seg`, `tightness_metrics`, `capsule_to_mesh_frame`, `best_capsule_distance`) and route through `evaluate_capsules`.

```python
def evaluate_capsules(caps_json, urdf_path):
    caps = json.load(open(caps_json))
    cols = parse_collisions(urdf_path)
    rows = []
    all_ok = True
    for link, body in caps.items():
        if link not in cols or "capsules" not in body:
            continue
        T, R, fn = cols[link]
        if fn is None:
            continue
        stl = fn.replace(MESH_PREFIX, os.path.join(REPO, "resources/fr3"))
        V = trimesh.load(stl).vertices
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
        all_ok &= covered
        inflation, radius_ratio = tightness_metrics(V, capsules, assigned)

        p0L = np.array(body["capsules"][0]["p0"], dtype=float)
        p1L = np.array(body["capsules"][0]["p1"], dtype=float)
        cap_axis = p1L - p0L
        seg_len = float(np.linalg.norm(cap_axis))
        cap_axis = cap_axis / seg_len if seg_len > 1e-12 else np.zeros(3)
        ext = V.max(axis=0) - V.min(axis=0)
        long_axis = np.zeros(3)
        long_axis[int(np.argmax(ext))] = 1.0
        align = abs(float(cap_axis @ long_axis))

        rows.append({
            "link": link,
            "capsules": len(body["capsules"]),
            "covered": bool(covered),
            "worst": worst,
            "radius": float(max(c[2] for c in capsules)),
            "maxd": float(raw.max()),
            "capV_aabb": float(inflation),
            "r_binMed": float(radius_ratio),
            "axis": [float(cap_axis[0]), float(cap_axis[1]), float(cap_axis[2])],
            "axis_length": seg_len,
            "bbox_long_axis": [int(long_axis[0]), int(long_axis[1]), int(long_axis[2])],
            "axis_bbox_align": align,
        })
    return {"all_covered": bool(all_ok), "links": rows}
```

Add this `main()`:

```python
def main():
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("--caps-json", default=CAPS_JSON)
    ap.add_argument("--urdf", default=FR3_URDF)
    ap.add_argument("--json", action="store_true", help="emit machine-readable metrics")
    args = ap.parse_args()

    result = evaluate_capsules(args.caps_json, args.urdf)
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
        return

    print(f"{'link':16} {'caps':>4} {'covered':8} {'worst':>9} {'radius':>8} "
          f"{'maxd':>8} {'capV/aabb':>9} {'r/binMed':>8} | "
          f"{'PCA axis (link)':26} {'bbox long axis':26}")
    for row in result["links"]:
        axis = row["axis"]
        bbox = row["bbox_long_axis"]
        print(f"{row['link']:16} {row['capsules']:4} {str(row['covered']):8} "
              f"{row['worst']:9.6f} {row['radius']:8.4f} {row['maxd']:8.4f} "
              f"{row['capV_aabb']:9.4f} {row['r_binMed']:8.2f} | "
              f"axis=[{axis[0]:+.2f} {axis[1]:+.2f} {axis[2]:+.2f}] "
              f"len={row['axis_length']:.3f} bbox_long=[{bbox[0]} {bbox[1]} {bbox[2]}] "
              f"align={row['axis_bbox_align']:.2f}")
    print(f"\nALL COVERED: {result['all_covered']}")
```

- [ ] **Step 3: Rewrite `check_capsule_tightness.py` to consume JSON metrics**

Replace its subprocess/text parsing with:

```python
#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--caps-json", default="resources/fr3/urdf/fr3_capsuleized.json")
    parser.add_argument("--urdf", default="resources/fr3/urdf/fr3.urdf")
    parser.add_argument("--max-capv-aabb", type=float, default=2.10)
    parser.add_argument("--max-r-binmed", type=float, default=1.45)
    parser.add_argument("--max-capsules", type=int, default=24)
    args = parser.parse_args()

    if not os.path.exists(args.caps_json):
        print(f"capsule json does not exist: {args.caps_json}", file=sys.stderr)
        return 2

    proc = subprocess.run(
        [
            sys.executable,
            "scripts/check_capsule_coverage.py",
            "--caps-json", args.caps_json,
            "--urdf", args.urdf,
            "--json",
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if proc.returncode != 0:
        print(proc.stdout)
        return proc.returncode

    result = json.loads(proc.stdout)
    print(json.dumps(result, indent=2, sort_keys=True))
    if not result["all_covered"]:
        print("coverage gate failed: not all links are covered", file=sys.stderr)
        return 1

    failures = []
    for row in result["links"]:
        if row["capsules"] > args.max_capsules:
            failures.append(f"{row['link']}: capsules {row['capsules']} > {args.max_capsules}")
        if row["capV_aabb"] > args.max_capv_aabb:
            failures.append(f"{row['link']}: capV/aabb {row['capV_aabb']:.2f} > {args.max_capv_aabb:.2f}")
        if row["r_binMed"] > args.max_r_binmed:
            failures.append(f"{row['link']}: r/binMed {row['r_binMed']:.2f} > {args.max_r_binmed:.2f}")

    if failures:
        print("tightness gate failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 4: Run diagnostics against the existing sparse file**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_coverage.py --json >/tmp/caps_metrics.json && python3 -m json.tool /tmp/caps_metrics.json >/dev/null'
```

Expected after implementation: command exits 0.

- [ ] **Step 5: Verify `--caps-json` is now honored**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/does-not-exist.json'
```

Expected after implementation:

```text
capsule json does not exist: /tmp/does-not-exist.json
```

Exit code: `2`.

- [ ] **Step 6: Commit diagnostics fix**

```bash
git add scripts/check_capsule_coverage.py scripts/check_capsule_tightness.py
git commit -m "test: make capsule tightness gate evaluate explicit outputs"
```

---

### Task 2: Add Regression Tests For Review Failures

**Files:**
- Modify: `test/test_capsule.cpp`

- [ ] **Step 1: Add shared test helpers**

Add these helpers near the existing capsule volume code in `test/test_capsule.cpp`:

```cpp
static double capsuleVolume(const Capsule& c) {
    const double L = (c.p1 - c.p0).norm();
    return M_PI * c.radius * c.radius * L + 4.0 * M_PI * c.radius * c.radius * c.radius / 3.0;
}

static double capsuleSetVolume(const std::vector<Capsule>& caps) {
    double v = 0.0;
    for (const auto& c : caps) v += capsuleVolume(c);
    return v;
}

static bool allVerticesCoveredByAnyCapsule(const Eigen::MatrixXd& V,
                                           const std::vector<Capsule>& caps,
                                           double eps = 1e-9) {
    for (int i = 0; i < V.rows(); ++i) {
        bool covered = false;
        Eigen::Vector3d p = V.row(i).transpose();
        for (const auto& c : caps) {
            if (pointToSegmentDistance(p, c.p0, c.p1) <= c.radius + eps) {
                covered = true;
                break;
            }
        }
        if (!covered) return false;
    }
    return true;
}

static double maxRadiusToMedianBinRatio(const Eigen::MatrixXd& V,
                                        const std::vector<Capsule>& caps) {
    double worst = 0.0;
    for (const auto& cap : caps) {
        Eigen::Vector3d axis = cap.p1 - cap.p0;
        double denom = axis.squaredNorm();
        std::vector<double> bins(10, 0.0);
        for (int i = 0; i < V.rows(); ++i) {
            Eigen::Vector3d p = V.row(i).transpose();
            double t = denom < 1e-12 ? 0.0 : std::clamp((p - cap.p0).dot(axis) / denom, 0.0, 1.0);
            int slot = std::min(9, std::max(0, static_cast<int>(t * 10.0)));
            bins[slot] = std::max(bins[slot], pointToSegmentDistance(p, cap.p0, cap.p1));
        }
        std::vector<double> nonzero;
        for (double b : bins)
            if (b > 1e-12) nonzero.push_back(b);
        if (nonzero.empty()) continue;
        std::sort(nonzero.begin(), nonzero.end());
        double median = nonzero[nonzero.size() / 2];
        worst = std::max(worst, cap.radius / median);
    }
    return worst;
}
```

- [ ] **Step 2: Restore the wide-box threshold**

Change:

```cpp
EXPECT_LT(tight_volume, 0.915 * sparse_volume)
```

to:

```cpp
EXPECT_LT(tight_volume, 0.85 * sparse_volume)
```

Remove the comment that says `0.85` is a future improvement.

- [ ] **Step 3: Add a threshold behavior regression test**

Add:

```cpp
TEST(CapsuleXSectionFit, CoaThresholdControlsCircleCount) {
    Contour2D c;
    c.points = {{-0.5, -0.1}, {0.5, -0.1}, {0.5, 0.1}, {-0.5, 0.1}};
    std::vector<Contour2D> contours{c};

    auto sparse = fitAdaptiveCirclesForPlane(contours, 10.0, 4);
    auto tight = fitAdaptiveCirclesForPlane(contours, 0.005, 4);

    ASSERT_EQ(sparse.size(), 1u);
    EXPECT_GT(tight.size(), sparse.size());
    EXPECT_LE(tight.size(), 4u);
}
```

This test requires Task 3 to expose `fitAdaptiveCirclesForPlane`.

- [ ] **Step 4: Add a local inflation regression test**

Add:

```cpp
TEST(CapsuleXSectionFit, LocalSplitReducesRadiusBinInflation) {
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeTwoBoxLink(V, F);

    CapsuleFitOptions opts;
    opts.n_sections = 6;
    opts.coa_threshold = 0.005;
    opts.max_circles_per_section = 3;
    opts.max_capsules = 12;
    opts.max_radius_bin_ratio = 1.45;
    opts.adaptive_circle_count = true;

    auto caps = fitCapsulesByCrossSection(V, F, opts);
    ASSERT_TRUE(allVerticesCoveredByAnyCapsule(V, caps));
    EXPECT_LE(maxRadiusToMedianBinRatio(V, caps), 1.45);
}
```

- [ ] **Step 5: Add a budget pruning regression test**

Add:

```cpp
TEST(CapsuleXSectionFit, BudgetPruningPreservesCoverage) {
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeTwoBoxLink(V, F);

    CapsuleFitOptions opts;
    opts.n_sections = 6;
    opts.coa_threshold = 0.005;
    opts.max_circles_per_section = 4;
    opts.max_capsules = 2;
    opts.max_radius_bin_ratio = 1.45;
    opts.adaptive_circle_count = true;

    auto caps = fitCapsulesByCrossSection(V, F, opts);
    ASSERT_LE(caps.size(), 2u);
    EXPECT_TRUE(allVerticesCoveredByAnyCapsule(V, caps));
}
```

- [ ] **Step 6: Run the new tests and verify they fail before algorithm work**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_filter=CapsuleXSectionFit.WideBoxUsesMultipleCapsulesWhenAllowed:CapsuleXSectionFit.CoaThresholdControlsCircleCount:CapsuleXSectionFit.LocalSplitReducesRadiusBinInflation:CapsuleXSectionFit.BudgetPruningPreservesCoverage'
```

Expected before Tasks 3-5 are implemented: build fails because `CapsuleFitOptions` and `fitAdaptiveCirclesForPlane` are not defined, or tests fail under the current fixed-`k` implementation.

- [ ] **Step 7: Commit failing regression tests**

```bash
git add test/test_capsule.cpp
git commit -m "test: capture capsule tightness review regressions"
```

---

### Task 3: Introduce Fit Options And Adaptive Plane Circle Fitting

**Files:**
- Modify: `include/CapsuleCrossSection.h`
- Modify: `src/CapsuleCrossSection.cpp`

- [ ] **Step 1: Add option and stats structs**

In `include/CapsuleCrossSection.h`, add before `fitCapsulesByCrossSection`:

```cpp
struct CapsuleFitOptions {
    int n_sections = 10;
    double coa_threshold = 1e-4;
    int max_circles_per_section = 4;
    int max_capsules = 6;
    double max_radius_bin_ratio = 1.45;
    bool adaptive_circle_count = true;
};

struct CapsuleFitStats {
    double cap_volume = 0.0;
    double aabb_volume = 0.0;
    double capV_aabb = 0.0;
    double max_radius_bin_ratio = 0.0;
    bool covered = false;
};

std::vector<Circle2D> fitAdaptiveCirclesForPlane(const std::vector<Contour2D>& contours,
                                                 double coa_threshold,
                                                 int max_circles);

std::vector<Capsule> fitCapsulesByCrossSection(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                               const CapsuleFitOptions& options);
```

Keep the existing overload declaration:

```cpp
std::vector<Capsule> fitCapsulesByCrossSection(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                               int n_sections = 10, double coa_threshold = 1e-4,
                                               int max_circles_per_section = 4,
                                               int max_capsules = 6);
```

- [ ] **Step 2: Add polygon area helper**

In `src/CapsuleCrossSection.cpp`, add near `sampleContour`:

```cpp
double contourAreaAbs(const Contour2D& c) {
    if (c.points.size() < 3) return 0.0;
    double area = 0.0;
    int n = static_cast<int>(c.points.size());
    for (int i = 0; i < n; ++i) {
        const auto& a = c.points[i];
        const auto& b = c.points[(i + 1) % n];
        area += a.x() * b.y() - b.x() * a.y();
    }
    return 0.5 * std::abs(area);
}
```

- [ ] **Step 3: Implement COA-driven adaptive plane fitting**

Add after `fitFixedCountCirclesForPlane`:

```cpp
double normalizedPlaneOutsideArea(const std::vector<Circle2D>& circles,
                                  const std::vector<Contour2D>& contours) {
    double outside = 0.0;
    double area = 0.0;
    for (const auto& contour : contours) {
        area += contourAreaAbs(contour);
        for (const auto& circle : circles) {
            outside += circleOutsideArea(circle, contour);
        }
    }
    if (area < 1e-12) return outside;
    return outside / area;
}

std::vector<Circle2D> fitAdaptiveCirclesForPlane(const std::vector<Contour2D>& contours,
                                                 double coa_threshold,
                                                 int max_circles) {
    int cap = std::max(1, max_circles);
    if (coa_threshold <= 0.0) return fitFixedCountCirclesForPlane(contours, 1);

    std::vector<Circle2D> best = fitFixedCountCirclesForPlane(contours, 1);
    double best_score = normalizedPlaneOutsideArea(best, contours);
    if (best_score <= coa_threshold) return best;

    for (int k = 2; k <= cap; ++k) {
        auto candidate = fitFixedCountCirclesForPlane(contours, k);
        double score = normalizedPlaneOutsideArea(candidate, contours);
        if (score < best_score) {
            best = candidate;
            best_score = score;
        }
        if (score <= coa_threshold) return candidate;
    }
    return best;
}
```

- [ ] **Step 4: Use adaptive plane fitting in `fitCapsulesByCrossSection`**

Replace the fixed requested-k block:

```cpp
int requested_k = std::max(1, max_circles_per_section);
if (coa_threshold <= 0.0) requested_k = 1;

for (auto& kv : planes) {
    auto circles = fitFixedCountCirclesForPlane(kv.second, requested_k);
```

with:

```cpp
for (auto& kv : planes) {
    auto circles = options.adaptive_circle_count
        ? fitAdaptiveCirclesForPlane(kv.second, options.coa_threshold, options.max_circles_per_section)
        : fitFixedCountCirclesForPlane(kv.second, std::max(1, options.max_circles_per_section));
```

This step also requires changing the implementation to use `const CapsuleFitOptions& options` internally. In the old integer overload, construct options like this:

```cpp
std::vector<Capsule> fitCapsulesByCrossSection(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                               int n_sections, double coa_threshold,
                                               int max_circles_per_section, int max_capsules) {
    CapsuleFitOptions options;
    options.n_sections = n_sections;
    options.coa_threshold = coa_threshold;
    options.max_circles_per_section = max_circles_per_section;
    options.max_capsules = max_capsules;
    options.adaptive_circle_count = max_circles_per_section > 1;
    return fitCapsulesByCrossSection(V, F, options);
}
```

- [ ] **Step 5: Run threshold test**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_filter=CapsuleXSectionFit.CoaThresholdControlsCircleCount'
```

Expected after implementation: PASS.

- [ ] **Step 6: Commit adaptive plane fitting**

```bash
git add include/CapsuleCrossSection.h src/CapsuleCrossSection.cpp test/test_capsule.cpp
git commit -m "fix: make capsule section circle count coa-adaptive"
```

---

### Task 4: Preserve Coverage With Variable-Count Matching And Budget Pruning

**Files:**
- Modify: `src/CapsuleCrossSection.cpp`
- Test: `test/test_capsule.cpp`

- [ ] **Step 1: Add coverage and scoring helpers**

In `src/CapsuleCrossSection.cpp`, add near `mergeCollinearCapsules`:

```cpp
bool allCovered(const std::vector<Capsule>& caps, const Eigen::MatrixXd& V, double eps = 1e-9) {
    if (caps.empty()) return V.rows() == 0;
    for (int i = 0; i < V.rows(); ++i) {
        Eigen::Vector3d p = V.row(i).transpose();
        bool covered = false;
        for (const auto& cap : caps) {
            if (pointToSegmentDistance(p, cap.p0, cap.p1) <= cap.radius + eps) {
                covered = true;
                break;
            }
        }
        if (!covered) return false;
    }
    return true;
}

double capsuleVolume(const Capsule& cap) {
    double L = (cap.p1 - cap.p0).norm();
    return M_PI * cap.radius * cap.radius * L + 4.0 * M_PI * cap.radius * cap.radius * cap.radius / 3.0;
}

double capsuleSetVolume(const std::vector<Capsule>& caps) {
    double total = 0.0;
    for (const auto& cap : caps) total += capsuleVolume(cap);
    return total;
}
```

- [ ] **Step 2: Replace the fixed-count-only matching assumption**

In the adjacent section loop, keep matching all circles from the smaller side and create degenerate capsules for unmatched circles so local coverage can be grown without forcing every plane to have the same count.

Use this helper inside `fitCapsulesByCrossSection`:

```cpp
auto emit_degenerate = [&](const Circle2D& c, double t) {
    Capsule cap;
    cap.p0 = to3d(c, t);
    cap.p1 = cap.p0;
    cap.radius = c.radius;
    caps.push_back(cap);
};
```

Replace the section loop with:

```cpp
for (int section = 0; section < N - 1; ++section) {
    const auto& A = planeCircles[section];
    const auto& B = planeCircles[section + 1];
    std::vector<char> usedB(B.size(), 0);

    for (const auto& a : A) {
        int best = -1;
        double best_dist = std::numeric_limits<double>::max();
        for (int j = 0; j < static_cast<int>(B.size()); ++j) {
            if (usedB[j]) continue;
            double d = (a.center - B[j].center).squaredNorm();
            if (d < best_dist) {
                best_dist = d;
                best = j;
            }
        }
        if (best >= 0) {
            usedB[best] = 1;
            emit_pair(a, planeT[section], B[best], planeT[section + 1]);
        } else {
            emit_degenerate(a, planeT[section]);
        }
    }

    for (int j = 0; j < static_cast<int>(B.size()); ++j) {
        if (!usedB[j]) emit_degenerate(B[j], planeT[section + 1]);
    }
}
```

- [ ] **Step 3: Make max-capsule pruning coverage-aware**

Replace the current shortest-capsule deletion loop with:

```cpp
while (static_cast<int>(caps.size()) > options.max_capsules && caps.size() > 1) {
    int best_remove = -1;
    double best_score = std::numeric_limits<double>::max();
    std::vector<Capsule> best_candidate;

    for (int i = 0; i < static_cast<int>(caps.size()); ++i) {
        std::vector<Capsule> candidate = caps;
        candidate.erase(candidate.begin() + i);
        growCapsulesToCover(candidate, V);
        if (!allCovered(candidate, V)) continue;
        double score = capsuleSetVolume(candidate);
        if (score < best_score) {
            best_score = score;
            best_remove = i;
            best_candidate = std::move(candidate);
        }
    }

    if (best_remove < 0) break;
    caps = std::move(best_candidate);
}
growCapsulesToCover(caps, V);
```

- [ ] **Step 4: Run budget coverage test**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_filter=CapsuleXSectionFit.BudgetPruningPreservesCoverage'
```

Expected after implementation: PASS.

- [ ] **Step 5: Commit coverage-aware matching/pruning**

```bash
git add src/CapsuleCrossSection.cpp test/test_capsule.cpp
git commit -m "fix: preserve coverage when matching and pruning capsules"
```

---

### Task 5: Split Locally Inflated Capsules Along Their Axis

**Files:**
- Modify: `src/CapsuleCrossSection.cpp`
- Test: `test/test_capsule.cpp`

- [ ] **Step 1: Add a bin analysis helper**

In `src/CapsuleCrossSection.cpp`, add:

```cpp
struct CapsuleBinProfile {
    std::vector<double> bin_max;
    double median_nonzero = 0.0;
    double ratio = 0.0;
};

CapsuleBinProfile profileCapsuleBins(const Capsule& cap, const Eigen::MatrixXd& V, int bins = 10) {
    CapsuleBinProfile out;
    out.bin_max.assign(bins, 0.0);
    Eigen::Vector3d axis = cap.p1 - cap.p0;
    double denom = axis.squaredNorm();
    for (int i = 0; i < V.rows(); ++i) {
        Eigen::Vector3d p = V.row(i).transpose();
        double t = denom < 1e-12 ? 0.0 : std::clamp((p - cap.p0).dot(axis) / denom, 0.0, 1.0);
        int slot = std::min(bins - 1, std::max(0, static_cast<int>(t * bins)));
        out.bin_max[slot] = std::max(out.bin_max[slot], pointToSegmentDistance(p, cap.p0, cap.p1));
    }
    std::vector<double> nonzero;
    for (double v : out.bin_max)
        if (v > 1e-12) nonzero.push_back(v);
    if (!nonzero.empty()) {
        std::sort(nonzero.begin(), nonzero.end());
        out.median_nonzero = nonzero[nonzero.size() / 2];
        out.ratio = cap.radius / out.median_nonzero;
    }
    return out;
}
```

- [ ] **Step 2: Add splitting helper**

Add:

```cpp
bool splitMostInflatedCapsule(std::vector<Capsule>& caps,
                              const Eigen::MatrixXd& V,
                              double max_ratio,
                              int max_capsules) {
    if (static_cast<int>(caps.size()) >= max_capsules) return false;

    int worst_index = -1;
    CapsuleBinProfile worst_profile;
    double worst_ratio = max_ratio;
    for (int i = 0; i < static_cast<int>(caps.size()); ++i) {
        double length = (caps[i].p1 - caps[i].p0).norm();
        if (length < 1e-9) continue;
        auto profile = profileCapsuleBins(caps[i], V);
        if (profile.ratio > worst_ratio) {
            worst_ratio = profile.ratio;
            worst_profile = profile;
            worst_index = i;
        }
    }
    if (worst_index < 0) return false;

    Capsule original = caps[worst_index];
    int split_bin = 5;
    double best_drop = -1.0;
    for (int i = 1; i < static_cast<int>(worst_profile.bin_max.size()); ++i) {
        double left = worst_profile.bin_max[i - 1];
        double right = worst_profile.bin_max[i];
        double drop = original.radius - std::max(left, right);
        if (drop > best_drop) {
            best_drop = drop;
            split_bin = i;
        }
    }

    double t = std::clamp(split_bin / double(worst_profile.bin_max.size()), 0.15, 0.85);
    Eigen::Vector3d mid = original.p0 + t * (original.p1 - original.p0);
    Capsule left{original.p0, mid, original.radius};
    Capsule right{mid, original.p1, original.radius};
    caps[worst_index] = left;
    caps.push_back(right);
    growCapsulesToCover(caps, V);
    return true;
}
```

- [ ] **Step 3: Call local splitting after grow and before final pruning**

After:

```cpp
growCapsulesToCover(caps, V);
```

add:

```cpp
while (splitMostInflatedCapsule(caps, V, options.max_radius_bin_ratio, options.max_capsules)) {
}
growCapsulesToCover(caps, V);
```

- [ ] **Step 4: Run local split test**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_filter=CapsuleXSectionFit.LocalSplitReducesRadiusBinInflation'
```

Expected after implementation: PASS.

- [ ] **Step 5: Run all C++ capsule tests**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule'
```

Expected after implementation: all tests pass.

- [ ] **Step 6: Commit local split fix**

```bash
git add src/CapsuleCrossSection.cpp test/test_capsule.cpp
git commit -m "fix: split locally inflated capsules"
```

---

### Task 6: Wire New Options Through Generator And Config

**Files:**
- Modify: `include/CapsuleURDFGenerator.h`
- Modify: `src/CapsuleURDFGenerator.cpp`
- Modify: `config/capsule/capsuleConfig.yml`
- Modify: `config/capsule/capsuleConfig_tight.yml`

- [ ] **Step 1: Add generator fields**

In `include/CapsuleURDFGenerator.h`, replace the private config fields with:

```cpp
std::string config_path_;
int n_sections_ = 4;
double coa_threshold_ = 0.005;
int max_circles_per_section_ = 1;
int max_capsules_ = 12;
double max_radius_bin_ratio_ = 1.45;
bool adaptive_circle_count_ = false;
```

- [ ] **Step 2: Read optional YAML keys**

In the constructor, after reading `MaxCapsulesPerLink`, add:

```cpp
max_radius_bin_ratio_ = doc["MaxRadiusBinRatio"].as<double>(max_radius_bin_ratio_);
adaptive_circle_count_ = doc["AdaptiveCircleCount"].as<bool>(adaptive_circle_count_);
```

- [ ] **Step 3: Pass `CapsuleFitOptions`**

Replace the call to `fitCapsulesByCrossSection` with:

```cpp
urdf_approx_geom::CapsuleFitOptions fit_options;
fit_options.n_sections = n_sections_;
fit_options.coa_threshold = coa_threshold_;
fit_options.max_circles_per_section = max_circles_per_section_;
fit_options.max_capsules = max_capsules_;
fit_options.max_radius_bin_ratio = max_radius_bin_ratio_;
fit_options.adaptive_circle_count = adaptive_circle_count_;

auto caps = urdf_approx_geom::fitCapsulesByCrossSection(Vlf, OUT_F, fit_options);
```

- [ ] **Step 4: Update sparse config**

Set `config/capsule/capsuleConfig.yml` to:

```yaml
# Capsule approximation config.
# Sparse default: few URDF-native primitives, collision-safe outer cover.

NSections: 4
CoaThreshold: 0.005
MaxCirclesPerSection: 1
MaxCapsulesPerLink: 12
AdaptiveCircleCount: false
MaxRadiusBinRatio: 1.45
```

- [ ] **Step 5: Update tight config**

Set `config/capsule/capsuleConfig_tight.yml` to:

```yaml
# Tight capsule approximation preset.
# Adaptive section circles plus local axial splitting reduce visual over-cover.

NSections: 6
CoaThreshold: 0.005
MaxCirclesPerSection: 4
MaxCapsulesPerLink: 24
AdaptiveCircleCount: true
MaxRadiusBinRatio: 1.45
```

- [ ] **Step 6: Build**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc)'
```

Expected after implementation: build exits 0.

- [ ] **Step 7: Commit config wiring**

```bash
git add include/CapsuleURDFGenerator.h src/CapsuleURDFGenerator.cpp config/capsule/capsuleConfig.yml config/capsule/capsuleConfig_tight.yml
git commit -m "config: wire adaptive capsule fitting options"
```

---

### Task 7: Add Preset Comparison Verification

**Files:**
- Create: `scripts/compare_capsule_presets.py`

- [ ] **Step 1: Create comparison script**

Create `scripts/compare_capsule_presets.py`:

```python
#!/usr/bin/env python3
import argparse
import json
import subprocess
import sys


def load_metrics(path):
    proc = subprocess.run(
        [sys.executable, "scripts/check_capsule_coverage.py", "--caps-json", path, "--json"],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if proc.returncode != 0:
        print(proc.stdout)
        raise SystemExit(proc.returncode)
    return json.loads(proc.stdout)


def worst(metrics, key):
    return max(row[key] for row in metrics["links"])


def count(metrics):
    return sum(row["capsules"] for row in metrics["links"])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sparse-json", required=True)
    ap.add_argument("--tight-json", required=True)
    args = ap.parse_args()

    sparse = load_metrics(args.sparse_json)
    tight = load_metrics(args.tight_json)
    if not sparse["all_covered"] or not tight["all_covered"]:
        print("coverage failed for sparse or tight preset", file=sys.stderr)
        return 1

    sparse_capv = worst(sparse, "capV_aabb")
    tight_capv = worst(tight, "capV_aabb")
    sparse_ratio = worst(sparse, "r_binMed")
    tight_ratio = worst(tight, "r_binMed")
    sparse_count = count(sparse)
    tight_count = count(tight)

    print(json.dumps({
        "sparse_count": sparse_count,
        "tight_count": tight_count,
        "sparse_worst_capV_aabb": sparse_capv,
        "tight_worst_capV_aabb": tight_capv,
        "sparse_worst_r_binMed": sparse_ratio,
        "tight_worst_r_binMed": tight_ratio,
    }, indent=2, sort_keys=True))

    if tight_count <= sparse_count:
        print("tight preset did not add detail", file=sys.stderr)
        return 1
    if tight_capv > sparse_capv:
        print("tight preset worsened worst capV/aabb", file=sys.stderr)
        return 1
    if tight_ratio > sparse_ratio:
        print("tight preset worsened worst r/binMed", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 2: Make script executable**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && chmod +x scripts/compare_capsule_presets.py'
```

- [ ] **Step 3: Generate sparse and tight outputs**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sparse_capsuleized.urdf --config config/capsule/capsuleConfig.yml && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_capsuleized.urdf --config config/capsule/capsuleConfig_tight.yml'
```

Expected after algorithm fixes: both commands exit 0.

- [ ] **Step 4: Compare presets**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/compare_capsule_presets.py --sparse-json /tmp/fr3_sparse_capsuleized.json --tight-json /tmp/fr3_tight_capsuleized.json'
```

Expected after algorithm fixes: command exits 0 and prints JSON summary.

- [ ] **Step 5: Run tightness gate on explicit tight output**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_capsuleized.json'
```

Expected after algorithm fixes: command exits 0.

- [ ] **Step 6: Commit comparison script**

```bash
git add scripts/compare_capsule_presets.py
git commit -m "test: compare sparse and tight capsule presets"
```

---

### Task 8: Update Documentation And Remove Stale Claims

**Files:**
- Modify: `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`

- [ ] **Step 1: Update current-state block**

Replace stale statements that still say:

```markdown
**FR3 result:** 13 capsules ...
CoaThreshold: 0.005       # not currently used
MaxCirclesPerSection: 4   # not currently used
```

with:

```markdown
**FR3 sparse result:** generated with `config/capsule/capsuleConfig.yml`; optimized for low primitive count.

**FR3 tight result:** generated with `config/capsule/capsuleConfig_tight.yml`; must pass `scripts/check_capsule_tightness.py --caps-json <tight-json>`.

```yaml
NSections: 6
CoaThreshold: 0.005
MaxCirclesPerSection: 4
MaxCapsulesPerLink: 24
AdaptiveCircleCount: true
MaxRadiusBinRatio: 1.45
```
```

- [ ] **Step 2: Update tuning section**

Use this wording:

```markdown
- **`AdaptiveCircleCount`**: when `true`, each section plane chooses the smallest circle count whose normalized COA proxy is below `CoaThreshold`, capped by `MaxCirclesPerSection`.
- **`CoaThreshold`**: lower values request tighter section coverage and may increase capsule count. Values `<= 0` force one circle per section.
- **`MaxRadiusBinRatio`**: local axial split threshold. If one capsule radius is much larger than the median axial bin radius, the fitter splits that capsule before final pruning.
- **`MaxCapsulesPerLink`**: hard output budget. Pruning must re-grow and preserve coverage.
```

- [ ] **Step 3: Update verification commands**

Replace the single hardcoded gate command with:

```markdown
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sparse_capsuleized.urdf --config config/capsule/capsuleConfig.yml'
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_capsuleized.urdf --config config/capsule/capsuleConfig_tight.yml'
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_capsuleized.json'
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/compare_capsule_presets.py --sparse-json /tmp/fr3_sparse_capsuleized.json --tight-json /tmp/fr3_tight_capsuleized.json'
```

- [ ] **Step 4: Commit docs**

```bash
git add doc/handoffs/2026-07-02-capsule-geometry-handoff.md
git commit -m "docs: document verified capsule tightness workflow"
```

---

### Task 9: Final Verification

**Files:**
- No code changes.

- [ ] **Step 1: Build**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc)'
```

Expected: build exits 0.

- [ ] **Step 2: Run C++ capsule tests**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule'
```

Expected: all tests pass.

- [ ] **Step 3: Generate sparse and tight FR3 outputs**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sparse_capsuleized.urdf --config config/capsule/capsuleConfig.yml && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_capsuleized.urdf --config config/capsule/capsuleConfig_tight.yml'
```

Expected: both commands exit 0.

- [ ] **Step 4: Run explicit tightness gate**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_capsuleized.json'
```

Expected: exit 0.

- [ ] **Step 5: Run preset comparison**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/compare_capsule_presets.py --sparse-json /tmp/fr3_sparse_capsuleized.json --tight-json /tmp/fr3_tight_capsuleized.json'
```

Expected: exit 0.

- [ ] **Step 6: Run Python tests only if the extension exists**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && if ls build/python/urdf_approx_geom/*.so >/dev/null 2>&1; then cp build/python/urdf_approx_geom/*.so python/urdf_approx_geom/ && PYTHONPATH=/workspace/python python3 -m pytest python/tests -q; else echo "SKIP python tests: build/python/urdf_approx_geom/*.so missing"; fi'
```

Expected: either Python tests pass or the command prints the explicit skip message.

- [ ] **Step 7: Check working tree**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git status --short'
```

Expected: only intentional source/config/script/doc changes are present. Do not include `build/`, `scripts/__pycache__/`, or `/tmp` outputs in commits.

---

## Self-Review

**Spec coverage:** The plan addresses every review finding: gate path correctness, machine-readable metrics, fixed-`k` tight preset regression, real `CoaThreshold` behavior, relaxed test threshold, variable-count matching, coverage-aware budget pruning, stale docs, and explicit verification.

**Placeholder scan:** The plan uses concrete file paths, code snippets, commands, and expected outcomes. It does not rely on unresolved placeholder language.

**Type consistency:** `CapsuleFitOptions`, `CapsuleFitStats`, `fitAdaptiveCirclesForPlane`, `evaluate_capsules`, `max_radius_bin_ratio`, and `adaptive_circle_count` are consistently named across tasks.
