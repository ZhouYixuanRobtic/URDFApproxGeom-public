# FR3 Link0 Capsule Detail Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `fr3_link0` in the tight capsule preset use a measurably finer capsule approximation instead of passing the global gate with one near-limit capsule.

**Architecture:** Keep `MaxCirclesPerSection: 1` as the winning FR3 strategy, and improve axial decomposition after merge by adding volume-driven capsule splitting. Add link-specific gate checks so the regression suite can require `fr3_link0` to have at least two capsules and a lower `capV/aabb` ceiling.

**Tech Stack:** C++17, Eigen, yaml-cpp, GoogleTest, Python 3 diagnostic scripts, Docker container `spherized-development`.

---

## File Structure

- Modify `include/CapsuleCrossSection.h`: add fit options for volume-driven split thresholds.
- Modify `src/CapsuleCrossSection.cpp`: replace ratio-only split selection with best-improving split selection driven by either `r/binMed` or `capV/aabb`.
- Modify `include/CapsuleURDFGenerator.h`: store new config fields with disabled-by-default values.
- Modify `src/CapsuleURDFGenerator.cpp`: read new YAML keys and pass them into `CapsuleFitOptions`.
- Modify `config/capsule/capsuleConfig_tight.yml`: enable the tighter `fr3_link0`-motivated volume split in the tight preset.
- Leave `config/capsule/capsuleConfig.yml` sparse behavior disabled unless a test proves it must change.
- Modify `scripts/check_capsule_tightness.py`: add explicit per-link ceilings and per-link minimum capsule counts.
- Modify `scripts/compare_capsule_presets.py`: tighten the tight preset absolute ceiling after the new fit passes.
- Modify `test/test_capsule.cpp`: add one end-to-end FR3 tight preset regression for `fr3_link0`.
- Modify `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`: record the new contract, metrics, and commands.

All commands in this plan must be run inside the container:

```bash
docker exec spherized-development bash -lc 'cd /workspace && <command>'
```

---

### Task 1: Add Link-Specific Tightness Gate

**Files:**
- Modify: `scripts/check_capsule_tightness.py:1-63`

- [ ] **Step 1: Run the intended gate before implementing it**

Generate a fresh tight output:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_link0_plan.urdf --config config/capsule/capsuleConfig_tight.yml'
```

Then run the intended link-specific gate:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_link0_plan.json --link-min-capsules fr3_link0=2 --link-max-capv-aabb fr3_link0=2.25'
```

Expected before this task is implemented: FAIL because `--link-min-capsules` and `--link-max-capv-aabb` are unknown arguments.

- [ ] **Step 2: Add link limit parsing**

Insert this helper above `main()` in `scripts/check_capsule_tightness.py`:

```python
def parse_link_limits(values, cast, option_name):
    limits = {}
    for value in values:
        if "=" not in value:
            raise SystemExit(f"{option_name} must use LINK=VALUE, got: {value}")
        link, raw = value.split("=", 1)
        link = link.strip()
        if not link:
            raise SystemExit(f"{option_name} has empty link name: {value}")
        try:
            limits[link] = cast(raw)
        except ValueError as exc:
            raise SystemExit(f"{option_name} has invalid value for {link}: {raw}") from exc
    return limits
```

- [ ] **Step 3: Add new CLI arguments**

Replace the argument block in `main()` with:

```python
    parser = argparse.ArgumentParser()
    parser.add_argument("--caps-json", required=True)
    parser.add_argument("--urdf", default="resources/fr3/urdf/fr3.urdf")
    parser.add_argument("--max-capv-aabb", type=float, default=2.35)
    parser.add_argument("--max-r-binmed", type=float, default=1.45)
    parser.add_argument("--max-capsules", type=int, default=24)
    parser.add_argument("--link-max-capv-aabb", action="append", default=[],
                        help="per-link capV/aabb ceiling as LINK=VALUE")
    parser.add_argument("--link-max-r-binmed", action="append", default=[],
                        help="per-link r/binMed ceiling as LINK=VALUE")
    parser.add_argument("--link-min-capsules", action="append", default=[],
                        help="per-link minimum capsule count as LINK=VALUE")
    args = parser.parse_args()

    link_max_capv = parse_link_limits(args.link_max_capv_aabb, float, "--link-max-capv-aabb")
    link_max_ratio = parse_link_limits(args.link_max_r_binmed, float, "--link-max-r-binmed")
    link_min_capsules = parse_link_limits(args.link_min_capsules, int, "--link-min-capsules")
```

- [ ] **Step 4: Enforce per-link limits in the existing loop**

Replace the current `for row in result["links"]:` failure block with:

```python
    seen_links = set()
    for row in result["links"]:
        link = row["link"]
        seen_links.add(link)
        max_capv = link_max_capv.get(link, args.max_capv_aabb)
        max_ratio = link_max_ratio.get(link, args.max_r_binmed)
        min_capsules = link_min_capsules.get(link)

        if min_capsules is not None and row["capsules"] < min_capsules:
            failures.append(f"{link}: capsules {row['capsules']} < {min_capsules}")
        if row["capsules"] > args.max_capsules:
            failures.append(f"{link}: capsules {row['capsules']} > {args.max_capsules}")
        if row["capV_aabb"] > max_capv:
            failures.append(f"{link}: capV/aabb {row['capV_aabb']:.2f} > {max_capv:.2f}")
        if row["r_binMed"] > max_ratio:
            failures.append(f"{link}: r/binMed {row['r_binMed']:.2f} > {max_ratio:.2f}")

    requested_links = set(link_max_capv) | set(link_max_ratio) | set(link_min_capsules)
    for missing in sorted(requested_links - seen_links):
        failures.append(f"{missing}: link not present in capsule metrics")
```

- [ ] **Step 5: Run the gate and verify it now fails for the right reason**

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_link0_plan.json --link-min-capsules fr3_link0=2 --link-max-capv-aabb fr3_link0=2.25'
```

Expected: FAIL with both of these facts visible in stderr/stdout:

```text
fr3_link0: capsules 1 < 2
fr3_link0: capV/aabb 2.33 > 2.25
```

- [ ] **Step 6: Commit**

```bash
git add scripts/check_capsule_tightness.py
git commit -m "test: add link-specific capsule tightness gate"
```

---

### Task 2: Add End-to-End FR3 Link0 Regression Test

**Files:**
- Modify: `test/test_capsule.cpp:483-544`

- [ ] **Step 1: Add the failing GoogleTest**

Append this test after `CapsuleRun.EmitsNativeCylinderSphere` and before `countDegenerateCapsules`:

```cpp
TEST(CapsuleRun, TightPresetAddsBaseLinkDetail) {
    const std::string out_urdf = "/tmp/fr3_tight_link0_detail_test.urdf";
    CapsuleURDFGenerator g(std::string(URDFApproxGeom_CONFIG_PATH) +
                           "/capsule/capsuleConfig_tight.yml");
    auto ret = g.run("/workspace/resources/fr3/urdf/fr3.urdf", out_urdf, {});
    ASSERT_TRUE(ret.isOk()) << ret.message();

    std::ifstream f("/tmp/fr3_tight_link0_detail_test.json");
    ASSERT_TRUE(f.good()) << "JSON sidecar not written";
    nlohmann::json j;
    f >> j;

    ASSERT_TRUE(j.contains("fr3_link0")) << "fr3_link0 missing from tight capsule JSON";
    ASSERT_TRUE(j["fr3_link0"].contains("capsules")) << "fr3_link0 capsules missing";
    EXPECT_GE(j["fr3_link0"]["capsules"].size(), 2u)
        << "The tight preset should not leave the base link as one near-threshold capsule";
}
```

- [ ] **Step 2: Run the single test and verify it fails**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_color=no --gtest_filter=CapsuleRun.TightPresetAddsBaseLinkDetail'
```

Expected: FAIL with:

```text
Expected: (j["fr3_link0"]["capsules"].size()) >= (2u), actual: 1 vs 2
```

- [ ] **Step 3: Commit**

```bash
git add test/test_capsule.cpp
git commit -m "test: require tight base link capsule detail"
```

---

### Task 3: Wire Volume Split Options Without Changing Behavior

**Files:**
- Modify: `include/CapsuleCrossSection.h:72-79`
- Modify: `include/CapsuleURDFGenerator.h:37-46`
- Modify: `src/CapsuleURDFGenerator.cpp:28-37,150-157`

- [ ] **Step 1: Extend `CapsuleFitOptions`**

Replace the struct in `include/CapsuleCrossSection.h` with:

```cpp
struct CapsuleFitOptions {
    int n_sections = 10;
    double coa_threshold = 1e-4;
    int max_circles_per_section = 4;
    int max_capsules = 6;
    double max_radius_bin_ratio = 1.45;
    double max_capv_aabb_ratio = -1.0;
    double min_split_volume_improvement = 0.005;
    bool adaptive_circle_count = true;
};
```

- [ ] **Step 2: Store generator defaults**

Replace the private fields in `include/CapsuleURDFGenerator.h` with:

```cpp
private:
    std::string config_path_;
    int n_sections_ = 4;
    double coa_threshold_ = 0.005;
    int max_circles_per_section_ = 1;
    int max_capsules_ = 12;
    double max_radius_bin_ratio_ = 1.45;
    double max_capv_aabb_ratio_ = -1.0;
    double min_split_volume_improvement_ = 0.005;
    bool adaptive_circle_count_ = false;
};
```

- [ ] **Step 3: Read YAML keys**

Add these two lines in `CapsuleURDFGenerator::CapsuleURDFGenerator()` after `max_radius_bin_ratio_` is read:

```cpp
        max_capv_aabb_ratio_ = doc["MaxCapVAabbRatio"].as<double>(max_capv_aabb_ratio_);
        min_split_volume_improvement_ = doc["MinSplitVolumeImprovement"].as<double>(min_split_volume_improvement_);
```

- [ ] **Step 4: Pass options into the fitter**

Add these two assignments before `adaptive_circle_count` is assigned into `fit_options`:

```cpp
        fit_options.max_capv_aabb_ratio = max_capv_aabb_ratio_;
        fit_options.min_split_volume_improvement = min_split_volume_improvement_;
```

- [ ] **Step 5: Build**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc)'
```

Expected: build succeeds. `CapsuleRun.TightPresetAddsBaseLinkDetail` still fails because no behavior uses the new options yet.

- [ ] **Step 6: Commit**

```bash
git add include/CapsuleCrossSection.h include/CapsuleURDFGenerator.h src/CapsuleURDFGenerator.cpp
git commit -m "config: wire capsule volume split thresholds"
```

---

### Task 4: Replace Ratio-Only Split With Best-Improving Split

**Files:**
- Modify: `src/CapsuleCrossSection.cpp:201-259,894-900`

- [ ] **Step 1: Replace `splitMostInflatedCapsule`**

Replace the whole `splitMostInflatedCapsule` function with:

```cpp
bool splitMostInflatedCapsule(std::vector<Capsule>& caps,
                              const Eigen::MatrixXd& V,
                              double max_ratio,
                              double max_capv_aabb_ratio,
                              double min_volume_improvement,
                              int max_capsules) {
    if (static_cast<int>(caps.size()) >= max_capsules) return false;

    auto before_metrics = evaluateCapsuleTightness(V, caps);
    bool ratio_pressure = max_ratio > 0.0 &&
                          before_metrics.max_radius_bin_ratio > max_ratio;
    bool volume_pressure = max_capv_aabb_ratio > 0.0 &&
                           before_metrics.capV_aabb > max_capv_aabb_ratio;
    if (!ratio_pressure && !volume_pressure) return false;

    auto assignment = assignVerticesToCapsules(V, caps);

    bool found = false;
    double best_score = std::numeric_limits<double>::max();
    std::vector<Capsule> best_candidate;

    for (int i = 0; i < static_cast<int>(caps.size()); ++i) {
        if ((caps[i].p1 - caps[i].p0).norm() < 1e-9) continue;
        Eigen::MatrixXd local = assignedVerticesForCapsule(V, assignment, i);
        if (local.rows() == 0) continue;
        auto profile = profileCapsuleBins(caps[i], local);
        if (profile.bin_max.size() < 2) continue;

        Capsule original = caps[i];
        for (int split_bin = 1; split_bin < static_cast<int>(profile.bin_max.size()); ++split_bin) {
            double t = std::clamp(split_bin / double(profile.bin_max.size()), 0.15, 0.85);
            Eigen::Vector3d mid = original.p0 + t * (original.p1 - original.p0);

            std::vector<Capsule> candidate = caps;
            candidate[i] = Capsule{original.p0, mid, 0.0};
            candidate.push_back(Capsule{mid, original.p1, 0.0});
            resizeCapsulesFromAssignedVertices(candidate, V);
            growCapsulesToCover(candidate, V);
            candidate = dedupeNestedCapsules(candidate);

            auto after_metrics = evaluateCapsuleTightness(V, candidate);
            if (!after_metrics.covered) continue;

            double volume_drop = before_metrics.capsule_volume - after_metrics.capsule_volume;
            double volume_drop_ratio = volume_drop / std::max(before_metrics.capsule_volume, 1e-12);
            bool improves_volume = volume_pressure &&
                after_metrics.capV_aabb < before_metrics.capV_aabb &&
                volume_drop_ratio >= min_volume_improvement;
            bool improves_ratio = ratio_pressure &&
                after_metrics.max_radius_bin_ratio < before_metrics.max_radius_bin_ratio;
            if (!improves_volume && !improves_ratio) continue;

            double score = after_metrics.capV_aabb;
            if (max_capv_aabb_ratio > 0.0) {
                score += std::max(0.0, after_metrics.capV_aabb - max_capv_aabb_ratio);
            }
            if (max_ratio > 0.0) {
                score += after_metrics.max_radius_bin_ratio / max_ratio;
            }

            if (score < best_score) {
                best_score = score;
                best_candidate = std::move(candidate);
                found = true;
            }
        }
    }

    if (!found) return false;
    caps = std::move(best_candidate);
    return true;
}
```

- [ ] **Step 2: Pass the new thresholds at the call site**

Replace the split loop at `src/CapsuleCrossSection.cpp:896-898` with:

```cpp
    if (options.max_radius_bin_ratio > 0 || options.max_capv_aabb_ratio > 0) {
        while (splitMostInflatedCapsule(caps,
                                        V,
                                        options.max_radius_bin_ratio,
                                        options.max_capv_aabb_ratio,
                                        options.min_split_volume_improvement,
                                        options.max_capsules)) {
        }
        growCapsulesToCover(caps, V);
    }
```

- [ ] **Step 3: Run focused existing split tests**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_color=no --gtest_filter=CapsuleXSectionFit.LocalSplitReducesRadiusBinInflation:CapsuleXSectionFit.LocalSplitReducesVolumeWhenAccepted:CapsuleXSectionFit.BudgetPruningPreservesCoverage'
```

Expected: all three tests PASS.

- [ ] **Step 4: Run the new link0 test**

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule --gtest_color=no --gtest_filter=CapsuleRun.TightPresetAddsBaseLinkDetail'
```

Expected at this point: still FAIL if `config/capsule/capsuleConfig_tight.yml` has not enabled `MaxCapVAabbRatio`.

- [ ] **Step 5: Commit**

```bash
git add src/CapsuleCrossSection.cpp
git commit -m "fix: split capsules on volume tightness pressure"
```

---

### Task 5: Enable the Link0 Tight Preset Contract

**Files:**
- Modify: `config/capsule/capsuleConfig_tight.yml:1-9`
- Modify: `scripts/compare_capsule_presets.py:33-37`

- [ ] **Step 1: Enable volume-driven split in the tight preset**

Replace `config/capsule/capsuleConfig_tight.yml` with:

```yaml
# Tight capsule approximation preset.
# More section planes for finer axial decomposition.

NSections: 6
CoaThreshold: 0.005
MaxCirclesPerSection: 1
MaxCapsulesPerLink: 16
AdaptiveCircleCount: false
MaxRadiusBinRatio: 1.45
MaxCapVAabbRatio: 2.25
MinSplitVolumeImprovement: 0.005
```

- [ ] **Step 2: Tighten comparison script defaults**

Change defaults in `scripts/compare_capsule_presets.py`:

```python
    ap.add_argument("--max-capv-aabb", type=float, default=2.25,
                    help="absolute capV/aabb ceiling for tight preset")
    ap.add_argument("--max-r-binmed", type=float, default=1.45,
                    help="absolute r/binMed ceiling for tight preset")
```

- [ ] **Step 3: Generate fresh sparse and tight outputs**

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sparse_link0_plan.urdf --config config/capsule/capsuleConfig.yml'
```

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_link0_plan.urdf --config config/capsule/capsuleConfig_tight.yml'
```

Expected: both commands exit 0 and write matching `.json` files in `/tmp`.

- [ ] **Step 4: Run the link0 gate**

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_link0_plan.json --max-capv-aabb 2.25 --max-r-binmed 1.45 --link-min-capsules fr3_link0=2 --link-max-capv-aabb fr3_link0=2.25'
```

Expected: PASS. The printed JSON must include:

```json
{
  "link": "fr3_link0",
  "covered": true,
  "capsules": 2
}
```

The exact `fr3_link0` capsule count may be greater than 2 if the best-improving split loop accepts more than one split, but it must not exceed `MaxCapsulesPerLink: 16`.

- [ ] **Step 5: Run the comparison gate**

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/compare_capsule_presets.py --sparse-json /tmp/fr3_sparse_link0_plan.json --tight-json /tmp/fr3_tight_link0_plan.json'
```

Expected: PASS with `tight_count > sparse_count`, `tight_worst_capV_aabb <= 2.25`, and `tight_worst_r_binMed <= 1.45`.

- [ ] **Step 6: Run the new C++ test**

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule --gtest_color=no --gtest_filter=CapsuleRun.TightPresetAddsBaseLinkDetail'
```

Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add config/capsule/capsuleConfig_tight.yml scripts/compare_capsule_presets.py
git commit -m "config: require tighter base-link capsule detail"
```

---

### Task 6: Update Documentation and Remove Stale Threshold Text

**Files:**
- Modify: `doc/handoffs/2026-07-02-capsule-geometry-handoff.md:30,56-66,240-252`

- [ ] **Step 1: Update the summary contract**

Replace the stale threshold sentence around line 30 with:

```markdown
**FR3 tight result:** generated with `config/capsule/capsuleConfig_tight.yml`. Comparison gate (`compare_capsule_presets.py`) must pass: tight improved over sparse on capV/aabb, r/binMed, and capsule count. Tightness gate (`check_capsule_tightness.py`) uses `--max-capv-aabb=2.25 --max-r-binmed=1.45` plus `--link-min-capsules fr3_link0=2` so the base link cannot regress to a single near-threshold capsule.
```

- [ ] **Step 2: Update the command block**

Use these commands in the handoff:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sparse_capsuleized.urdf --config config/capsule/capsuleConfig.yml'
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_capsuleized.urdf --config config/capsule/capsuleConfig_tight.yml'
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_capsuleized.json --max-capv-aabb 2.25 --max-r-binmed 1.45 --link-min-capsules fr3_link0=2 --link-max-capv-aabb fr3_link0=2.25'
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/compare_capsule_presets.py --sparse-json /tmp/fr3_sparse_capsuleized.json --tight-json /tmp/fr3_tight_capsuleized.json'
```

- [ ] **Step 3: Record the verified metrics**

After running Task 7 verification, update the metrics JSON in the handoff with the actual command output. The entry must include:

```json
{
  "sparse_count": 15,
  "tight_count": 18,
  "tight_worst_capV_aabb": 2.25,
  "tight_worst_r_binMed": 1.45
}
```

Use the actual values from `compare_capsule_presets.py`; the values above are the maximum allowed shape of the record, not expected exact numbers.

- [ ] **Step 4: Commit**

```bash
git add doc/handoffs/2026-07-02-capsule-geometry-handoff.md
git commit -m "docs: document base-link capsule tightness contract"
```

---

### Task 7: Full Verification

**Files:**
- No source edits in this task.

- [ ] **Step 1: Build**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc)'
```

Expected: build exits 0.

- [ ] **Step 2: Run C++ capsule suite**

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule --gtest_color=no'
```

Expected: all GoogleTest cases PASS.

- [ ] **Step 3: Regenerate sparse and tight outputs**

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_sparse_capsuleized.urdf --config config/capsule/capsuleConfig.yml'
```

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_tight_capsuleized.urdf --config config/capsule/capsuleConfig_tight.yml'
```

Expected: both commands exit 0.

- [ ] **Step 4: Run the tightness gate**

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_tight_capsuleized.json --max-capv-aabb 2.25 --max-r-binmed 1.45 --link-min-capsules fr3_link0=2 --link-max-capv-aabb fr3_link0=2.25'
```

Expected: exit 0, `all_covered` is `true`, and `fr3_link0` has at least 2 capsules.

- [ ] **Step 5: Run the comparison gate**

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 scripts/compare_capsule_presets.py --sparse-json /tmp/fr3_sparse_capsuleized.json --tight-json /tmp/fr3_tight_capsuleized.json'
```

Expected: exit 0, tight preset improves sparse on worst `capV/aabb`, worst `r/binMed`, and count, with tight worst `capV/aabb <= 2.25`.

- [ ] **Step 6: Run Python tests and preserve the known failure note if it remains**

```bash
docker exec spherized-development bash -lc 'cd /workspace/python && pytest -q'
```

Expected: if this still fails with the known Bus error in `python/tests/test_capsule.py`, do not hide it. Keep the handoff note explicit that C++ and capsule gates pass while Python binding tests still crash.

- [ ] **Step 7: Commit any metric/doc correction**

```bash
git add doc/handoffs/2026-07-02-capsule-geometry-handoff.md
git commit -m "docs: record verified base-link capsule metrics"
```

Run this commit only if Task 6's metric block changed after full verification.

---

## Self-Review Notes

- Spec coverage: the plan addresses the user concern directly by requiring `fr3_link0` to have at least two capsules and a lower `capV/aabb` ceiling, while preserving the known finding that multi-circle sections worsen FR3 metrics.
- Placeholder scan: no step relies on undefined files, vague validation, or deferred implementation details.
- Type consistency: new option names are `max_capv_aabb_ratio` and `min_split_volume_improvement` in C++, and `MaxCapVAabbRatio` / `MinSplitVolumeImprovement` in YAML.
- Risk: if volume-driven axial splits cannot get `fr3_link0` below `2.25` without exceeding 16 capsules, stop after Task 5 verification failure and debug split placement with `scripts/check_capsule_coverage.py --caps-json /tmp/fr3_tight_link0_plan.json --json`; do not loosen the gate without review.
