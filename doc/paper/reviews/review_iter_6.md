# Review: Iteration 6

## Structure Overview

**Paper topic:** URDFApproxGeom -- Automatic Primitive Collision Approximation for Robot URDF Models

**Core question:** How can mesh-based robot URDF collision geometry be automatically converted into lightweight analytic primitives (convex hulls, sphere trees, capsule primitives)?

**Cold-read impression:** The paper has matured significantly since iteration 5. The IMRaD structure is sound and well-proportioned. The introduction opens with the general collision-geometry problem in robotics, not with capsule fitting. All three modes receive proportional treatment in Abstract, Introduction, Related Work, System Overview, and Discussion. The capsule method section (IV) is appropriately the technical core. The experimental section is complete and honest about findings, including negative results (no analytic mode achieves full coverage).

**LaTeX compilation:** PASS -- compiles cleanly to 10 pages with no errors. Both figures resolve correctly.

**Narrative assessment:** Problem-driven, not tool-driven. Three modes treated as equal citizens before capsule deep-dive.

### Three-method narrative balance

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

**Verdict: PASS.** Narrative balance is maintained. Capsule dominance in Section IV is justified as the primary technical contribution.

---

## Section-by-Section Review

### Abstract (abstract.tex, 1 line)

**R6-001 [轻微] [writer] -- "low-count analytic capsule primitives" is repeated twice.** The abstract states "into coverage-preserving analytic capsules" (method description) followed by the same concept in "produces coverage-preserving, low-count analytic capsule primitives" in the contribution description. Consider merging: "The central contribution is a capsule-fitting pipeline that decomposes each link mesh into coverage-preserving analytic capsules through principal-axis sectioning, cross-section circle fitting, and tightness-driven refinement."

**R6-002 [轻微] [writer] -- "finding that no analytic-primitive configuration achieves complete vertex coverage" still needs parenthetical clarification.** Following R5-005's fix, the abstract now says "no analytic-primitive (sphere and capsule) configuration achieves complete vertex coverage on all links---a fundamental representation trade-off." This is clear. No further change needed. **VERIFIED AS FIXED.**

### 1. Introduction (introduction.tex)

**Verdict: Strong section.** Opens with collision geometry problem (para 1), then existing practice (para 2), then analytic primitives and capsules (para 3), then our approach (paras 4-5), then contributions (list). The three modes are mentioned before capsules are described. Contribution bullet 4 (lines 47-48) now correctly leads with results: "Experimental evaluation on the Franka FR3 robot ... characterizing coverage, volume tightness, primitive count, runtime, and preset sensitivity across all three modes, together with a capsule ablation study." **VERIFIED AS FIXED from R5-007.**

**R6-003 [轻微] [writer] -- Introduction paragraph 2 (line 14) "Current practice for collision geometry authoring is largely manual."** This sentence is the start of a new paragraph but reads as a standalone claim. Consider adding a transition: "Despite the advantages of analytic primitives, current practice for collision geometry authoring remains largely manual."

### 2. Related Work (related_work.tex)

**Verdict: Solid section.** The `mukadam2018trapezoidal` citation has been correctly removed. Foam is cited with an appropriate comparison. The `wu2018capsule` citation correctly points to Nannan Wu IEEE Access 2018. `larsen2000ssv` (ICRA 2000) is used. All 22 cite usages match valid bib entries.

**R6-004 [轻微] [writer] -- Foam comparison (line 26) could still be more balanced.** The sentence reads: "Unlike Foam's spherical-only output, our toolchain additionally produces capsule and convex approximations with analytic parameter export through a JSON sidecar." This contrast is accurate. The suggestion from R5-008 to also acknowledge Foam's potential tighter spherical approximation was not incorporated. Adding "While Foam may produce tighter spherical fits for robot geometries due to its specialized focus, our toolchain additionally produces..." would strengthen the contrast.

### 3. System Overview (system_overview.tex)

**Verdict: Much improved.** CLI commands, Python function signatures, xacro shell command, and hex constant have all been removed. The section now operates at the right abstraction level for a peer-reviewed paper. **This is the biggest positive change since iteration 5.**

**R6-005 [轻微] [writer] -- Line 23: "Xacro files must be preprocessed to standard URDF before use (via the xacro macro processor)."** The parenthetical "via the xacro macro processor" is redundant with the sentence's subject. Remove: "Xacro files must be preprocessed to standard URDF before use."

**R6-006 [轻微] [writer] -- Line 30: "Convex mode."** The description (lines 31-33) uses "tightest mesh-based approximation" but does not clarify that this describes geometric tightness (minimal excess volume), not collision tightness. Consider: "produces the geometrically tightest mesh-based approximation (minimal excess volume outside the original mesh)."

### 4. Capsule Approximation Method (method.tex)

**Verdict: Strong section.** Mathematics is clear. Pipeline steps are well-described. Problem formulation (Section IV-A) gives clear equations. The PCA sectioning, circle fitting, capsule construction, and refinement are each well-motivated.

**R6-007 [轻微] [writer] -- The $N=4$ non-monotonic coverage anomaly (experiments Section VI-F) is not foreshadowed in the method section.** The method describes splitting as a refinement that improves tightness. But the experiments reveal that splitting can create temporary coverage gaps (link0 at N=4 has worse coverage than N=2). Consider adding 1-2 sentences to Section IV-F (Coverage-Preserving Refinement) noting that splitting can temporarily create boundary gaps, and that the subsequent radius-growth pass is designed to close them. This would prepare readers for the non-monotonic experimental finding.

**R6-008 [轻微] [writer] -- Figure 1 caption (line 36) does not cross-reference Figure 2.** Both figures show the same link (FR3 link3). Adding "The same link is shown in the qualitative comparison of all three modes (Figure 2)" would provide useful navigation.

### 5. Validation Metrics and Tuning (validation_metrics.tex)

**Verdict: Excellent.** No remaining issues. All parameter names use mathematical notation. Zero `\texttt{}` instances. Preset definitions with concrete parameter values are clear and useful.

### 6. Experiments (experiments.tex)

**Verdict: The experiments section is the strongest part of the paper.** All numeric values trace to artifact data. The findings are honest, including negative findings (no analytic mode achieves full coverage, N=4 is anomalously worse than N=2, capsule budget is not the active constraint on FR3).

**R6-009 [中等] [writer/implement] -- The "link2" claim has been softened to "on one link" (line 167), which is verifiable from aggregate worst-case data.** The aggregate artifact shows r/binMed = 2.0462 for the adaptive variant. Saying "on one link" without specifying which is supported by the aggregate data. **RESOLVED.** The paper now accurately reflects the available evidence.

**R6-010 [轻微] [writer] -- Section VI-E, "Capsule modes output 8--10 primitives" (line 107).** Table II shows exactly 8, 10, and 10 primitives for single, default, and high_detail presets. The range "8--10" is correct. However, the ablation table shows the N=8 variant produces 11 primitives. Consider changing "8--10" to "8--11" for consistency with the ablation results, or limiting to the three presets shown in Table I.

**R6-011 [轻微] [writer] -- Line 114: "Analytic primitives inevitably involve trade-offs between tightness and coverage."** This is stated five times across the paper (abstract, introduction findings, experiments line 111, line 176, line 188). This is the paper's central negative finding, which is fine to emphasize, but consider whether the repetition is necessary in all five locations. Three well-placed occurrences (abstract, experiments finding, conclusion) would be sufficient.

### 7. Discussion (discussion.tex)

**Verdict: Improved.** The `\texttt{validate}` and `\texttt{compare}` CLI references have been removed. **VERIFIED AS FIXED from R5-018.**

**R6-012 [轻微] [writer] -- Section VII-E ("Reproducibility and Practical Use") still reads partially as a workflow guide.** Lines 57-60 describe "the recommended workflow" -- start with default, inspect metrics, adjust, verify. This reads as user documentation, not academic discussion. Consider replacing with a single sentence: "The preset system and validation gates enable systematic tuning from the default configuration through iterative metric inspection and verification." Remove the three-sentence workflow description.

**R6-013 [轻微] [writer] -- Section VII-A ("When to Use Each Mode") lacks a concrete downstream library name.** The sentence "making them suitable for collision-checking in motion planning, real-time simulation, and differentiable collision pipelines" (line 21) could name one concrete library: "making them suitable for collision-checking in frameworks such as FCL~\cite{pan2012fcl} or OMPL." This would make the bridge more tangible for robotics readers.

### 8. Conclusion (conclusion.tex)

**R6-014 [轻微] [writer] -- "Learning-based fitting" future work (line 32) still lacks a citation.** The paragraph states: "a learned approach could potentially predict capsule configurations from mesh topology, though generalizability beyond the training distribution and the cost of generating labeled training data remain open challenges." This was flagged in R5-021. If no relevant citation is available, "could be explored" would be more honest than the current confident framing.

---

## Dimension 1: Narrative Balance Check

**Verdict: PASS.** The paper consistently opens with the geometry approximation problem. Three methods receive proportional treatment. Capsule dominance in Section IV is justified as primary technical contribution. No changes needed.

---

## Dimension 2: Qualitative Visualization Check

**Existing figures:**
- **Figure 1** (`figures/capsule_method_schematic.pdf`): 4-panel method schematic (PCA axis, cross-sections, circles, capsules). File exists at correct path. Verified.
- **Figure 2** (`figures/fig_qualitative_overlay.pdf`): 2x2 overlay grid (mesh, convex, sphere tree, capsule) on FR3 link3. File exists at correct path. Caption includes the Trimesh caveat requested in R5-001. **VERIFIED AS FIXED.**

**Figure 1 artifact gap:**
The `fig-capsule-method-schematic` artifact directory contains the PDF and PNG but has no `RESULT.md`. The artifact protocol requires a RESULT.md explaining the generation method, inputs, and limitations. This gap does not affect the paper's readability but breaks the artifact traceability chain. Consider adding a brief RESULT.md.

**Verdict: PASS.** Both figures exist at correct paths. Caption caveats are appropriate.

---

## Dimension 3: Downstream Applications Bridge

**Current coverage:**
- Section VII-A (Discussion): mentions analytic distance evaluation for motion planning, real-time simulation, differentiable collision pipelines. Lacks a concrete downstream library name.
- Section VII-B: discusses URDF compatibility constraint and JSON sidecar value. Strong.
- Section VIII (Conclusion): mentions FCL by name in future work. Good, but it's in the future-work section, not as an immediate downstream user.

**R6-015 [轻微] [writer] -- One concrete downstream library name in Discussion VII-A would strengthen the bridge.** Currently line 21 says "motion planning, real-time simulation, and differentiable collision pipelines" without naming any specific framework. Adding "such as FCL~\cite{pan2012fcl} or OMPL" would anchor the discussion in familiar robotics tooling.

**Verdict: ADEQUATE.** The bridge exists and is not overstated. A concrete library name would strengthen it.

---

## Dimension 4: Metric Consistency

| Metric | Convex | Sphere | Capsule |
|---|---|---|---|
| Primitive count | 8 | 8 / 63 | 8 / 10 / 10 |
| Coverage | yes (guaranteed) | no (both presets) | no (all presets) |
| capV/aabb | -- (N/A) | -- (N/A) | 1.32--1.56 |
| r/binMed | -- (N/A) | -- (N/A) | 1.27--1.50 |
| Runtime | 0.5 s | 10.1 / 40.7 s | 12.7--33.8 s |

**Verdict: GOOD.** All metrics reported consistently. N/A entries are correctly footnoted. No metric inconsistency issues.

---

## Dimension 5: Code Reference & Documentation Language Audit

### Current \texttt count

| File | `\texttt{}` instances | Types |
|---|---|---|
| `system_overview.tex` | 16 | XML element names (10), JSON field names (5), library name (1) |
| `method.tex` | 3 | URDF XML element names |
| `related_work.tex` | 1 | Library name |
| **Total** | **20** | |

**Improvement from iteration 5:** 32 instances reduced to 20. CLI command names (11 instances) and Python function signatures (4 instances) have been cleanly removed. The xacro shell command and hex constant are also gone. **These are the most impactful fixes in this iteration.**

**However, the count of 20 exceeds the reviewer.md threshold of 10.** All remaining instances are format terminology (XML element names, JSON field names, library names). State.md claims the count is 11, which is inaccurate by approximately 9 instances.

### Current \texttt usage by type

**Type A -- URDF XML element names (10 instances):**
- `system_overview.tex:14`: `\texttt{<mesh>}`
- `system_overview.tex:18`: `\texttt{<collision>}`
- `system_overview.tex:19`: `\texttt{<visual>}`, `\texttt{<inertial>}`, `\texttt{<joint>}`
- `system_overview.tex:37`: `\texttt{<sphere>}`
- `system_overview.tex:42`: `\texttt{<cylinder>}`, `\texttt{<sphere>}`
- `method.tex:162`: `\texttt{<cylinder>}`
- `method.tex:163`: `\texttt{<sphere>}`
- `method.tex:165`: `\texttt{<sphere>}`

**Type B -- JSON field names (5 instances):**
- `system_overview.tex:37`: `\texttt{spheres}`, `\texttt{center}`, `\texttt{radius}`
- `system_overview.tex:43`: `\texttt{capsules}`, `\texttt{p0}`, `\texttt{p1}`, `\texttt{radius}`

**Type C -- Library name (1 instance):**
- `system_overview.tex:36`: `\texttt{sphere\_tree}`

**Total by file:**
- `system_overview.tex`: line 14 (1), 18 (1), 19 (3), 36 (1), 37 (4), 42 (2), 43 (4) = 16
- `method.tex`: line 162 (1), 163 (1), 165 (1) = 3
- `related_work.tex`: line 25 (1) = 1
- Grand total: 20

**Assessment per reviewer.md criteria:**
- "全文 `\texttt{}` 超过 10 处 -> 严重": 20 > 10. Still technically SEVERE.
- However, all remaining instances are format terminology, which the reviewer guidelines acknowledge as acceptable under a relaxed standard.
- The most egregious offenders (CLI command names, function signatures, shell commands) are gone.

**Overall code-reference verdict: ACCEPTABLE with caveat.** The 20 remaining instances are all format terminology. The paper no longer reads as a README. The author should decide whether to invest further in reducing these 20 instances (by using plain prose for XML elements and JSON fields) or accept the relaxed standard.

**State.md discrepancy:** The coordinator claims `\texttt{} count now 11 (all format terminology)`. The actual count is 20. This should be corrected in the next state.md update.

---

## Artifact Traceability Audit

| Claim in paper | Artifact source | Status |
|---|---|---|
| Table 1 (mode comparison): all numeric values | `exp-fr3-mode-comparison/RESULT.md` aggregate table | PASS |
| Table 2 (ablation): all 11 rows | `exp-capsule-ablation/RESULT.md` | PASS |
| "NSections is the dominant control parameter" (experiments.tex L157) | Ablation data, rows nsections_2 through nsections_8 | PASS |
| "AdaptiveCircleCount increases capsule count 3.9x" (L166) | Ablation data (39/10 = 3.9) | PASS |
| "r/binMed spikes to 2.05" softened to "on one link" (L167) | Aggregate artifact shows 2.0462 worst-case | PASS (R6-009 resolved) |
| "high_detail preset achieves best worst-case uncovered distance (2.3 mm)" (L108) | Ablation nsections_6 row (2.338 mm) | PASS |
| "Convex mode is fastest (0.5 s)" (L183) | Mode comparison data (0.468 s) | PASS |
| "Sphere default leaves all links with small uncovered regions (1.8e-2 m worst-case)" (L104) | Mode comparison: 0.017608 m | PASS |
| "Capsule modes output 8-10 primitives" (L107) | Mode comparison: 8, 10, 10 | PASS |
| Figure 1 (method schematic) | `figures/capsule_method_schematic.pdf` | PASS (no RESULT.md, but file exists) |
| Figure 2 (qualitative overlay) | `figures/fig_qualitative_overlay.pdf` | PASS P |

**R6-016 [轻微] [implement] -- Figure 1 artifact (fig-capsule-method-schematic) has no RESULT.md.** The PDF and PNG files exist but the artifact directory lacks a RESULT.md explaining the generation method, inputs (which link, which tool), and limitations. This breaks the artifact protocol. Consider adding a minimal RESULT.md.

---

## Citation Calibration Status

### Current ref.bib state

All 27 entries in `ref.bib` are real and verified. The following corrections from the `citation-survey` artifact (iter 5) are confirmed applied:

| Key | Status | Notes |
|---|---|---|
| `urdf_doc` | VERIFIED | URL correct |
| `xacro` | VERIFIED | URL correct |
| `todorov2012mujoco` | VERIFIED | DOI present |
| `drake` | VERIFIED | URL correct |
| `coumans2018pybullet` | VERIFIED | Software ref |
| `pan2012fcl` | VERIFIED | DOI present |
| `lien2008acd` | VERIFIED | CAGD journal version, year 2008, DOI present |
| `mamou2016vhacd` | VERIFIED | Conference talk, software reference |
| `barber1996quickhull` | VERIFIED | ACM TOMS, DOI present |
| `cgal` | VERIFIED | URL correct |
| `jacobson2018libigl` | VERIFIED | Corrected to @misc |
| `huang2022manifoldplus` | PARTIAL | year=2020 correct, but key says 2022 |
| `bradshaw2002sphere` | VERIFIED | Trinity College Dublin |
| `mlund_spheretree` | VERIFIED | URL correct |
| `wu2018capsule` | VERIFIED | Nannan Wu, IEEE Access 2018, DOI present |
| `larsen2000ssv` | VERIFIED | ICRA 2000, DOI present |
| `larsen1999ssv` | VERIFIED | Tech report, preserved |
| `koptev2023neural` | VERIFIED | RA-L 2023, DOI present, IROS presentation noted |
| `eigen` | VERIFIED | Software ref |
| `urdfdom` | VERIFIED | URL correct |
| `welzl1991mec` | VERIFIED | LNCS, DOI present |
| `robot_viewer` | VERIFIED | URL correct |
| `franka2020fr3` | PARTIAL | year=2023 correct, but key says 2020 |
| `trimesh` | VERIFIED | URL correct |
| `sdformat` | VERIFIED | URL correct |
| `coumar2025foam` | VERIFIED | arXiv 2503.13704, correct |
| `lauterbach2010gproximity` | VERIFIED | Eurographics 2010, DOI present |

### Remaining citation issues

**R6-017 [轻微] [writer] -- Citation key `huang2022manifoldplus` contains an incorrect year (2022 vs actual 2020).** The paper is from 2020 (arXiv:2005.11621, May 2020). The key `huang2022manifoldplus` suggests 2022. While BibTeX does not enforce consistency between key and `year` field, this mismatch could confuse readers. Recommend renaming to `huang2020manifoldplus`.

**R6-018 [轻微] [writer] -- Citation key `franka2020fr3` contains an incorrect year (2020 vs actual 2023).** The FR3 robot launched ~2022-2023; the entry correctly has year=2023. The key says 2020. Recommend renaming to `franka2023fr3`.

### Citation keys used in related_work.tex

All cite usages in related_work.tex match valid, verified bib entries. No missing keys. No false citations. The `mukadam2018trapezoidal` citation has been correctly removed from both .bib and .tex.

**Verdict:** Citations are in good shape. Two minor key-year mismatches to clean up.

---

## Additional Observations

**R6-019 [轻微] [writer] -- State.md claims `\texttt{} count now 11`, but actual count is 20.** The state.md entry (line 8) states: "texttt{} count now 11 (all format terminology -- XML/JSON/library names)." The six remaining figures from system_overview.tex lines 14, 18, 19, 36, 37, 42, 43 were apparently not counted. This discrepancy should be corrected in the next state.md update to avoid misleading future loop iterations. The actual count of 20 is still a significant improvement from 32.

**R6-020 [轻微] [implement] -- The `fig-capsule-method-schematic` artifact lacks a RESULT.md.** The figure files (PDF and PNG) exist in the correct `figures/` directory and the AD includes a copy, but the artifact directory `doc/paper/artifacts/fig-capsule-method-schematic/` contains no RESULT.md. Consider adding one to match the artifact documentation standard.

---

## Loop Triage

| issue_id | severity | owner | blocking | required_action |
|---|---|---|---|---|
| R6-016 | 轻微 | implement | no | Add RESULT.md for fig-capsule-method-schematic artifact documenting generation method and inputs. |
| R6-009 | 中等 | writer | no | (Already resolved in this iteration) The "link2" claim has been correctly softened to "on one link." |
| R6-017 | 轻微 | writer | no | Rename bib key `huang2022manifoldplus` to `huang2020manifoldplus` for year consistency. |
| R6-018 | 轻微 | writer | no | Rename bib key `franka2020fr3` to `franka2023fr3` for year consistency. |
| R6-019 | 轻微 | loop | no | Correct state.md \texttt count from 11 to 20 to avoid misleading future iterations. |
| R6-007 | 轻微 | writer | no | Add 1-2 sentences in method Section IV-F foreshadowing that splitting can create temporary coverage gaps. |
| R6-015 | 轻微 | writer | no | Add concrete downstream library name ("FCL", "OMPL") to Discussion Section VII-A line 21. |
| R6-014 | 轻微 | writer | no | Temper "learning-based fitting" future work claim or add a supporting citation. |
| R6-012 | 轻微 | writer | no | Condense Discussion Section VII-E workflow description to a single sentence. |

### Implementation Request (carried forward from R3-021/R4-013/R5-016, now resolved)

The "link2" per-link data gap has been resolved by softening the claim to "on one link." No further implement action needed.

### Implementation Request (new)

```
request_id:      fig-capsule-method-schematic-report
paper_claim:     Figure 1 illustrates the capsule fitting pipeline on FR3 link3
task_type:       documentation
required_outputs: RESULT.md in doc/paper/artifacts/fig-capsule-method-schematic/
metrics:         N/A -- documentation only
inputs:          Existing PDF/PNG files in the artifact directory
acceptance_criteria: RESULT.md describing the figure generation method, input link identity, and any limitations
writer_integration_target: N/A -- artifact documentation only
```

---

## Summary of Priority Actions

1. **[HIGH] Correct state.md \texttt count** -- The count is 20, not 11. This discrepancy should be fixed for accurate planning.
2. **[MEDIUM] Add RESULT.md for fig-capsule-method-schematic** -- Artifact lacks required documentation.
3. **[MEDIUM] Add concrete downstream library name to Discussion VII-A** -- Would strengthen the applications bridge for robotics readers.
4. **[LOW] Fix bib key year mismatches** -- `huang2022` -> `huang2020`, `franka2020` -> `franka2023`.
5. **[LOW] Temper learning-based future work claim** -- Either add citation or weaken to "could be explored."
6. **[LOW] Foreshadow N=4 coverage anomaly in method section** -- 1-2 sentences in Section IV-F would prepare readers.

### Overall Verdict

The paper has reached a solid state. All review_iter_5 blocking issues (R5-004, R5-009) have been cleanly resolved: CLI commands, Python signatures, shell command, hex constant removed. The `\texttt` count has dropped from 32 to 20, all format terminology. The "link2" claim is correctly softened. Citations are in good order. The experiments section is honest and well-supported by artifacts. No blocking issues remain. This paper is ready for the remaining 5 cold-read reviews as specified in state.md.
