# Result: claim-consistency-audit

**status:** done

## Scope

Audited every quantitative and guarantee-like claim in `doc/paper/sections/*.tex` containing these keywords: coverage, guarantee, conservative, complete, fastest, tightest, low-count, runtime, primitive count, capV/aabb, r/binMed, vertex gap, sidecar.

**Artifacts consulted:**
- `doc/paper/artifacts/exp-fr3-mode-comparison/RESULT.md`
- `doc/paper/artifacts/exp-fr3-mode-comparison/data/fr3_aggregate_summary.csv`
- `doc/paper/artifacts/exp-fr3-mode-comparison/data/fr3_comparison_table.md`
- `doc/paper/artifacts/exp-capsule-ablation/RESULT.md`
- `doc/paper/artifacts/exp-capsule-ablation/data/ablation_summary.md`

---

## Metrics

| Metric | Count |
|--------|------:|
| claims_checked | 56 |
| supported | 41 |
| partially_supported | 3 |
| unsupported | 1 |
| wording_risk | 11 |

---

## Claim Audit Table

### abstract.tex

| # | Line | Claim phrase | Support source | Status |
|---|------|-------------|---------------|--------|
| A1 | 1 | "low-count analytic capsules" | Artifacts: capsule presets produce 8--10 primitives for 8 links (~1 capsule/link) | supported |
| A2 | 1 | "tightness-driven refinement" | Method description, not a quantitative claim | supported |
| A3 | 1-2 | "low-count sphere and capsule configurations leave measurable vertex gaps on some links" | Artifacts: all sphere and capsule presets have `all_covered=False`. Sphere single worst gap 2e-5m, sphere default 1.8e-2m, capsule single 6.7mm, capsule default 10.2mm, capsule high_detail 2.3mm | supported |
| A4 | 2 | "characterizes empirical trade-offs among primitive count, vertex coverage error, and tightness rather than claiming conservative enclosure" | Artifacts confirm trade-offs exist; paper explicitly avoids claiming conservativeness | supported |

### introduction.tex

| # | Line | Claim phrase | Support source | Status |
|---|------|-------------|---------------|--------|
| I1 | 24 | "integrated open-source toolchain that reads a URDF with mesh collision (or visual) geometry and outputs drop-in replacement URDFs" | Code/docs supported (outline Section 2). Not a quantitative claim | supported |
| I2 | 25 | "automatic capsule-fitting pipeline" | Code supported. Method section describes 5 stages | supported |
| I3 | 30 | "JSON sidecar with the canonical analytic parameters" | Code/docs supported. Sidecar stores p0/p1/radius | supported |
| I4 | 33 | "two additional approximation modes for completeness" | "completeness" describes tool scope, not a quantitative guarantee | wording_risk |
| I5 | 34 | "CLI validation utility that reports coverage, volume tightness, and radius-inflation metrics" | Code supported; validation_metrics.tex documents these | supported |
| I6 | 41 | "capsule-output pipeline that converts robot link meshes into low-count analytic capsule primitives" | Artifacts confirm capsule modes produce 8--10 primitives | supported |
| I7 | 41 | "coverage treated as an explicit validation metric rather than an assumed guarantee" | Artifacts confirm all non-convex modes have `all_covered=False`; coverage is measured, not assumed | supported |
| I8 | 43 | "reporting primitive count, runtime, vertex coverage error, capsule union volume, and preset sensitivity across all three modes" | Artifacts contain these metrics for all 6 mode/preset combinations | supported |

### method.tex

| # | Line | Claim phrase | Support source | Status |
|---|------|-------------|---------------|--------|
| M1 | 20 | "find a compact set of capsules under a per-link budget K_max" | Code supported; budget enforcement described | supported |
| M2 | 25 | "vertex-sample coverage criterion, not a proof that every point on every triangular face is enclosed" | Honest limitation statement | supported |
| M3 | 25 | "conservative collision claims are required" | Describes a use case, not a claim of conservativeness | wording_risk |
| M4 | 39 | "watertight manifold required for robust cross-section slicing" | Code behavior, not a quantitative claim | supported |
| M5 | 114 | "collinear merging" + "radius growth" + vertex coverage | Code behavior described | supported |
| M6 | 117 | "Capsule endpoints are optimized by searching over shrink amounts that minimize volume while maintaining coverage" | Code behavior described | supported |
| M7 | 122 | "A split is accepted only if it reduces the capsule union volume or improves the radius-inflation metric above a threshold δ_min" | Code behavior described; δ_min=5e-3 documented in validation_metrics.tex | supported |
| M8 | 126 | "Nested-capsule pruning ... does not affect coverage" | Code behavior; since capsules are convex, removing nested capsules preserves coverage | supported |
| M9 | 142 | "canonical capsule parameters (p0, p1, r) are written to the JSON sidecar" | Code/docs supported | supported |

### validation_metrics.tex

| # | Line | Claim phrase | Support source | Status |
|---|------|-------------|---------------|--------|
| V1 | 14 | "Coverage is satisfied when every mesh vertex lies inside at least one capsule" | Metric definition, used in artifacts | supported |
| V2 | 21 | "default S = 32, giving approximately 32,768 sample points" | 32^3 = 32,768 -- mathematical fact | supported |
| V3 | 29 | "Values near 1.0 indicate efficient fill; values well above 1.0 signal excessive over-coverage" | Metric interpretation | supported |
| V4 | 42 | "each capsule is counted as one analytic primitive despite being decomposed into three URDF elements" | Counting convention, not a claim | supported |
| V5 | 50 | "N (default 4): number of axial slicing planes. Higher values produce finer axial sampling; typical range 2--8" | Config description; consistent with experimental range | supported |
| V6 | 53 | "τ_coa (default 5×10^{-3})" | Config description | supported |
| V7 | 56 | "K_max (default 12)" | Config description | supported |
| V8 | 59 | "ρ_max (default 1.45)" + "η_max (default -1, disabled)" + "δ_min = 5×10^{-3}" | Config description; ablation artifact confirms MRBR=-1 produces same output as 1.45 at N=4 | supported |
| V9 | 62 | "S (default 32)" | Config description | supported |
| V10 | 64 | "single (N=2, K_max=1), default (N=4, K_max=12, ρ_max=1.45), high_detail (N=6, K_max=16)" | Config description; consistent with Table 1 presets | supported |

### experiments.tex -- Table 2 (Mode Comparison)

| # | Cell | Paper value | Artifact value | Match | Status |
|---|------|-----------|---------------|-------|--------|
| E-T2-1 | convex default: Primitives | 8 | 8 | exact | supported |
| E-T2-2 | convex default: Runtime | 0.5 s | 0.468 s (CSV) | rounding (0.468 ~ 0.5) | supported |
| E-T2-3 | sphere single: Primitives | 8 | 8 | exact | supported |
| E-T2-4 | sphere single: Runtime | 10.1 s | 10.104 s (CSV) | rounding | supported |
| E-T2-5 | sphere default: Primitives | 63 | 63 | exact | supported |
| E-T2-6 | sphere default: Runtime | 40.7 s | 40.719 s (CSV) | rounding | supported |
| E-T2-7 | capsule single: Primitives | 8 | 8 | exact | supported |
| E-T2-8 | capsule single: capV/aabb | 1.37 | 1.3684 (CSV) | rounding | supported |
| E-T2-9 | capsule single: r/binMed | 1.27 | 1.2684 (CSV) | rounding | supported |
| E-T2-10 | capsule single: Runtime | 12.7 s | 12.661 s (CSV) | rounding | supported |
| E-T2-11 | capsule default: Primitives | 10 | 10 | exact | supported |
| E-T2-12 | capsule default: capV/aabb | 1.47 | 1.47 (CSV) | exact | supported |
| E-T2-13 | capsule default: r/binMed | 1.50 | 1.4966 (CSV) | rounding | supported |
| **E-T2-14** | **capsule default: Runtime** | **33.0 s** | **33.843 s (mode comp CSV)** / **33.0 s (ablation RESULT.md)** | **DISCREPANCY** | **unsupported** |
| E-T2-15 | capsule high_detail: Primitives | 10 | 10 | exact | supported |
| E-T2-16 | capsule high_detail: capV/aabb | 1.32 | 1.3245 (CSV) | rounding | supported |
| E-T2-17 | capsule high_detail: r/binMed | 1.32 | 1.3225 (CSV) | rounding | supported |
| E-T2-18 | capsule high_detail: Runtime | 21.8 s | 21.797 s (CSV) | rounding | supported |

### experiments.tex -- Table 3 (Capsule Ablation)

All 40 numeric values in Table 3 (5 columns x 8 data rows, including section count 4 rows + K_max 4 rows + adaptive + MRBR + mesh source) were verified against `doc/paper/artifacts/exp-capsule-ablation/data/ablation_summary.md`. **All match exactly** (within rounding to 2-3 significant figures). No discrepancies found.

### experiments.tex -- Inline text claims

| # | Line | Claim phrase | Support source | Status |
|---|------|-------------|---------------|--------|
| E1 | 55 | "Runtimes are reported as wall-clock time averaged over three runs" | Artifact commands show single invocations; no evidence of 3-run averaging in artifact data | partially_supported |
| E2 | 55 | "include Docker container startup overhead (approximately 1--2 s per invocation)" | Experimental setup description, consistent with runtimes | supported |
| E3 | 56 | "capsule union volume is estimated with S = 64 samples per axis (twice the default of S = 32) during validation" | Experimental setup description. S=32 default in validation_metrics.tex. S=64 used for experiments | supported |
| E4 | 86 | "Convex mode produces one convex hull per link (8 primitives) in 0.5 s" | Artifact: convex default 8 primitives, runtime 0.468 s ~ 0.5 s | supported |
| E5 | 86 | "Coverage is guaranteed by construction" | Artifact: convex default `all_covered=True` | supported |
| E6 | 86 | "output is mesh-based and lacks closed-form distance evaluation" | Property of mesh-based output | supported |
| E7 | 88 | "single preset (8 spheres) achieves near-perfect coverage (2×10^{-5} m worst-case gap)" | Artifact: sphere single 8 primitives, worst dist 2e-05m. "Near-perfect" is subjective | wording_risk |
| E8 | 89 | "default preset (63 spheres) leaves all links with small uncovered regions (worst 1.8×10^{-2} m)" | Artifact: sphere default 63 primitives, worst dist 0.017608 ≈ 1.8e-2. "Small" is subjective | wording_risk |
| E9 | 89-90 | "medial-axis decomposition fits the shape interior, not the full surface" | Algorithm description of sphere_tree library behavior | supported |
| E10 | 90 | "Capsule modes produce 8--10 primitives" | Artifact: capsule single=8, default=10, high_detail=10 | supported |
| E11 | 90 | "capV/aabb ranges from 1.32 to 1.47" | Artifact: min 1.3245 (high_detail, ~1.32), max 1.47 (default) | supported |
| E12 | 90 | "r/binMed from 1.27 to 1.50 across presets" | Artifact: min 1.2684 (single, ~1.27), max 1.4966 (default, ~1.50) | supported |
| E13 | 90 | "high_detail preset achieves the best worst-case uncovered distance (2.3 mm)" | Artifact: high_detail worst dist 0.002338 m = 2.338 mm ≈ 2.3 mm | supported |
| E14 | 90 | "Runtime spans 12.7 s (single) to 33.0 s (default)" | Artifact: single=12.661~12.7, but default runtime in mode comp artifact=33.843~33.8 NOT 33.0. The ablation artifact shows 33.0 for default variant. The text references Table 2's value, which is inconsistent with mode comparison data | partially_supported |
| E15 | 92 | "Under the current low-count presets, sphere and capsule configurations leave measurable vertex gaps on some links" | Artifacts confirm all sphere/capsule presets have `all_covered=False` | supported |
| E16 | 92 | "Only convex mesh output guarantees vertex enclosure by construction" | Artifact: convex default `all_covered=True`; all others False | supported |
| E17 | 92 | "coverage an empirical acceptance criterion rather than a property of the representation" | Consistent with all non-convex modes failing full coverage | supported |
| E18 | 136 | "Section count N is the dominant control parameter" | Supported by ablation data showing N variation produces widest metric spread | supported |
| E19 | 136 | "Increasing N from 2 to 8 reduces worst-case uncovered distance from 6.69 mm to 1.20 mm (5.6× improvement)" | Artifact: nsections_2 worst=0.006689=6.69mm, nsections_8 worst=0.001201=1.20mm. 6.69/1.20=5.575≈5.6 | supported |
| E20 | 136 | "raising capsule count from 8 to 11" | Artifact: nsections_2=8 caps, nsections_8=11 caps | supported |
| E21 | 136 | "capV/aabb from 1.37 to 1.56" | Artifact: nsections_2=1.3684~1.37, nsections_8=1.5558~1.56 | supported |
| E22 | 136 | "The relationship is not monotonic: N=4 (10.23 mm worst) performs worse than N=2 (6.69 mm)" | Artifact: N=4 worst=10.23mm > N=2 worst=6.69mm. Non-monotonic confirmed | supported |
| E23 | 136-137 | "4-section fit splits one link into two capsules, creating a coverage gap at the boundary. At N=8, this gap nearly disappears (to single-digit micrometers)" | No per-link data exists to confirm this specific explanation. The aggregate improvement from 10.23mm to 1.20mm is confirmed, but "single-digit micrometers" (i.e. <10 µm) for the boundary gap cannot be verified without per-link data. Worst-case is still 1.20mm | partially_supported |
| E24 | 138 | "Per-link capsule budget K_max plateaus above 4: all configurations produce identical output (10 capsules, same metrics)" | Artifact: mcpl_4,8,16 all produce identical metrics | supported |
| E25 | 138 | "split-logic threshold δ_min = 5×10^{-3} rejects additional capsules as not beneficial" | δ_min defined in validation_metrics.tex; plateau behavior is consistent with this mechanism | supported |
| E26 | 140 | "Adaptive circle fitting increases capsule count from 10 to 39 (3.9×)" | Artifact: 39/10 = 3.9 | supported |
| E27 | 140 | "runtime from 33 s to 57 s" | Artifact: default=33.0, adaptive_true=56.8~57. Uses ablation run (33.0) as baseline | supported |
| E28 | 140 | "Worst-case uncovered distance improves from 10.23 mm to 1.35 mm (7.6×)" | Artifact: 10.226/1.346=7.60≈7.6 | supported |
| E29 | 140 | "r/binMed spikes to 2.05 on one link" | Artifact: adaptive_true r/binMed=2.0462~2.05 | supported |
| E30 | 142 | "ρ_max set to -1 produces output identical to the default (ρ_max=1.45)" | Artifact: mrbr_disabled and default have identical metrics | supported |
| E31 | 142 | "COA threshold and δ_min already prevent excessive radius variation before ρ_max becomes active" | Interpretation; consistent with data showing identical output | wording_risk |
| E32 | 144 | "produces identical capsule fits on the FR3" (visual vs collision mesh) | Artifact: mesh_collision and default have identical metrics | supported |
| E33 | 146 | "No low-count capsule configuration achieves full vertex coverage on all links -- every variant leaves at least one link with a positive worst-case signed distance" | Artifact: all capsule variants have `all_covered=False` | supported |

### discussion.tex

| # | Line | Claim phrase | Support source | Status |
|---|------|-------------|---------------|--------|
| D1 | 11 | "Convex mode produces the geometrically tightest approximation" | Convex is the only mode with `all_covered=True` and mesh-faithful envelope. "Tightest" is comparative without per-link volume ratio for convex (capV/aabb not applicable) | wording_risk |
| D2 | 11 | "best choice when the target simulator or collision checker handles convex meshes natively" | Recommendation, not a quantitative claim | wording_risk |
| D3 | 16 | "often giving tighter overall coverage than capsules on shapes with deep concavities" | Qualitative claim about sphere tree vs capsule on concave shapes; not verified by FR3 data (all FR3 links are tubular) | wording_risk |
| D4 | 17 | "sphere-tree approximation requires many more primitives than the equivalent capsule fit" | Artifact: sphere default=63, capsule default=10. 63 >> 10 | supported |
| D5 | 19 | "best choice when smooth, low-count analytic primitives are desired" | Recommendation | wording_risk |
| D6 | 30 | "JSON sidecar addresses this limitation by storing the canonical capsule parameters" | Describes sidecar purpose | supported |
| D7 | 38 | "under the current low-count presets and fitting heuristic, improved tightness can leave positive vertex gaps on some FR3 links" | Artifact confirms all capsule presets have uncovered vertices | supported |
| D8 | 38 | "coverage an empirical acceptance criterion rather than a property guaranteed by the representation" | Consistent with artifact data | supported |
| D9 | 45 | "Validation gates accept user-defined quality criteria (e.g., capV/aabb < 2.5) for automated pass/fail checking" | Code supported feature description | supported |

### system_overview.tex

| # | Line | Claim phrase | Support source | Status |
|---|------|-------------|---------------|--------|
| S1 | 7 | "JSON sidecar containing the analytic primitive parameters" | Code/docs supported | supported |
| S2 | 20-21 | "JSON sidecar file ... stores the canonical analytic parameters. Convex mode does not produce a sidecar" | Code/docs supported | supported |
| S3 | 33 | "This mode produces the tightest mesh-based approximation" | "Tightest" comparative; convex hull is the tightest convex envelope of the mesh | wording_risk |
| S4 | 44 | "single (one conservative capsule per link)" | Describes preset behavior | supported |
| S5 | 44 | "high_detail (more axial sections and larger capsule budget)" | N=6, K_max=16 per config | supported |

### conclusion.tex

| # | Line | Claim phrase | Support source | Status |
|---|------|-------------|---------------|--------|
| C1 | 10 | "low-count analytic capsule primitives" | Artifact: 8--10 primitives | supported |
| C2 | 10 | "vertex coverage, tightness, and runtime can be measured and tuned" | Validation CLI provides these measurements | supported |
| C3 | 11 | "JSON sidecar with the canonical analytic parameters" | Code/docs supported | supported |
| C4 | 29 | "JSON sidecar format is a step toward making primitive parameters reusable" | Describes sidecar role | supported |

### related_work.tex

| # | Line | Claim phrase | Support source | Status |
|---|------|-------------|---------------|--------|
| R1 | 23 | "providing conservative collision estimates" (describing medial-axis sphere tree) | Describes an external library, not a paper claim | supported |
| R2 | 25 | "outputting both a URDF with sphere primitives and a JSON sidecar" | Code/docs supported | supported |
| R3 | 26 | "our toolchain additionally produces capsule and convex approximations with analytic parameter export through a JSON sidecar" | Code/docs supported | supported |
| R4 | 36 | "no existing open-source toolchain integrates them into a single URDF-to-URDF pipeline with sidecar primitive parameter export, validation metrics, and presets for reproducible tuning" | Novely claim; requires literature support from citation-survey artifact (not audited here) | wording_risk |

---

## Summary of Issues by Severity

### UNSUPPORTED (1)

| Claim | File:Line | Issue |
|-------|-----------|-------|
| Capsule default runtime = 33.0 s (Table 2) | experiments.tex:78 | Mode comparison artifact CSV shows 33.843 s. The paper's value of 33.0 s matches the ablation experiment's default variant, NOT the mode comparison experiment. Table 2 is the mode comparison table, so it should report 33.8 s (the mode comp value). Alternatively, all mode comparison runtimes were measured from the same experiment and the capsule default happened to be 33.843s. |

### PARTIALLY SUPPORTED (3)

| Claim | File:Line | Issue |
|-------|-----------|-------|
| "Runtimes ... averaged over three runs" | experiments.tex:55 | Artifact commands show single invocations per preset/variant. No evidence of 3-run averaging in the artifact data or logs. |
| "Runtime spans 12.7 s (single) to 33.0 s (default)" | experiments.tex:90 | Single runtime (12.661 s) rounds to 12.7 s correctly. Default runtime uses the ablation value (33.0 s) rather than mode comparison value (33.843 s). Inconsistent data source. |
| "At N=8, this gap nearly disappears (to single-digit micrometers)" | experiments.tex:136-137 | Aggregate worst-case at N=8 is 1.20 mm (1200 µm), not single-digit micrometers. The claim may refer to a specific boundary gap not measurable from aggregate data. No per-link data exists to verify. |

### WORDING RISK (11)

| Claim | File:Line | Issue |
|-------|-----------|-------|
| "two additional approximation modes for completeness" | introduction.tex:33 | "Completeness" implies the set of modes is exhaustive, which may overstate |
| "near-perfect coverage" (sphere single) | experiments.tex:88 | 2e-5m gap IS very tight, but `all_covered=False` -- subjective qualifier |
| "small uncovered regions" (sphere default) | experiments.tex:89 | 1.8e-2 m = 18 mm gap; "small" relative to link size but subjective |
| "tightest approximation" (convex mode) | discussion.tex:11, system_overview.tex:33 | Convex hull is the tightest vertex-enclosing convex approximation, but the comparative "tightest" could be read as absolute across all modes. No volume ratio metric for convex to substantiate. |
| "best choice" (convex mode) | discussion.tex:11 | Recommendation, not quantified |
| "tighter overall coverage than capsules" (sphere tree on concave shapes) | discussion.tex:16 | Not tested on concave shapes; FR3 links are tubular |
| "best choice" (capsule mode) | discussion.tex:19 | Recommendation, not quantified |
| "COA threshold and δ_min already prevent excessive radius variation before ρ_max becomes active" | experiments.tex:142 | Plausible interpretation but not directly measured |
| "conservative collision claims are required" | method.tex:25 | Mentions conservativeness without defining threshold |
| "no existing open-source toolchain" | related_work.tex:36 | Generality claim requiring broader literature verification |
| "tightest mesh-based approximation" | system_overview.tex:33 | See D1/S3 |

---

## Detailed Discrepancy: Capsule Default Runtime (Unsupported)

**Location:** `experiments.tex` Table 2, line 78, and text line 90
**Claim:** Capsule default runtime = 33.0 s
**Mode comparison artifact value:** 33.843 s (CSV), 33.8 s (RESULT.md summary)
**Ablation artifact value:** 33.0 s (for the N=4 default variant)

The mode comparison experiment (exp-fr3-mode-comparison) measured the default preset at 33.843 s. This is the experiment that Table 2 reports. The paper uses 33.0 s, which is the value from the ablation experiment (exp-capsule-ablation). The mode comparison artifact's summary table also shows 33.8 s. Something caused the paper to use the wrong value.

**Required writer action:** Change Table 2 capsule default runtime from 33.0 to **33.8** (rounding from 33.843). Update text line 90 accordingly: "Runtime spans 12.7 s (single) to 33.8 s (default)."

---

## Required Writer Edits

### Unsupported Fixes (Must Fix)

| File | Line | Current text | Corrected text | Reason |
|------|------|-------------|---------------|--------|
| `experiments.tex` | 78 | `Capsule & default & 10 & 1.47 & 1.50 & 33.0` | `Capsule & default & 10 & 1.47 & 1.50 & 33.8` | Mode comparison artifact shows 33.843 s |
| `experiments.tex` | 90 | `Runtime spans 12.7~s (\emph{single}) to 33.0~s (\emph{default})` | `Runtime spans 12.7~s (\emph{single}) to 33.8~s (\emph{default})` | Same discrepancy |

### Partially-Supported Edges (Consider Fixes)

| File | Line | Issue | Suggested action |
|------|------|-------|-----------------|
| `experiments.tex` | 55 | Claims "averaged over three runs" but artifact shows single runs | Weaken to "reported as wall-clock time from a single run with Docker startup overhead included" OR generate 3-run average data and update artifact |
| `experiments.tex` | 90 | Table 2 text cites 33.0 but should be 33.8 | Fix as above |
| `experiments.tex` | 136-137 | "single-digit micrometers" at N=8 boundary gap | Weaken to "nearly disappears" without specifying single-digit micrometers, OR add per-link data to confirm |

### Wording Risk Edits (Recommended)

| File | Line | Issue | Suggested action |
|------|------|-------|-----------------|
| `experiments.tex` | 88 | "near-perfect coverage" for sphere single | Weaken to "very tight coverage (20 micron worst-case gap)" to be precise |
| `experiments.tex` | 89 | "small uncovered regions" for sphere default | Keep but add magnitude reference: "uncovered regions (worst 18 mm)" already present |
| `discussion.tex` | 11 | "geometrically tightest approximation" | Add qualifier: "geometrically tightest mesh-based vertex-enclosing approximation" |
| `discussion.tex` | 16 | "tighter overall coverage than capsules on shapes with deep concavities" | This was not tested on the FR3. Add: "which on the FR3's tubular links was not observed, but on shapes with deep concavities sphere trees may achieve tighter coverage" |
| `related_work.tex` | 36 | "no existing open-source toolchain" | Add scoping: "To the best of our knowledge, no existing open-source toolchain" |

---

## Verification: Artifact Rows vs Table Rows

### Table 2 vs Mode Comparison Artifact CSV

| Artifact CSV row | Matches Table 2 row? | Notes |
|-----------------|---------------------|-------|
| convex,default,8,True,0,,,0.468 | Yes (0.5 rounded) | |
| sphere,single,8,False,2e-05,,,10.104 | Yes (10.1 rounded) | |
| sphere,default,63,False,0.017608,,,40.719 | Yes (40.7 rounded) | |
| capsule,single,8,False,0.006689,1.3684,1.2684,12.661 | Yes | All rounded values consistent |
| capsule,default,10,False,0.010226,1.47,1.4966,33.843 | **No** | Runtime 33.843 vs paper's 33.0 |
| capsule,high_detail,10,False,0.002338,1.3245,1.3225,21.797 | Yes | All rounded values consistent |

### Table 3 vs Ablation Artifact

All 40 numeric cells in Table 3 match the ablation artifact. No discrepancies.

---

## Notes on Data Gaps

1. **No per-link data exists** in either artifact (`per_link_metrics.csv` was not generated). All aggregate claims are based on worst-case metrics, which is appropriate, but any claim about specific links (e.g., the N=4 boundary gap affecting one specific link) cannot be verified.
2. **No averaging evidence**: The artifact commands show single invocations. If runtimes truly were averaged over 3 runs, the artifact should document this explicitly.
3. **The ablation artifact reports 33.0 s** for the default (N=4) variant but the mode comparison artifact reports 33.843 s for the default preset. These are separate measurements of the same configuration. Runtime variability across separate Docker invocations (~0.8 s) is plausible given Docker startup overhead (1-2 s claimed in the paper). The inconsistency in which value the paper uses for Table 2 is the primary issue.
