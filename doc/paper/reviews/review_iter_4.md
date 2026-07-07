# Review: Iteration 4

## Structure Overview

**Paper topic:** URDFApproxGeom -- Automatic Primitive Collision Approximation for Robot URDF Models

**Core question:** How can mesh-based URDF collision geometry be automatically converted into lightweight analytic primitives (convex hulls, sphere trees, capsule primitives)?

**Overall structure assessment:** The paper follows a well-organized IMRaD+ structure. Narrative is problem-driven (geometry approximation need), not capsule-driven. Introduction opens with collision geometry as the general problem. This is the third cold re-read and the draft is in solid shape. Most issues from earlier reviews have been addressed. Below I focus on remaining gaps, new findings, and a fresh assessment of item 5 (code references) which reviewers 1-3 flagged as severe but where I find the writer's and reviewer's standards are in unresolved tension.

---

## Dimension 1: Narrative Balance Check

Quantitative estimate of three methods' share of total text (measured by dedicated paragraph count across all sections):

| Section | Convex | Sphere | Capsule | General/Other |
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

**Overall capsule proportion: ~42-45%.** This is below the 50% threshold. Capsule dominance in the Method section is justified as the paper's primary technical contribution. Convex and sphere receive genuinely equal treatment in Related Work, System Overview, and Discussion.

**Verdict: PASS.** Narrative balance is acceptable. The paper frames itself as a three-mode toolchain with capsule as the primary technical contribution, matching the outline's intent.

---

## Dimension 2: Qualitative Visualization Check

Figures in the paper:
- **Figure 1** (`figures/capsule_method_schematic.pdf`): 4-panel method schematic (PCA axis, cross-sections, circles, capsules). Exists at 300 DPI.
- **Figure 2** (`figures/fig_qualitative_overlay.pdf`): 2x2 overlay grid showing original mesh + convex hull + sphere tree + capsule on FR3 link3. Exists at 3.2 MB. This was the figure generated in response to R3-016.

**R4-001 [轻微] [writer] -- Figure 2 caption does not explain the convex hull origin.** The caption says "The convex hull shown is computed via Trimesh from the visual mesh and is representative of the tool's convex output." This caveat is good, but a first-time reader may not notice that the convex panel uses a Trimesh hull rather than the CGAL/libigl pipeline output. This is documented in the artifact RESULT.md but the paper's caption should state it more prominently. Suggest: "Note: the convex hull visual is computed via Trimesh and is representative; the tool's production pipeline uses CGAL/libigl, producing comparable output."

**R4-002 [轻微] [writer] -- Figure 2 covers only link3 with one view angle.** The overlay figure shows a single view angle (elev=25, azim=-60) on a single link. For a geometry processing paper, this is adequate but minimal. Consider adding a second overlay figure in supplementary material. Not blocking.

**Verdict: PASS.** The qualitative overlay gap from review_iter_3 is resolved. One overlay figure with appropriate caveats exists.

---

## Dimension 3: Downstream Applications Bridge

**R4-003 [轻微] [writer] -- Downstream bridge is present but could be more concrete.** The Discussion now has the sentence from R3-026: "These analytic primitives support constant-time collision distance evaluation, making them suitable for collision-checking in motion planning, real-time simulation, and differentiable collision pipelines." This is good.

However, there are no explicit mentions of concrete downstream pipelines by name (e.g., "OMPL for motion planning," "the Drake GeometrySystem," "FCL's distance query API"). The Conclusion mentions FCL but Discussion does not. Consider adding one specific downstream example in Discussion Section VII-A or VII-B to make the bridge more tangible for robotics readers.

**Verdict: ADEQUATE.** The bridge exists but could benefit from one concrete downstream pipeline reference.

---

## Dimension 4: Metric Consistency

- Coverage: reported for all modes (convex=yes, sphere=no, capsule=no). Consistent.
- Primitive count: reported for all modes. Consistent.
- capV/aabb: marked as capsule-specific in table footnote. Consistent.
- r/binMed: marked as capsule-specific. Consistent.
- Runtime: reported for all modes. Consistent.

**Gap (unchanged from previous reviews):** No volumetric tightness metric exists for convex or sphere modes. Convex hull volume / original mesh volume would be quantifiable but is not computed. Sphere tree has no volume-overlap metric. This is acknowledged in the table footnote and is a known limitation. Not blocking.

**Verdict: GOOD.** No metric inconsistencies found.

---

## Dimension 5: Code Reference & Documentation Language Audit

### Current State

The writer claims "~35 remaining \texttt{} instances are legitimate code/syntax references." The actual count is:

| File | `\texttt{}` instances | Type |
|---|---|---|
| `system_overview.tex` | 42 | CLI commands, Python function signatures, file extensions, XML elements, flag names, package names |
| `method.tex` | 3 | URDF XML element names |
| `discussion.tex` | 2 | CLI command names |
| `related_work.tex` | 1 | Library name (`sphere_tree`) |
| **Total** | **48** | |

### Assessment per reviewer.md criteria

The reviewer.md (lines 91-103) states:
- "允许少量保留的写法（累计不超过 5 处）" — at most 5 instances allowed.
- "全文 \texttt{} 超过 10 处 → 严重"

**48 instances far exceeds both thresholds.** This is objectively "严重" (severe) by the reviewer's own standard.

### Writer's counter-argument

The writer claims the remaining instances are "legitimate" because they refer to:
1. URDF format terminology (`<mesh>`, `<collision>`, `<sphere>`, `<cylinder>`, etc.) — these are XML element names, not tool-specific code.
2. The JSON sidecar field names (`p0`, `p1`, `radius`, `capsules`, `spheres`) — these are part of the tool's output contract.
3. The CLI command names (`generate`, `validate`, `compare`, etc.) — these are entry points to numbered pipeline stages.
4. The Python function signatures — the writer argues these document the public API.
5. The package name `urdf-approx-geom` — a proper name for the software.

### Reviewer's counter-assessment

**Type 1 (XML elements):** 10 instances in system_overview.tex. These are URDF format terminology. They are not tool-specific code. However, repeating them in `\texttt{}` every time rather than using descriptive prose is unnecessary. Example: "cylinder and sphere XML elements" is clearer than "`\texttt{<cylinder>}` and `\texttt{<sphere>}` elements." **Mitigating: MEDIUM.**

**Type 2 (JSON field names):** 5 instances (`capsules`, `spheres`, `center`, `radius`, `p0`, `p1`). These describe the output data format, which is part of the paper's technical contribution. A reader needs to know the sidecar schema. However, these should be expressed as mathematical symbols or list notation, not `\texttt{}`. **Mitigating: MEDIUM.**

**Type 3 (CLI commands):** 12+ instances (`generate`, `presets`, `validate`, `compare`, `compare-all`, `visualize`, `--mode`, `--config`, `--require-improvement`, `--mesh-source visual`, `--mesh-source collision`). These are pure documentation. The reader does not need to know the exact command names to understand the pipeline. **Severe: HIGH.** These should be replaced with descriptive prose ("the generation pipeline," "the validation module") as recommended in R3-007.

**Type 4 (Python function signatures):** 6+ instances (`generate(mode, input_urdf, output_urdf, ...)`, `GenerateResult`, `generate_all(...)`, `generate_capsule_multi(...)`, `generate_sphere_pair(...)`). These are the worst offenders. A function signature with argument names has zero place in a peer-reviewed methods paper. **Severe: CRITICAL.**

**Type 5 (package name):** 1 instance (`urdf-approx-geom`). The single occurrence of the tool's name in `\texttt{}` is acceptable. **PASS.**

**Type 6 (xacro shell command):** 1 instance (`\texttt{python -m xacro robot.urdf.xacro -o robot.urdf}`). This is a literal shell command. R3-008 flagged this as severe but it remains in the text. **Severe: HIGH.**

### Verdict

The 48 remaining `\texttt{}` instances, concentrated in `system_overview.tex` (42/48), constitute a **severe** violation of the code-reference review criteria (threshold: 10 instances). The writer's claim that these are "legitimate" does not align with the reviewer.md standard, which permits only 5 instances regardless of type.

**The writer must choose one of:**
1. **Remove all 48 instances** and replace with descriptive prose (per reviewer.md criteria), OR
2. **Formally petition the loop coordinator** to relax the \texttt{} constraint for this paper, arguing that URDF format terminology is inherently code-like and that the reader benefits from seeing precise CLI/Python signatures. The loop coordinator would need to override the reviewer.md standard.

**Recommendation:** Regardless of which path is chosen, the CLI command listing (Section III-C) and Python function signatures must be removed. These have no legitimate place in a peer-reviewed paper. The URDF XML elements and JSON field names could be retained in a relaxed standard, but CLI/API listings cannot.

---

## Section-by-Section Review

### Abstract (abstract.tex)

**R4-004 [轻微] [writer] -- The phrase "analytic-primitive configuration" is ambiguous.** Line 1: "finding that no analytic-primitive configuration achieves complete vertex coverage on all links." Since convex hulls are mesh-based (not analytic primitives) and DO achieve full coverage, the sentence is technically correct. But a reader may wonder whether "analytic-primitive" includes convex. This was flagged in R3-001 and only partially addressed. The writer's current text says "no analytic-primitive configuration" which is unambiguous IF the reader distinguishes analytic (sphere/capsule) from mesh-based (convex). But the paper itself calls convex a "mode" alongside sphere and capsule, creating confusion. Suggest: "finding that analytic-primitive (sphere and capsule) configurations do not achieve complete vertex coverage on all links."

**R4-005 [轻微] [writer] -- Abstract understates the key negative finding.** The abstract mentions the coverage limitation as a single dependent clause at the end. Given that this is the paper's most striking finding (stated 5+ times in the body), consider elevating it: "..., finding that, unlike convex mesh output which guarantees coverage, analytic-primitive configurations (sphere and capsule) inevitably trade coverage for geometric tightness, with no configuration achieving full vertex enclosure across all links."

---

### 1. Introduction (introduction.tex)

**R4-006 [轻微] [writer] -- Contribution bullet 4 remains a protocol claim, not a result.** Lines 47-48: "An experimental protocol and reference results on the Franka FR3 robot." The experiments exist and contain concrete numbers. The wording still frames this as a methodology contribution. Rephrase: "Experimental evaluation on the Franka FR3 robot characterizing coverage, volume tightness, primitive count, runtime, and preset sensitivity across all three modes and a capsule ablation study." This was first flagged in R3-004 and is still not fully addressed.

**Verdict:** Introduction is in good shape. The narrative opens with the general problem, not with capsule. All three modes are mentioned before the capsule deep-dive. The capsule-priming effect from review_iter_3 (R3-003) has been fixed via the "Spheres, capsules, and boxes" framing at line 20-24.

---

### 2. Related Work (related_work.tex)

**R4-007 [轻微] [writer] -- Foam comparison is well-placed but the Foam paper could use a sentence about its limitations.** Line 26: "More recently, Foam introduced an adaptive medial-axis approximation method specialized for spherical approximation of robot URDF geometries." This is accurate. The comparison sentence ("Unlike Foam's spherical-only output, our toolchain additionally produces capsule and convex approximations") is also accurate. However, the paragraph does not mention that Foam is spherical-only by design and may produce tighter spherical approximations than a general-purpose toolchain. A balanced comparison would acknowledge this.

**Verdict:** Related work is well-researched. All citations are correctly placed. The cross-section method (Wu2018) citation now correctly points to Nannan Wu IEEE Access 2018 (critical fix from iter_1 verified in iter_2/iter_3).

---

### 3. System Overview (system_overview.tex)

This section remains the paper's weakest part. Issues from previous reviews (R3-007, R3-008) persist. Rather than repeating those, I issue a consolidated finding:

**R4-008 [严重] [writer] -- Section III-C (Python and CLI Interface, lines 49-74) is a README excerpt, not academic prose.** This subsection lists:
- 6 CLI commands in an itemized list with full argument syntax
- 4 Python function signatures with argument names
- Configuration file reference syntax

This violates the fundamental principle that a peer-reviewed paper explains *what* the system does and *why*, not *how* to type specific commands. The fix is straightforward: **delete the itemized lists and replace them with a single descriptive paragraph** as suggested in R3-007.

**R4-009 [中等] [writer] -- Line 24: Xacro preprocessing command.** The literal shell command `\texttt{python -m xacro robot.urdf.xacro -o robot.urdf}` remains in the text despite being flagged in R3-008. Replace with: "Xacro files must be preprocessed to standard URDF before use (via the xacro macro processor)."

**R4-010 [中等] [writer] -- Line 81: Random seed value.** "fixed at \texttt{0xC0FFEE}" uses \texttt{} for a hex constant. Replace with "fixed at $0x C0FFEE$" or "fixed at 0xC0FFEE (decimal 12,646,126)".

**R4-011 [轻微] [writer] -- Reproducibility paragraph mixes prose with code references.** Lines 79-82 mention Docker, random seeds, validate, and compare in \texttt{}. These are the final three uses of \texttt{} in the file. They should be converted to plain prose to match the rest of the file.

---

### 4. Capsule Approximation Method (method.tex)

**Verdict: Strong section.** The method is clearly described with mathematical notation. The three remaining \texttt{} instances (lines 162, 163, 165) are XML element names in the URDF emission subsection. These are acceptable as format terminology, but see the code-reference discussion above for the global standard.

**R4-012 [轻微] [writer] -- Figure 1 (method schematic) reference in method.tex.** Line 31: "Figure~\ref{fig:method_schematic} illustrates the full pipeline visually." The figure exists and is correctly placed. Caption mentions "FR3 link (link3)". However, the PCA axis overlay in panel (a) is the same link shown in Figure 2. This repetition is fine -- different views serve different purposes -- but the caption should note that Figure 1 shows the *same link* as Figure 2 for cross-reference.

---

### 5. Validation Metrics and Tuning (validation_metrics.tex)

**Verdict: Excellent.** This section was heavily rewritten per R3-013 and is now clean. Parameter names use mathematical notation ($N$, $K_{\max}$, $\tau_{\mathrm{coa}}$, $\rho$, $\eta$, $\delta_{\min}$). No \texttt{} instances remain. The Monte Carlo sampling method is clearly described. The preset definitions at line 82 give concrete values for all parameters.

**No new issues.** This section is ready for publication.

---

### 6. Experiments (experiments.tex)

**Verdict: Strong section.** All quantitative values match artifact data. The writer cleaned up file paths (R3-018), Docker image names (R3-019), and code references (R3-020). The experiments section has 0 remaining \texttt{} instances.

**R4-013 [中等] [writer/implement] -- The "link2" claim for adaptive-circle r/binMed spike (line 167) remains unverifiable.** "the radius-inflation metric r/binMed spikes to 2.05 on link2" is stated as a fact, but the artifact only contains aggregate worst-case data. The specific link identity (link2) is not traceable to any artifact output. This was first flagged in R3-021 and remains unresolved. Either (a) provide per-link data confirming link2, or (b) soften to "some link" or "one link."

**R4-014 [轻微] [writer] -- The findings summary (lines 182-189) is well-written.** Findings 1-6 are precise and traceable. Finding 6 ("no analytic-primitive mode achieves full coverage") repeats the abstract's key finding. This is acceptable for emphasis.

**R4-015 [轻微] [writer] -- Table 2 footnote: "Dash (---) indicates metric not applicable."** This is correct and clear. However, the same footnote repeats verbatim in the table. A single well-placed caption sentence would suffice.

---

### 7. Discussion (discussion.tex)

**Verdict: Balanced and well-written.** The "When to Use Each Mode" subsection gives equal treatment to all three modes. The PCA limitation is honestly discussed. The URDF compatibility constraint is properly contextualized.

**R4-016 [轻微] [writer] -- Lines 57, 60: Remaining \texttt{validate} and \texttt{compare} references.** Two \texttt{} instances remain in discussion.tex (lines 57, 60), referencing CLI commands. These should be replaced with descriptive prose. This was flagged in R3-024 and R3-025.

**R4-017 [轻微] [writer] -- The "Reproducibility and Practical Use" subsection (lines 53-61) reads as a workflow guide.** This subsection describes "the recommended workflow" --- start with default, inspect metrics, adjust, compare. This is useful for users but reads as documentation. Consider moving this content to the project README and leaving discussion.tex to discuss only the *why* (principles), not the *how* (workflow).

---

### 8. Conclusion (conclusion.tex)

**Verdict: Good.** The conclusion restates the main contributions, summarizes the pipeline, and lists five future work directions. No overclaiming.

**R4-018 [轻微] [writer] -- Conclusion paragraph 1 is dense.** It recites the full pipeline algorithm (10 discrete steps: PCA sectioning, adaptive circle fitting, collinear merging, coverage-preserving growth, etc.). Consider a shorter summary for the first paragraph and a second paragraph elaborating the pipeline steps. This was partially addressed from R3-027 but still reads as a single block.

**R4-019 [轻微] [writer] -- Future work: "Learning-based fitting" (line 32).** This paragraph is speculative ("could predict capsule configurations directly from mesh topology"). It is the only future work item without a concrete precedent or related reference. Consider either adding a citation to a relevant learning-based geometry-processing paper, or tempering the claim to "could be explored."

---

## Citation Calibration Status

All 27 citations in `ref.bib` were verified in previous reviews (iter_3 calibration table). I confirm no new citations have been added since iter_3, and no previously verified citations have been modified.

| Citation Key | Calibration Status | Notes |
|---|---|---|
| `urdf_doc` | Verified (iter_3) | Unchanged |
| `xacro` | Verified (iter_3) | Unchanged |
| `todorov2012mujoco` | Verified (iter_3) | DOI present |
| `drake` | Verified (iter_3) | Unchanged |
| `coumans2018pybullet` | Verified (iter_3) | Unchanged |
| `pan2012fcl` | Verified (iter_3) | DOI present |
| `lien2008acd` | Verified (iter_3) | CAGD journal version |
| `mamou2016vhacd` | Verified (iter_3) | GTC talk |
| `barber1996quickhull` | Verified (iter_3) | DOI present |
| `cgal` | Verified (iter_3) | Unchanged |
| `jacobson2018libigl` | Verified (iter_3) | @misc |
| `huang2022manifoldplus` | Verified (iter_3) | 2020 arXiv, correct |
| `bradshaw2002sphere` | Verified (iter_3) | Trinity College Dublin |
| `mlund_spheretree` | Verified (iter_3) | Unchanged |
| `wu2018capsule` | Verified (iter_3) | CRITICAL FIX: now correct paper |
| `larsen2000ssv` | Verified (iter_3) | ICRA 2000, DOI |
| `larsen1999ssv` | Verified (iter_3) | Uncited, bibliographic only |
| `koptev2023neural` | Verified (iter_3) | RA-L, DOI |
| `eigen` | Verified (iter_3) | Unchanged |
| `urdfdom` | Verified (iter_3) | Unchanged |
| `welzl1991mec` | Verified (iter_3) | DOI present |
| `robot_viewer` | Verified (iter_3) | Unchanged |
| `franka2020fr3` | Verified (iter_3) | Year 2023, correct |
| `trimesh` | Verified (iter_3) | Unchanged |
| `sdformat` | Verified (iter_3) | Unchanged |
| `coumar2025foam` | Verified (iter_3) | Newly added |
| `lauterbach2010gproximity` | Verified (iter_3) | Newly added, DOI |

**Update record:** All 27 citations remain in the same verification state as iter_3. No new citations added. No citation status changed since iter_3.

---

## Artifact Traceability Audit

| Claim in paper | Artifact source | Status |
|---|---|---|
| Table 2 (mode comparison): all numeric values | `exp-fr3-mode-comparison/RESULT.md` aggregate table | PASS (all within rounding tolerance) |
| Table 3 (ablation): all 11 rows | `exp-capsule-ablation/RESULT.md` | PASS (all within 3 significant figures) |
| "NSections is the dominant control parameter" (experiments.tex L157) | Ablation data, rows nsections_2 through nsections_8 | PASS |
| "AdaptiveCircleCount increases capsule count 3.9x" (L166) | Ablation data (39/10 = 3.9) | PASS |
| "r/binMed spikes to 2.05 on link2" (L167) | No per-link data in artifact | **FAIL** -- link identity unverifiable (R4-013) |
| "high_detail preset achieves best worst-case uncovered distance (2.3 mm)" (L108) | Ablation nsections_6 row (2.338 mm = 2.3 mm rounded) | PASS |
| "Convex mode is fastest (0.5 s)" (L183) | Mode comparison data (0.468 s = 0.5 s) | PASS |
| "Sphere default leaves all links with small uncovered regions (1.8e-2 m worst-case)" (L104) | Mode comparison: sphere default worst_dist = 0.017608 m | PASS (1.8e-2 = 0.018, rounded) |
| "Capsule modes output 8-10 primitives" (L107) | Mode comparison: 8, 10, 10 | PASS |
| Figure 1 (method schematic) | `figures/capsule_method_schematic.pdf` exists | PASS |
| Figure 2 (qualitative overlay) | `figures/fig_qualitative_overlay.pdf` exists (3.2 MB) | **PASS WITH CAVEAT** (caption caveat R4-001) |

---

## Loop Triage

| issue_id | severity | owner | blocking | required_action |
|---|---|---|---|---|
| R4-008 | 严重 | writer | no (loop decides) | Rewrite Section III-C (system_overview.tex lines 49-74) to remove CLI listings and function signatures. Replace with single descriptive paragraph. |
| R4-009 | 中等 | writer | no | Remove literal shell command (line 24): replace `\texttt{python -m xacro...}` with prose description. |
| R4-013 | 中等 | implement | no | Provide per-link data identifying which link produces the r/binMed=2.05 spike for adaptive circles, or authorize writer to soften claim. |
| R4-010 | 中等 | writer | no | Replace `\texttt{0xC0FFEE}` with hex notation in math mode or plain prose. |
| R4-004 | 轻微 | writer | no | Clarify "analytic-primitive" in abstract to explicitly exclude convex hulls. |
| R4-006 | 轻微 | writer | no | Rephrase contribution bullet 4 from "protocol" to "results." |
| R4-001 | 轻微 | writer | no | Strengthen Figure 2 caption caveat about Trimesh vs CGAL convex hull. |
| R4-016 | 轻微 | writer | no | Replace `\texttt{validate}` and `\texttt{compare}` in discussion.tex lines 57, 60 with descriptive prose. |
| R4-019 | 轻微 | writer | no | Add citation or temper claim for learning-based fitting future work. |

### Global code-reference escalation

The 48 remaining `\texttt{}` instances (42 in system_overview.tex alone) constitute a **severe** violation per reviewer.md criteria (threshold: 10). However, the writer has argued that some of these are legitimate format terminology. The loop coordinator needs to decide:

**Option A (strict):** Enforce the 5-instance cap. Require writer to rewrite system_overview.tex to eliminate all CLI commands, function signatures, shell commands, and all but 5 total `\texttt{}` instances. This would require a substantial rewrite of Section III.

**Option B (relaxed):** Accept up to 15 `\texttt{}` instances for URDF XML element names and JSON field names (format terminology), but require removal of all CLI commands (12+ instances), Python function signatures (6+ instances), and the shell command (1 instance). This is a moderate rewrite of Section III-C.

**Option C (minimal):** Accept the current state, noting in a final note that 8-10 `\texttt{}` instances are CLI/signature references that academic rigor normally would exclude, but the paper's toolchain nature makes them informative.

I recommend **Option B** as the pragmatic compromise: the XML and JSON references are meaningful format documentation, but the CLI listings and function signatures have no place in academic prose.

---

### Implementation Request Specifications

**request_id:** (none new -- R4-013 refers to existing `exp-capsule-ablation` artifact gap)

The per-link data gap from R3-017/R3-021 persists. The implement agent should either:
- Regenerate with per-link metric output, OR
- Provide a statement that per-link data cannot be produced without rerunning the experiment.

---

## Summary of Priority Actions

1. **[HIGH] Resolve code-reference standard** -- The loop coordinator must decide whether to enforce the 5-instance cap (Option A) or accept a relaxed standard (Option B/C). This is a policy decision, not a writer decision. `owner: loop/user`

2. **[HIGH] Rewrite Section III-C** -- Regardless of the option chosen above, the CLI command listing (12+ instances) and Python function signatures (6+ instances) must be removed. These are objectively out of scope for a peer-reviewed paper. `owner: writer`

3. **[MEDIUM] Resolve per-link data gaps** -- The "link2" claim for the adaptive-circle r/binMed spike cannot be verified from existing artifacts. Either provide data or soften the claim. `owner: implement`

4. **[MEDIUM] Remove shell command and seeded-constant \texttt{}** -- Lines 24 and 81 in system_overview.tex. `owner: writer`

5. **[LOW] Language polishing** -- Abstract clarification (R4-004, R4-005), contribution bullet 4 strengthening (R4-006), Figure 2 caption caveat (R4-001), learning-based citation (R4-019), discussion workflow guide (R4-017).
