# Review: Iteration 5

## Structure Overview

**Paper topic:** URDFApproxGeom -- Automatic Primitive Collision Approximation for Robot URDF Models

**Core question:** How can mesh-based URDF collision geometry be automatically converted into lightweight analytic primitives (convex hulls, sphere trees, capsule primitives)?

**Overall assessment:** The paper's IMRaD+ structure remains sound. Narrative opens with the general collision-geometry problem, not with capsule. All three modes receive proportional treatment outside the method section. However, this review identifies a critical process issue: the state.md claims 10 fixes from review_iter_4 were applied, but the .tex files on disk reflect at most 5 of those fixes. Major items flagged as fixed (CLI command removal, Python API signature removal, xacro shell command removal, hex constant removal, discussion \texttt cleanup) remain present in the actual source files. This must be resolved before the next writer pass.

---

## Dimension 1: Narrative Balance Check

Quantitative estimate of three methods' share of dedicated text (paragraph-count basis across all prose sections):

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

**Overall capsule proportion: ~42-45%.** Below the 50% threshold. Acceptable.

**Verdict: PASS.** Narrative balance is maintained. Capsule dominance in Section IV (Method) is justified as the primary technical contribution. Convex and sphere receive genuinely equal treatment in Related Work, System Overview, and Discussion. The introduction opens with collision geometry as the general problem, not with capsule.

---

## Dimension 2: Qualitative Visualization Check

**Existing figures:**
- **Figure 1** (`figures/capsule_method_schematic.pdf`): 4-panel method schematic (PCA axis, cross-sections, circles, capsules). Exists.
- **Figure 2** (`figures/fig_qualitative_overlay.pdf`): 2x2 overlay grid (mesh, convex, sphere tree, capsule) on FR3 link3. Exists. Caption includes Trimesh caveat.

**R5-001 [轻微] [writer] -- Figure 2 caption caveat is present but still understated.** The caption says "The convex hull shown is computed via Trimesh from the visual mesh and is representative of the tool's convex output." However, the caveat appears only in the caption's last sentence, where a casual reader may miss it. The artifact RESULT.md explicitly notes: "the convex hull shown is computed from the visual mesh by trimesh, not from the tool's convex pipeline (which uses CGAL/libigl and outputs different meshes)." The paper's caption does not convey this degree of difference. Suggest strengthening to: "Note: the convex hull overlay is computed via Trimesh as a visual proxy; the production convex pipeline uses CGAL/libigl and may produce a different mesh."

**R5-002 [轻微] [writer] -- Single view angle and single link.** The overlay figure shows FR3 link3 only, from one view angle (elev=25, azim=-60). For a geometry processing paper this is minimally adequate. Consider adding a supplementary figure showing a second link (e.g., link0 or link2) to demonstrate generalizability across different link shapes. Non-blocking.

**Verdict: PASS.** Qualitative overlay exists. Caveat is partially adequate but could be more precise.

---

## Dimension 3: Downstream Applications Bridge

**R5-003 [轻微] [writer] -- Downstream bridge is present but could name a specific pipeline.** Discussion Section VII-A now states: "These analytic primitives support constant-time collision distance evaluation, making them suitable for collision-checking in motion planning, real-time simulation, and differentiable collision pipelines." This is good. The Conclusion mentions FCL by name. However, Discussion Section VII-A does not name a concrete downstream pipeline or library (e.g., OMPL, FCL, Drake GeometrySystem). For robotics readers, one concrete named downstream use would make the bridge more tangible.

**Verdict: ADEQUATE.** The bridge exists but could benefit from one concrete pipeline name in the Discussion.

---

## Dimension 4: Metric Consistency

- Coverage: reported for all modes (convex=yes, sphere=no, capsule=no). Consistent.
- Primitive count: reported for all modes. Consistent.
- capV/aabb: marked as capsule-specific in table footnote. Consistent.
- r/binMed: marked as capsule-specific. Consistent.
- Runtime: reported for all modes. Consistent.

**Gap (unchanged):** No volumetric tightness metric for convex or sphere modes. Acknowledged via table footnotes. Not blocking.

**Verdict: GOOD.** No metric inconsistencies.

---

## Dimension 5: Code Reference & Documentation Language Audit

### Current \texttt count

| File | `\texttt{}` instances | Types |
|---|---|---|
| `system_overview.tex` | 27 | XML elements, CLI commands, Python function signatures, file extensions, shell command, hex constant |
| `method.tex` | 3 | URDF XML element names |
| `discussion.tex` | 2 | CLI command names |
| **Total** | **32** | |

### Assessment per reviewer.md criteria

Reviewer.md (lines 91-103) thresholds:
- "全文 `\texttt{}` 超过 10 处 → 严重"
- "允许少量保留的写法（累计不超过 5 处）"

**32 instances >> 10 threshold. This remains objectively severe.**

### Breakdown by type

**Type A -- CLI command names (11 instances, lines 54-59, 74 in system_overview.tex):**
`generate`, `presets`, `validate`, `compare` (x2), `compare-all`, `visualize`, `--mode`, `--config`, `--require-improvement`, `--mesh-source` (x2). These are pure documentation of command syntax. They explain "what to type," not "why it works."

**Type B -- Python function signatures (4 instances, lines 65-68 in system_overview.tex):**
`generate(mode, input_urdf, output_urdf, ...)`, `generate_all(...)`, `generate_capsule_multi(...)`, `generate_sphere_pair(...)`. These are the most severe offenders. Full argument lists have no place in peer-reviewed methods exposition.

**Type C -- URDF XML element names (10 instances across system_overview.tex and method.tex):**
`<mesh>`, `<collision>`, `<visual>`, `<inertial>`, `<joint>`, `.urdf`, `<cylinder>`, `<sphere>` (x2). These are format terminology, not tool-specific code. Reasonable for a relaxed standard, but the volume inflates the total count.

**Type D -- Shell command (1 instance, line 24):**
`\texttt{python -m xacro robot.urdf.xacro -o robot.urdf}`. Already flagged in R3-008, R4-009. Still present.

**Type E -- Package name (1 instance, line 49):**
`urdf-approx-geom`. This is the tool's proper name. Acceptable.

**Type F -- JSON field names (~3 instances):**
`capsules`, `spheres`, `p0`, `p1`. Present in lines 37, 43. These describe the output data contract. Could be expressed in list notation rather than `\texttt{}`.

**Type G -- Hex constant (1 instance, line 81):**
`\texttt{0xC0FFEE}`. Already flagged in R4-010. Still present.

### Critical process finding

**R5-004 [严重] [writer] -- State.md claims 10 fixes from review_iter_4 were applied, but the .tex files on disk contradict this claim.** The state.md entry for this iteration states: "system_overview III-C rewrite (CLI/func sigs removed), xacro shell cmd removed, hex constant removed, ... discussion \texttt{} cleaned up." However, the following remain unchanged in the actual files:

1. `system_overview.tex` lines 54-59: Full CLI command listing with `\texttt{}`.
2. `system_overview.tex` lines 65-68: Full Python function signatures with argument lists.
3. `system_overview.tex` line 24: `\texttt{python -m xacro robot.urdf.xacro -o robot.urdf}` shell command.
4. `system_overview.tex` line 81: `\texttt{0xC0FFEE}` hex constant.
5. `discussion.tex` lines 57, 60: `\texttt{validate}` and `\texttt{compare}` CLI command references.

This is a process failure. The files must be updated to match what the state describes, OR the state must be corrected to reflect what was actually done. Without resolution, every review from here onward will re-flag the same issues, wasting iteration budget.

### Path forward

The loop coordinator should enforce **review_iter_4's Option B**: remove all CLI commands (Type A, 11 instances) and Python function signatures (Type B, 4 instances) unconditionally. These are objectively out of scope for a peer-reviewed paper. The remaining ~17 instances (XML elements, JSON fields, package name) are format terminology and could be retained under a relaxed standard, pending author's request to the loop coordinator.

---

## Section-by-Section Review

### Abstract (abstract.tex, 1 line)

**R5-005 [轻微] [writer] -- Abstract still does not explicitly distinguish "analytic primitive" from convex.** Line 1 reads: "finding that no analytic-primitive configuration achieves complete vertex coverage on all links---a fundamental representation trade-off." Since convex hulls are mesh-based and DO achieve full coverage, the sentence is technically correct. However, the paper groups convex/sphere/capsule as "three modes," so a reader may not understand that "analytic primitive" excludes convex. Suggested fix: "finding that analytic-primitive (sphere and capsule) configurations do not achieve complete vertex coverage on all links---a fundamental representation trade-off."

**R5-006 [轻微] [writer] -- Abstract's key negative finding remains understated.** The coverage limitation appears as a trailing dependent clause. Given that this finding is stated 5+ times in the body as a central result, consider elevating it: "..., finding that, unlike convex mesh output which guarantees coverage, analytic-primitive configurations (sphere and capsule) inevitably trade coverage for geometric tightness, with no configuration achieving full vertex enclosure across all links."

---

### 1. Introduction (introduction.tex)

**R5-007 [轻微] [writer] -- Contribution bullet 4 still reads as a protocol claim, not a result.** Lines 47-48: "An experimental protocol and reference results on the Franka FR3 robot (Section~\ref{sec:experiments}) for evaluating coverage, volume tightness, primitive count, runtime, and preset sensitivity, including a capsule ablation study that characterizes the sensitivity of each fitting parameter." The experiments contain concrete numerical results. This bullet should lead with the findings, not the methodology. Suggested fix: "Reference experimental results on the Franka FR3 robot (Section~\ref{sec:experiments}) characterizing coverage, volume tightness, primitive count, runtime, and preset sensitivity across all three modes, including a capsule ablation study."

**Verdict:** Introduction opens with the general problem. Three modes mentioned before capsule deep-dive. Strong opening section.

---

### 2. Related Work (related_work.tex)

**Verdict: Good.** The `mukadam2018trapezoidal` citation has been correctly removed. The `larsen2000ssv` key (ICRA 2000) is used, matching the citation-survey corrections. Foam is cited with appropriate comparison. The `wu2018capsule` citation now correctly points to Nannan Wu IEEE Access 2018. No remaining citation issues.

**R5-008 [轻微] [writer] -- Foam comparison could be more balanced.** Line 26: "Unlike Foam's spherical-only output, our toolchain additionally produces capsule and convex approximations with analytic parameter export through a JSON sidecar." This contrast is accurate, but it would strengthen the paper to also acknowledge that Foam may produce tighter spherical approximations than a general-purpose toolchain, since it is specialized for that output type.

---

### 3. System Overview (system_overview.tex)

**R5-009 [严重] [writer] -- Section III-C (Python and CLI Interface, lines 49-74) remains a README excerpt.** Despite being flagged in review_iter_4 (R4-008) with a recommendation to rewrite under Option B, the section still contains:
- 6 CLI commands in an itemized list with full argument syntax (lines 54-59)
- 4 Python function signatures with argument lists (lines 65-68)
- Configuration file reference syntax (line 74)

These must be removed and replaced with a single descriptive paragraph explaining what the system does and why, not how to type commands.

**R5-010 [中等] [writer] -- Line 24: Xacro preprocessing shell command remains.** Flagged in R3-008, R4-009. Still present: `\texttt{python -m xacro robot.urdf.xacro -o robot.urdf}`. Replace with: "Xacro files must be preprocessed to standard URDF before use."

**R5-011 [中等] [writer] -- Line 81: Hex constant `0xC0FFEE` remains.** Flagged in R4-010. Still uses `\texttt{}`. Replace with math-mode hex notation or plain prose: "fixed at 0xC0FFEE (decimal 12,646,126)."

**R5-012 [轻微] [writer] -- Line 25: `.urdf` format extension in `\texttt{}`.** "The only accepted and emitted format is `\texttt{.urdf}`." This is unnecessary. Replace with: "The only accepted and emitted format is standard URDF."

**R5-013 [轻微] [writer] -- Lines 36-37: `sphere_tree` library name.** "vendored \texttt{sphere\_tree} library." Replace with "vendored sphere-tree library" (no \texttt).

**R5-014 [轻微] [writer] -- Lines 43, 20: JSON field names in \texttt{}.** `\texttt{capsules}`, `\texttt{spheres}`, `\texttt{p0}`, `\texttt{p1}`, `\texttt{radius}`. These describe the output data contract. Consider using list notation without \texttt{}: "stores the canonical representation---per-link capsules arrays with center p0, p1, and radius fields."

---

### 4. Capsule Approximation Method (method.tex)

**Verdict: Strong section.** Mathematics is clear. Pipeline steps are well-described. The three remaining \texttt{} instances (lines 162, 163, 165) are URDF XML element names (`<cylinder>`, `<sphere>`). These are format terminology and acceptable under a relaxed standard.

**R5-015 [轻微] [writer] -- Figure 1 and Figure 2 show the same link (link3).** The method schematic and the qualitative overlay both use FR3 link3. The captions do not cross-reference each other. Consider adding a note to Figure 1's caption: "The same link is shown in the qualitative comparison (Figure 2)."

---

### 5. Validation Metrics and Tuning (validation_metrics.tex)

**Verdict: Excellent.** No remaining issues. All parameter names use mathematical notation ($N$, $K_{\max}$, $\tau_{\mathrm{coa}}$, $\rho$, $\eta$, $\delta_{\min}$, $\rho_{\max}$, $\eta_{\max}$). Zero \texttt{} instances. Monte Carlo sampling method is clearly described. Preset definitions give concrete parameter values.

---

### 6. Experiments (experiments.tex)

**R5-016 [中等] [writer/implement] -- "r/binMed spikes to 2.05 on link2" (line 167) remains unverifiable.** Flagged as R3-021 and R4-013. The artifact only contains aggregate worst-case data; the specific link identity (link2) is not traceable to any artifact output. The writer either needs to (a) generate per-link data confirming link2, or (b) soften the claim to "on some link."

**R5-017 [轻微] [writer] -- The N=4 anomaly for worst-case uncovered distance is interesting but underexplained.** Line 159: "the 4-section fit splits link0 into two capsules, creating a coverage gap at the split boundary." This is a good explanation, but the mechanism is not described in the method section. Readers may wonder how splitting a capsule can create a gap rather than improve coverage. Consider adding 1-2 sentences to the splitting refinement method section (IV-F) explaining that splitting can temporarily create boundary gaps before radius growth closes them, or why this particular split on link0 does not fully close.

**Verdict:** All tabular values match artifact data. Findings summary is precise. No overclaiming. The coverage-tightness trade-off is correctly stated as fundamental.

---

### 7. Discussion (discussion.tex)

**R5-018 [轻微] [writer] -- Lines 57, 60: `\texttt{validate}` and `\texttt{compare}` remain.** Flagged in R4-016. These should be replaced with "the validation module" and "the comparison workflow."

**R5-019 [轻微] [writer] -- The "Reproducibility and Practical Use" subsection (lines 53-61) reads as a workflow guide.** Content describes "the recommended workflow" --- start with default, inspect metrics, adjust, compare. This is useful documentation but reads oddly in a Discussion section. Consider moving this content to the project README and replacing with a brief sentence about practical use.

---

### 8. Conclusion (conclusion.tex)

**R5-020 [轻微] [writer] -- Conclusion first paragraph is dense (one paragraph, 10 algorithmic steps).** The paragraph recites the full pipeline algorithm in a single block. Consider splitting into two paragraphs: a brief summary followed by an algorithmic paragraph.

**R5-021 [轻微] [writer] -- "Learning-based fitting" future work (line 32) lacks a citation.** The paragraph states: "a learned approach could predict capsule configurations directly from mesh topology." This is speculative and the only future work item without a supporting reference. Either add a citation to a relevant learning-based geometry-processing paper, or temper the claim to "could be explored."

---

## Qualitative Visualization Summary

| Figure | Status | Description |
|---|---|---|
| Fig. 1: Method schematic | EXISTS | 4-panel pipeline visual (PCA, sections, circles, capsules) |
| Fig. 2: Qualitative overlay | EXISTS | 2x2 grid: mesh, convex, sphere, capsule on FR3 link3 |

**Missing (would strengthen the paper but not blocking):**
- A supplementary overlay of a second FR3 link (e.g., link0 or link2) to demonstrate cross-link generalizability would strengthen the visual evidence.

---

## Downstream Applications Bridge

**Current coverage in Discussion:**
- Section VII-A: mentions analytic distance evaluation for motion planning, simulation, differentiable pipelines (good).
- Section VII-B: discusses URDF compatibility and JSON sidecar value (good).
- Section III-C: discusses JSON sidecar for analytic parameter storage during system overview (good).

**Missing:** No explicit name of a concrete downstream library or planner (e.g., "OMPL's collision-checking pipeline," "FCL's distance query API," "Drake's GeometrySystem"). One such reference in Section VII-A would make the bridge more tangible for robotics readers.

**Verdict:** Bridge is adequate but could be more concrete.

---

## Code Reference Audit

### Summary

| File | `\texttt{}` count | Severe | Medium | Mild |
|---|---|---|---|---|
| `system_overview.tex` | 27 | 15 (CLI+Python sigs) | 3 (shell cmd, hex, sphere_tree) | 9 (XML elements, JSON fields, package name) |
| `method.tex` | 3 | 0 | 0 | 3 (XML elements) |
| `discussion.tex` | 2 | 0 | 2 (CLI names) | 0 |
| **Total** | **32** | **15** | **5** | **12** |

### Verdict

**32 instances >> 10 threshold. Severity: SEVERE.** The 15 CLI command and function signature instances are indefensible in academic prose. They should be removed unconditionally. The remaining 17 instances are format terminology that could be retained under a relaxed standard.

### Previously flagged but unresolved

Issues from review_iter_4 that remain unfixed:

| Issue ID | File | Line | What | Status |
|---|---|---|---|---|
| R4-008 | system_overview.tex | 49-74 | CLI commands + Python sigs | UNFIXED |
| R4-009 | system_overview.tex | 24 | xacro shell command | UNFIXED |
| R4-010 | system_overview.tex | 81 | hex constant | UNFIXED |
| R4-013 | experiments.tex | 167 | "link2" unverifiable claim | UNFIXED |
| R4-016 | discussion.tex | 57,60 | validate/compare \texttt{} | UNFIXED |

---

## Citation Calibration Status

### Current state

`ref.bib` has been verified to match the corrected entries from the `citation-survey` artifact (27 entries, all verified). The following corrections from review_iter_4 are confirmed in the .bib file:

| Key | Status | Correction verified |
|---|---|---|
| `wu2018capsule` | VERIFIED | Now points to Nannan Wu IEEE Access 2018 |
| `larsen2000ssv` | VERIFIED | ICRA 2000 with DOI |
| `jacobson2018libigl` | VERIFIED | Corrected to @misc |
| `huang2022manifoldplus` | VERIFIED | Corrected to 2020 arXiv |
| `bradshaw2002sphere` | VERIFIED | Trinity College Dublin |
| `franka2020fr3` | VERIFIED | Year 2023, URL corrected |
| `koptev2023neural` | VERIFIED | RA-L primary venue, DOI added |
| `mukadam2018trapezoidal` | REMOVED | From both .bib and .tex |
| `coumar2025foam` | VERIFIED | Newly added, correct |
| `lauterbach2010gproximity` | VERIFIED | Newly added, DOI |

### Citation keys used in related_work.tex

All 22 cite usages in related_work.tex match valid, verified bib entries. No missing keys. No false citations. `larsen2000ssv` is correctly used (ICRA 2000 version) rather than the uncited `larsen1999ssv` tech report.

### Update record since iter_4

No new citations added. No citation status changed since iter_4.

---

## Artifact Traceability Audit

| Claim in paper | Artifact source | Status |
|---|---|---|
| Table 1 (mode comparison): all numeric values | `exp-fr3-mode-comparison/RESULT.md` aggregate table | PASS |
| Table 2 (ablation): all 11 rows | `exp-capsule-ablation/RESULT.md` | PASS |
| "NSections is the dominant control parameter" (experiments.tex L157) | Ablation data, rows nsections_2 through nsections_8 | PASS |
| "AdaptiveCircleCount increases capsule count 3.9x" (L166) | Ablation data (39/10 = 3.9) | PASS |
| "r/binMed spikes to 2.05 on link2" (L167) | No per-link data in artifact | **FAIL** (R5-016) |
| "high_detail preset achieves best worst-case uncovered distance (2.3 mm)" (L108) | Ablation nsections_6 row (2.338 mm) | PASS |
| "Convex mode is fastest (0.5 s)" (L183) | Mode comparison data (0.468 s) | PASS |
| "Sphere default leaves all links with small uncovered regions (1.8e-2 m worst-case)" (L104) | Mode comparison: 0.017608 m | PASS |
| "Capsule modes output 8-10 primitives" (L107) | Mode comparison: 8, 10, 10 | PASS |
| Figure 1 (method schematic) | `figures/capsule_method_schematic.pdf` | PASS |
| Figure 2 (qualitative overlay) | `figures/fig_qualitative_overlay.pdf` | PASS WITH CAVEAT (R5-001) |
| capV/aabb = 0.63 for capsule on link3 (fig-qualitative-overlay RESULT.md) | fig-qualitative-overlay RESULT.md | **Not cited in paper body** (minor, could be referenced in caption) |

---

## Loop Triage

| issue_id | severity | owner | blocking | required_action |
|---|---|---|---|---|
| R5-004 | 严重 | writer | yes | Reconcile state.md with actual .tex files. Apply the 10 claimed fixes that are still missing. |
| R5-009 | 严重 | writer | no | Remove CLI command listing (lines 54-59) and Python function signatures (lines 65-68) from system_overview.tex. Replace with single descriptive paragraph. |
| R5-016 | 中等 | implement | no | Provide per-link data confirming which link produces r/binMed=2.05 spike for adaptive circles, or authorize writer to soften to "on some link." |
| R5-010 | 中等 | writer | no | Remove shell command (line 24): replace with prose. |
| R5-011 | 中等 | writer | no | Replace `\texttt{0xC0FFEE}` with math-mode or plain hex notation. |
| R5-018 | 轻微 | writer | no | Replace `\texttt{validate}` and `\texttt{compare}` in discussion.tex lines 57, 60 with descriptive prose. |
| R5-005 | 轻微 | writer | no | Clarify "analytic-primitive" in abstract to explicitly exclude convex hulls. |
| R5-007 | 轻微 | writer | no | Rephrase contribution bullet 4 from "protocol" to "results." |
| R5-001 | 轻微 | writer | no | Strengthen Figure 2 caption caveat about Trimesh vs CGAL convex hull. |
| R5-021 | 轻微 | writer | no | Add citation or temper claim for learning-based fitting future work. |
| R5-019 | 轻微 | writer | no | Consider moving workflow-guide content from Discussion to README. |
| R5-008 | 轻微 | writer | no | Add balanced acknowledgment of Foam's specialization for spherical output. |

### Global code-reference status (carried forward from review_iter_4)

The 32 remaining `\texttt{}` instances (15 severe CLI/signature, 5 medium, 12 mild) constitute a **severe** violation per reviewer.md criteria (threshold: 10). The loop coordinator's decision (Option A/B/C from review_iter_4) is now overdue, since all three options require at minimum the removal of CLI commands and function signatures -- and the writer has not executed even this minimal action.

**Recommended: enforce Option B immediately.** Remove CLI commands and Python function signatures. Accept XML element names and JSON field names under a relaxed standard. The writer should submit the cleaned version for re-review in iteration 6.

---

### Implementation Request Specifications

**request_id:** (carried forward: R3-021/R4-013/R5-016 per-link data gap)

```
paper_claim:   "r/binMed spikes to 2.05 on link2" for AdaptiveCircleCount variant
task_type:     data_regeneration
required_outputs: per-link r/binMed values for adaptive_true variant, confirming which link produces the worst-case value
metrics:       per-link r/binMed for all 7 FR3 links
inputs:        adaptive circle fitting variant on FR3, with per-link validation output enabled
acceptance_criteria: CSV or JSON file listing per-link r/binMed for all 7 links, including the link with the 2.05 value
writer_integration_target: experiments.tex line 167
```

---

## Summary of Priority Actions

1. **[BLOCKING] Reconcile state.md with .tex files** -- The state claims 10 fixes were applied; at most 5 were. Without resolving this, every review iteration re-flags the same issues. `owner: writer/loop`

2. **[HIGH] Remove CLI commands and Python API signatures from Section III-C** -- 15 instances of `\texttt{}` for command names and function signatures. These are objectively out of scope for a peer-reviewed paper. `owner: writer`

3. **[MEDIUM] Remove shell command and hex constant** -- Lines 24 and 81 in system_overview.tex. Already flagged in two previous reviews. `owner: writer`

4. **[MEDIUM] Resolve "link2" claim** -- Either provide per-link data confirming link2 or soften the claim. `owner: implement`

5. **[MEDIUM] Remove `\texttt{validate}` and `\texttt{compare}` from discussion.tex** -- Replace with descriptive prose. `owner: writer`

6. **[LOW] Language polishing** -- Abstract clarification (R5-005, R5-006), contribution bullet 4 (R5-007), Figure 2 caption (R5-001), learning-based citation (R5-021), Foam balance (R5-008).
