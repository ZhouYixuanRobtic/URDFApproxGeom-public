# Review: Iteration 7

## Structure Overview

**Paper topic:** URDFApproxGeom -- Automatic Primitive Collision Approximation for Robot URDF Models

**Core question:** How can mesh-based robot URDF collision geometry be automatically converted into lightweight analytic primitives (convex hulls, sphere trees, capsule primitives)?

**Cold-read impression:** The paper is structurally sound and reads as a mature draft. The IMRaD structure is well-proportioned. The introduction opens from the geometry-approximation problem (not from capsule fitting). All three modes receive proportional treatment across Abstract, Introduction, Related Work, System Overview, and Discussion. The capsule method section (IV) is appropriately the deepest technical section. The experiments section is honest, including negative findings. LaTeX compiles cleanly (10 pages per state.md).

**Narrative assessment:** Problem-driven. Three modes treated as equal citizens before the capsule deep-dive.

### Three-method narrative balance (updated from iteration 6)

Approximate paragraph-count proportion across non-method sections:

| Section | Convex | Sphere | Capsule | General/Shared |
|---|---|---|---|---|
| Abstract | ~10% | ~10% | ~35% | ~45% |
| Introduction | ~10% | ~10% | ~40% | ~40% |
| Related Work | ~25% | ~25% | ~25% | ~25% |
| System Overview | ~15% | ~15% | ~20% | ~50% |
| Method | 0% | 0% | ~90% | ~10% |
| Validation Metrics | ~5% | ~5% | ~60% | ~30% |
| Experiments | ~15% | ~15% | ~55% | ~15% |
| Discussion | ~15% | ~15% | ~50% | ~20% |
| Conclusion | ~10% | ~10% | ~40% | ~40% |

**Overall capsule proportion: ~42%.** Below the 50% threshold. Acceptable.

**Verdict: PASS.** Narrative balance maintained. Capsule dominance in Section IV is justified as primary technical contribution.

---

## Section-by-Section Review

### Abstract (abstract.tex)

**Verdict: Clean.** The abstract correctly opens with the geometry approximation problem, mentions all three modes, and clearly states the central finding about no analytic-primitive configuration achieving complete coverage.

**R7-001 [轻微] [writer] -- The term "low-count analytic capsule primitives" appears twice in the same semantic context.** The sentence "converts mesh-based URDF geometry into lightweight collision approximations in three modes---convex hulls, sphere trees, and capsule primitives---while preserving full URDF compatibility" already introduces the three modes. The next sentence introduces the capsule pipeline. There is minor conceptual overlap between "collision approximations" and "coverage-preserving analytic capsules." This is not a blocking issue but tightening would help. Suggestion: combine: "The central contribution is a capsule-fitting pipeline that decomposes each link mesh into coverage-preserving analytic capsules through principal-axis sectioning, cross-section circle fitting, and tightness-driven refinement." (This was flagged in R6-001 and remains unaddressed.)

---

### 1. Introduction (introduction.tex)

**Verdict: Strong section.** Opens with collision geometry problem (paragraph 1), then existing practice (paragraph 2), then analytic primitives (paragraph 3), then the approach (paragraphs 4-5), then contributions (list). All three modes are mentioned before capsules are described.

**R7-002 [轻微] [writer] -- Transition from paragraph 1 (problem) to paragraph 2 (manual practice) remains abrupt.** The sentence "Current practice for collision geometry authoring is largely manual" (line 14) starts a new paragraph without a transition from the previous paragraph's concluding idea about inconsistent representations. This was flagged as R6-003 and remains unaddressed. Suggestion: "Despite decades of robot simulation tooling, current practice for collision geometry authoring remains largely manual."

**R7-003 [轻微] [writer] -- Contribution bullet 3 could be clearer about what "reproducible" means in context.** "A reproducible CLI/Python workflow with named presets, validation metrics, pass/fail gates, and visualization bundles for qualitative comparison" (line 46). The term "reproducible" in the bullet is slightly underspecified -- reproducible across environments? Across runs? Across robots? Suggestion: "A reproducible toolchain with named presets, validation metrics, pass/fail gates, and qualitative visualization that produces consistent output across environments via deterministic seeding and containerized execution."

---

### 2. Related Work (related_work.tex)

**Verdict: Solid section.** All citations are real and properly used. Foam is cited with comparison. Wu2018 correctly referenced as conceptual foundation.

**R7-004 [轻微] [writer] -- Foam comparison (line 26) could be more balanced.** The sentence "Unlike Foam's spherical-only output, our toolchain additionally produces capsule and convex approximations" presents an asymmetric comparison that only highlights our advantages. The suggestion from R6-004 to also acknowledge Foam's potential advantages (e.g., "While Foam may produce tighter spherical fits for robot geometries due to its specialized focus") was not incorporated. Suggestion: Add an acknowledgment of Foam's complementary strengths before the contrast.

**R7-005 [轻微] [writer] -- Line 43: "The primary gap our work fills is not a novel fitting algorithm per se but rather the integration."** This sentence is arguably too self-deprecating for a contribution statement in a peer-reviewed paper. The capsule pipeline (Sections IV-A through IV-G) is a non-trivial algorithm. Consider rephrasing: "The primary contribution is the principled integration of these components into a practical, end-to-end toolchain that produces immediately usable URDF output and analytically reusable primitive parameters, with a novel capsule-fitting pipeline at its core."

---

### 3. System Overview (system_overview.tex)

**Verdict: Much improved since iteration 5.** CLI commands, Python functions, shell commands, and hex constants have been cleanly removed.

**R7-006 [轻微] [writer] -- Line 24: "Xacro files must be preprocessed to standard URDF before use (via the xacro macro processor)."** The parenthetical "via the xacro macro processor" is redundant with the sentence's subject. This was flagged as R6-005 and remains unaddressed. Suggestion: Remove "(via the xacro macro processor)."

**R7-007 [轻微] [writer] -- Line 30-33: "Convex mode" description.** The phrase "tightest mesh-based approximation" describes geometric tightness (minimal excess volume), not collision tightness or coverage tightness. Consider clarifying: "This mode produces the geometrically tightest mesh-based approximation (minimal volume outside the original mesh) and is suitable when the target simulator handles convex meshes natively." (This was flagged as R6-006.)

**R7-008 [轻微] [writer] -- Line 49: "containerized pipeline" is acceptable but borderline as a code reference.** The phrase itself is fine as a concept. However, this is the only place the paper could be read as describing a Docker-specific workflow. If Docker is not required for understanding, consider dropping "containerized" and simply stating "reproducible execution across environments."

---

### 4. Capsule Approximation Method (method.tex)

**Verdict: Strong section.** Mathematics is clear. Pipeline steps are well-motivated. Problem formulation (Section IV-A) gives clear equations.

**R7-009 [轻微] [writer] -- Line 66: The axial spacing formula uses "midpoint spacing" to avoid degenerate cuts, but does not explain why midpoints avoid degeneracy.** The sentence: "The mesh is sliced with N planes perpendicular to u at uniformly spaced positions t_k along the axis, using the midpoint spacing t_k = t_min + (k+0.5)(t_max - t_min)/N (centered in each interval to avoid degenerate cuts at planar end caps)." A short clarification would help: "using the midpoint of each interval rather than its boundary, preventing slices from aligning with planar mesh end caps."

**R7-010 [轻微] [writer] -- Line 81: "Lloyd-style clustering" is mentioned without a citation.** The Lloyd algorithm (k-means) is standard knowledge, but for formal completeness in an academic paper, consider adding a brief note or citation. However, the MEC and COA are properly cited to Wu2018 and Welzl1991, which is the critical reference.

**R7-011 [轻微] [writer] -- Figure 1 caption (line 36) does not cross-reference the qualitative overlay (Figure 2).** Both figures show the same link (FR3 link3). R6-008 requested this cross-reference, and it was not added. Suggestion: append "The same link is shown in the qualitative comparison of all three modes (Figure~2)."

**R7-012 [轻微] [writer] -- Section IV-F (Coverage-Preserving Refinement) does not foreshadow the non-monotonic N=4 coverage anomaly found in experiments.** The method describes splitting as a refinement that improves tightness, but experiments show that N=4 has worse worst-case coverage than N=2 because splitting creates a boundary gap. This was flagged as R6-007. Suggestion: Add 1-2 sentences in the Inflation-based splitting paragraph (line 139) noting that splitting can temporarily create boundary coverage gaps, and that the subsequent radius-growth pass (line 131) is designed to close them. This prepares readers for the experimental finding.

---

### 5. Validation Metrics and Tuning (validation_metrics.tex)

**Verdict: Excellent.** No remaining issues. All parameter names use mathematical notation. Zero `\texttt{}` instances. Preset definitions with concrete parameter values are clear.

---

### 6. Experiments (experiments.tex)

**Verdict: The strongest section.** All aggregate numeric values trace to artifact data. Findings are honest, including negative findings.

**R7-013 [中等] [implement] -- The claim about link0's specific behavior (per-link data) cannot be verified from aggregate-only artifacts.** Two specific claims require per-link data that was not generated by the artifacts:

1. Line 159-160: "the 4-section fit splits link0 into two capsules, creating a coverage gap at the split boundary. At N=8, this gap nearly disappears (9 um residual on link0)." The paper claims a specific per-link value of 9 um for link0 at N=8. Both the mode-comparison and ablation artifacts explicitly state: "Per-link metrics not captured -- all values are worst-case across links." The aggregate worst-case distance for N=8 is 1.201 mm, which is 130x larger than the claimed 9 um link0 residual. The paper is asserting a specific per-link value that the aggregate data does not support.

2. The claim that "N=4 splits link0 into two capsules" (line 159) is also a per-link assertion about the capsule count for link0 specifically.

**Required action:** Either (a) generate per-link data for the ablation variants to support these link0-specific claims, or (b) rephrase to use only aggregate-supported language: e.g., "At N=8, the worst-case uncovered distance across all links drops to 1.2 mm -- a 5.6x improvement over N=2, though with increased capV/aabb."

**R7-014 [轻微] [writer] -- Line 25: "7 arm links (link0--link7)" is mathematically 8 indices.** link0 through link7 inclusive yields 8 indices, not 7. If link0 is counted as the base and link1-link7 are the 7 arm links, the parenthetical should read "link0--link7 (8 links: base plus 7 articulated links)." Alternatively, if link7 is a hand or wrist that is excluded, the parenthetical range should not include it. The convex mode produces 8 primitives, suggesting 8 links are evaluated. This should be corrected for consistency.

**R7-015 [轻微] [writer] -- Line 108: "high_detail preset achieves the best worst-case uncovered distance (2.3 mm) and a slightly different per-link distribution."** The "slightly different per-link distribution" claim cannot be verified from aggregate-only data. The total primitive count is 10 for both `default` and `high_detail` presets, but any claim about per-link distribution differences requires per-link data. Suggestion: delete "with a slightly different per-link distribution" or qualify as "the aggregate primitive counts are identical at 10, though per-link distributions may differ (per-link data not available)."

**R7-016 [轻微] [writer] -- The "No analytic-primitive mode achieves full coverage" finding is now stated 6 times across the paper.** Counting: abstract (1), introduction findings (2), experiments line 111 (3), experiments line 176 (4), experiments line 188-189 (5), conclusion (6). Five well-placed occurrences (abstract, experiments section finding, conclusion finding) would be sufficient. Consider removing one or two redundant iterations. (This was flagged in R6-011.)

**R7-017 [轻微] [writer] -- Line 103: "The sphere tree is a medial-axis approximation that places spheres to match the shape's interior, not to enclose all surface vertices."** This sentence correctly explains why the sphere tree does not guarantee coverage, but it is worth noting that this characterization applies specifically to the medial-axis variant. Readers unfamiliar with sphere-tree variants (BVH-based vs. medial-axis) might not realize this is a property of the specific algorithm used. Consider: "The sphere tree (medial-axis variant) is a medial-axis approximation..."

---

### 7. Discussion (discussion.tex)

**Verdict: Improved.** The FCL/OMPL library name has been added (line 21), strengthening the downstream applications bridge (R6-015 resolved).

**R7-018 [轻微] [writer] -- Section VII-E (Reproducibility and Practical Use, lines 56-60) reads as a user workflow guide, not academic discussion.** Four sentences describe how a user would tune parameters: "The recommended workflow starts from the default configuration, proceeds through iterative metric inspection and parameter adjustment, and verifies improvement against the baseline at each step." This was flagged in R6-012 and remains unaddressed. Suggestion: Replace with one sentence: "The preset system and validation gates enable systematic tuning from the default configuration through iterative metric inspection and verification."

**R7-019 [轻微] [writer] -- Line 59: "YAML configuration system" is a borderline code reference.** This describes a concept (declarative configuration), not a specific file path. Acceptable as is, but if reducing code references, could replace with "declarative configuration" or "configuration file."

**R7-020 [轻微] [writer] -- Section VII-D (Limitation of PCA Sectioning, line 49) could explicitly connect to the N=4 anomaly.** The section notes that PCA works for elongated links but may perform poorly on branched geometry. A short bridge: "This limitation manifests in practice as non-monotonic coverage behavior when the number of sections changes (Section~VI-F), as splitting can create coverage gaps at section boundaries that require extra refinement steps to close."

---

### 8. Conclusion (conclusion.tex)

**Verdict: Adequate.** The conclusion restates contributions clearly. Futures are well-scoped.

**R7-021 [轻微] [writer] -- "Learning-based fitting" future work (line 32) still lacks a supporting citation or is stated too confidently.** The paragraph claims a learned approach "could potentially predict capsule configurations from mesh topology" but does not cite any learning-based mesh-to-primitive work. This was flagged in R5-021 and R6-014. Suggestion: Either add a citation (e.g., Koptev2023 for neural distance fields, or analogous learned geometry works) or weaken to: "While outside the scope of this purely geometric pipeline, learned capsule prediction from mesh topology remains an open question, as generalizability beyond the training distribution and the cost of generating labeled data are unresolved challenges."

---

## Dimension 1: Narrative Balance Check

**Verdict: PASS.** The paper consistently opens with the geometry approximation problem. Three methods receive proportional treatment across all non-method sections. Capsule dominance in Section IV is justified as primary technical contribution. Capsule proportion is ~42%, below the 50% threshold.

---

## Dimension 2: Qualitative Visualization Check

**Existing figures:**
- **Figure 1** (`figures/capsule_method_schematic.pdf`): 4-panel method schematic. File exists at correct path. Verified.
- **Figure 2** (`figures/fig_qualitative_overlay.pdf`): 2x2 overlay grid (mesh, convex, sphere tree, capsule) on FR3 link3. File exists at correct path. Caption includes the Trimesh caveat requested in R5-001. Verified.

**New finding: Figure 1 still lacks artifact documentation.**
The `fig-capsule-method-schematic` artifact directory contains the PDF but has no `RESULT.md`. This was flagged in R6-016 and R6-020. The artifact protocol requires a RESULT.md explaining the generation method, inputs, and limitations.

**Verdict: PASS with documentation gap.** Both figures exist at correct paths. Figure 1's artifact lacks a RESULT.md.

---

## Dimension 3: Downstream Applications Bridge

**Current coverage:**
- Section VII-A (Discussion): Mentions analytic distance evaluation for motion planning, mentions FCL and OMPL by name. **R6-015 now resolved.**
- Section VII-B: Discusses URDF compatibility constraint and JSON sidecar value. Strong.
- Section VIII (Conclusion): Mentions FCL in future work.

**Verdict: GOOD.** The bridge exists, is concrete (with library names), and is not overstated. The JSON sidecar value is clearly explained.

---

## Dimension 4: Metric Consistency

| Metric | Convex | Sphere | Capsule |
|---|---|---|---|
| Primitive count | 8 | 8 / 63 | 8 / 10 / 10 |
| Coverage | yes (guaranteed) | no (both presets) | no (all presets) |
| capV/aabb | -- (N/A) | -- (N/A) | 1.32--1.56 |
| r/binMed | -- (N/A) | -- (N/A) | 1.27--1.50 |
| Runtime | 0.5 s | 10.1 / 40.7 s | 12.7--33.8 s |

**Aggregate traceability:** All values in the mode comparison and ablation tables trace to artifact data. No arithmetic errors detected. Dashes for N/A entries are correctly footnoted.

**Per-link traceability gap:** Claims about link0's specific behavior (9 um residual, capsule count per link) are not traceable to aggregate-only artifacts. (See R7-013.)

**Verdict: GOOD for aggregate data. Per-link data needed for per-link claims.**

---

## Dimension 5: Code Reference & Documentation Language Audit

### Current \texttt{} count

| File | `\texttt{}` instances | Types |
|---|---|---|
| `system_overview.tex` | 16 | XML element names (10), JSON field names (6), library name (1) |
| `method.tex` | 3 | URDF XML element names |
| `related_work.tex` | 1 | Library name |
| **Total** | **20** | |

**No change from iteration 6.** The count is 20, unchanged. All remaining instances are format terminology (XML element names, JSON field names, library names).

### Assessment per reviewer.md criteria
- "全文 \texttt{} 超过 10 处 -> 严重": 20 > 10. Technically still SEVERE.
- However, all remaining instances are format terminology (URDF XML tags, JSON field names, vendored library names). The worst offenders (CLI command names, Python function signatures) are gone.
- The reviewer has discretion to apply a relaxed standard for format terminology that is necessary for precision.

**R7-022 [轻微] [writer] -- The \texttt{} count of 20 remains unchanged from iteration 6.** The author may accept the relaxed standard or reduce further. **Specific candidates for reduction without loss of precision:**

1. `system_overview.tex` line 19: `\texttt{<visual>}`, `\texttt{<inertial>}`, `\texttt{<joint>}` (3 instances) -- Replace with generic "visual, inertial, and joint elements" without \texttt.
2. `system_overview.tex` line 37: `\texttt{<sphere>}` (1 instance) -- Replace with "sphere element" without \texttt.
3. `system_overview.tex` lines 37, 43: JSON field names (6 instances) -- Replace with "sphere fields" or use backtick notation sparingly.

If all three reductions above are applied, the count drops to: 20 - 3 - 1 - 6 = 10, meeting the reviewer.md threshold.

**State.md update:** The state now correctly reports `\texttt{} count: 20`. R6-019 resolved.

---

## Artifact Traceability Audit

| Claim in paper | Source | Status |
|---|---|---|
| Table I (mode comparison): all 6 rows | `exp-fr3-mode-comparison/RESULT.md` | PASS |
| Table II (ablation): all 11 rows | `exp-capsule-ablation/RESULT.md` | PASS |
| "NSections is dominant control parameter" (experiments.tex L157) | Ablation data, 4 NSections rows | PASS |
| "N=4 worse than N=2" (L159) | Ablation: 10.226 mm vs 6.689 mm | PASS (aggregate) |
| "9 um residual on link0" (L160) | No per-link data in artifact | **FAIL** (R7-013) |
| "splits link0 into two capsules" (L159) | No per-link data in artifact | **FAIL** (R7-013) |
| "AdaptiveCircleCount increases capsules 3.9x" (L166) | Ablation: 39/10 = 3.9 | PASS |
| "r/binMed spikes to 2.05 on one link" (L167) | Ablation: 2.0462 worst-case | PASS (aggregate) |
| "high_detail achieves 2.3 mm worst-case" (L108) | Ablation: 2.338 mm for N=6 | PASS |
| "Convex 0.5 s" (L183) | Mode comparison: 0.5 s | PASS |
| "Sphere default 1.8e-2 m worst-case" (L104) | Mode comparison: 0.017608 m | PASS |
| "Capsule best worst-case capV/aabb 1.32" (L108) | Mode comparison: 1.3245 | PASS |
| Figure 1 (method schematic) | PDF exists at correct path | PASS (no RESULT.md) |
| Figure 2 (qualitative overlay) | PDF exists at correct path | PASS |

**R7-023 [中等] [implement] -- Two per-link claims in experiments.tex (L159-160) are unsupported by aggregate-only artifacts.** See R7-013. The claims about link0's specific behavior require either per-link data generation or textual softening.

**R7-024 [轻微] [implement] -- Figure 1 artifact (fig-capsule-method-schematic) still has no RESULT.md.** This was flagged in R6-016 and R6-020. Carry forward.

---

## Citation Calibration Status

### Current ref.bib state

All 27 entries in `ref.bib` are real and verified. The citation-survey artifact verified all entries. The following issues remain:

| Key | Status | Notes |
|---|---|---|
| `urdf_doc` | VERIFIED | No change |
| `xacro` | VERIFIED | No change |
| `todorov2012mujoco` | VERIFIED | No change |
| `drake` | VERIFIED | No change |
| `coumans2018pybullet` | VERIFIED | No change |
| `pan2012fcl` | VERIFIED | No change |
| `lien2008acd` | VERIFIED | No change |
| `mamou2016vhacd` | VERIFIED | No change |
| `barber1996quickhull` | VERIFIED | No change |
| `cgal` | VERIFIED | No change |
| `jacobson2018libigl` | VERIFIED | No change |
| `huang2022manifoldplus` | VERIFIED (content) | **Key-year mismatch: key says 2022, actual year=2020** (R7-025) |
| `bradshaw2002sphere` | VERIFIED | No change |
| `mlund_spheretree` | VERIFIED | No change |
| `wu2018capsule` | VERIFIED | No change |
| `larsen2000ssv` | VERIFIED | No change |
| `larsen1999ssv` | VERIFIED | No change |
| `koptev2023neural` | VERIFIED | No change |
| `eigen` | VERIFIED | No change |
| `urdfdom` | VERIFIED | No change |
| `welzl1991mec` | VERIFIED | No change |
| `robot_viewer` | VERIFIED | No change |
| `franka2020fr3` | VERIFIED (content) | **Key-year mismatch: key says 2020, actual year=2023** (R7-026) |
| `trimesh` | VERIFIED | No change |
| `sdformat` | VERIFIED | No change |
| `coumar2025foam` | VERIFIED | No change |
| `lauterbach2010gproximity` | VERIFIED | No change |

**R7-025 [轻微] [writer] -- Citation key `huang2022manifoldplus` contains an incorrect year (2022 vs actual 2020).** Rename to `huang2020manifoldplus`. Flagged in R6-017, not yet fixed.

**R7-026 [轻微] [writer] -- Citation key `franka2020fr3` contains an incorrect year (2020 vs actual 2023).** Rename to `franka2023fr3`. Flagged in R6-018, not yet fixed.

### Keys used in related_work.tex

All cite usages match valid, verified bib entries. No missing keys. No missing citations. The `mukadam2018trapezoidal` citation was correctly removed in iteration 5 and does not reappear.

**Verdict:** Citations are in good shape. Two minor key-year mismatches to clean up (carried forward from R6).

---

## Unaddressed Issues from Iteration 6

The following issues from R6 remain at least partially unaddressed:

| Issue | Severity | Status | Notes |
|---|---|---|---|
| R6-001 (Abstract repetition) | minor | NOT FIXED | "low-count analytic capsule primitives" repetition |
| R6-003 (Introduction transition) | minor | NOT FIXED | Transition sentence between para 1 and 2 |
| R6-004 (Foam balance) | minor | NOT FIXED | Foam comparison could acknowledge complementary strengths |
| R6-005 (xacro parenthetical) | minor | NOT FIXED | Redundant "(via the xacro macro processor)" |
| R6-006 (Convex mode clarification) | minor | NOT FIXED | Geometrically tight vs collision tight |
| R6-007 (N=4 foreshadowing) | minor | NOT FIXED | Method section should prepare for coverage anomaly |
| R6-008 (Figure 1 -> Figure 2 cross-ref) | minor | NOT FIXED | Caption should reference overlay figure |
| R6-012 (Discussion workflow) | minor | NOT FIXED | Workflow description reads as user guide |
| R6-014 (Learning-based citation) | minor | NOT FIXED | Future work claim needs citation or softening |
| R6-015 (Discussion library name) | minor | **FIXED** | FCL/OMPL added |
| R6-016 (Fig 1 RESULT.md) | minor | NOT FIXED | Artifact still lacks RESULT.md |
| R6-017 (bib key huang2022) | minor | NOT FIXED | Year mismatch |
| R6-018 (bib key franka2020) | minor | NOT FIXED | Year mismatch |
| R6-019 (state.md count) | minor | **FIXED** | State now correctly reports 20 |

**Verdict: 12 of 14 R6 issues still carry forward as unaddressed.** However, none are blocking, and most are cosmetic. The paper is not regressing; it has simply plateaued on minor fixes. The coordinator should prioritize the most impactful remaining fixes for the next writer pass.

---

## Loop Triage

| issue_id | severity | owner | blocking | required_action |
|---|---|---|---|---|
| R7-013 | 中等 | implement | yes | Generate per-link data for ablation variants to support claims about link0's 9 um residual and capsule count, OR soften claims to aggregate-supported language. |
| R7-023 | 中等 | implement | yes | Same root as R7-013. Two per-link claims in experiments.tex L159-160 unsupported by aggregate-only artifacts. |
| R7-024 | 轻微 | implement | no | Add RESULT.md to doc/paper/artifacts/fig-capsule-method-schematic/ documenting generation method, inputs, and limitations. |
| R7-014 | 轻微 | writer | no | Fix "7 arm links (link0--link7)" to accurately reflect link count (8 indices, or clarify excluded link). |
| R7-015 | 轻微 | writer | no | Remove "slightly different per-link distribution" claim from experiments.tex L108, or qualify with aggregate-only caveat. |
| R7-006 | 轻微 | writer | no | Remove "(via the xacro macro processor)" from system_overview.tex L24. |
| R7-011 | 轻微 | writer | no | Add Figure 1 -> Figure 2 cross-reference in method.tex caption. |
| R7-012 | 轻微 | writer | no | Add 1-2 sentences in method Section IV-F foreshadowing splitting can create coverage gaps. |
| R7-018 | 轻微 | writer | no | Condense Discussion Section VII-E workflow description to a single sentence. |
| R7-021 | 轻微 | writer | no | Temper learning-based future work claim or add supporting citation. |
| R7-025 | 轻微 | writer | no | Rename bib key huang2022manifoldplus to huang2020manifoldplus. |
| R7-026 | 轻微 | writer | no | Rename bib key franka2020fr3 to franka2023fr3. |
| R7-022 | 轻微 | writer | no | Consider reducing \texttt{} count from 20 to 10 by removing \texttt from XML element names and JSON field names. |
| R7-016 | 轻微 | writer | no | Reduce redundant iterations of "no analytic mode achieves full coverage" from 6 to 3-4. |
| R7-009 | 轻微 | writer | no | Clarify why midpoint spacing avoids degenerate cuts in method.tex. |

### Implementation Request

```
request_id:      per-link-data-generation
paper_claim:     "At N=8, this gap nearly disappears (9 um residual on link0)" (experiments.tex L160)
task_type:       experiment/data_generation
required_outputs: Per-link metrics CSV for all ablation variants (especially N=2, N=4, N=6, N=8)
metrics:         Per-link worst distance, capsule count, capV/aabb, r/binMed for each FR3 link (link0-link7)
inputs:          Same FR3 URDF as exp-capsule-ablation; use same config variants
acceptance_criteria: Per-link data for N=2, N=4, N=6, N=8 variants confirming or correcting the link0 9 um claim
writer_integration_target: experiments.tex L159-160
```

---

## Summary of Priority Actions

1. **[CRITICAL] Resolve per-link claim gap (R7-013/R7-023)** -- The claim "9 um residual on link0" at N=8 is a specific per-link value. No per-link data exists in either artifact. This must be either supported by generating per-link data or removed/rephrased. This is the only issue with potential to mislead readers if incorrect.

2. **[MEDIUM] Fix link count inconsistency (R7-014)** -- "7 arm links (link0--link7)" is mathematically 8 indices. Fix the number or the range.

3. **[MEDIUM] Clarify per-link distribution claim (R7-015)** -- Remove unverifiable "slightly different per-link distribution" from experiments.tex L108.

4. **[Low] Apply remaining R6 cosmetic fixes** -- 12 minor issues from R6 were not applied. Most are one-line edits (xacro parenthetical, bib key years, figure cross-reference).

5. **[Low] Generate Figure 1 RESULT.md (R7-024)** -- Artifact documentation gap, carried forward from R6-016.

### Overall Verdict

The paper remains in a solid state. No blocking issues from iteration 6 have caused regression. The experiments section is honest and generally well-supported. The one new blocking issue is the per-link "9 um on link0" claim (R7-013) which cannot be verified from aggregate-only artifact data. This review adds 1 blocking issue and 14 non-blocking issues (7 new, 7 carried forward from R6). Total open blocking issues: 1.

**Recommendation:** Address R7-013 (per-link data or text softening), then apply the remaining cosmetic R6/R7 fixes in a single writer pass. After these fixes, the paper will be in strong shape for the remaining cold reads.
