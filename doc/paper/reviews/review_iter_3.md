# Review: Iteration 3

## Structure Overview

**Paper topic:** URDFApproxGeom -- Automatic Primitive Collision Approximation for Robot URDF Models

**Core question addressed:** How can mesh-based URDF collision geometry be automatically converted into lightweight analytic primitives (convex hulls, sphere trees, capsule primitives)?

**Overall structure assessment:** The paper follows a clear IMRaD structure and the narrative is predominantly **problem-driven** (geometry approximation need), not capsule-driven. The introduction opens with the general collision geometry problem rather than leading with capsule fitting. This is commendable.

---

### Narrative Balance Statistics

Quantitative estimation of the three methods' coverage across sections:

| Section | Convex | Sphere | Capsule | General/Other |
|---|---|---|---|---|
| Abstract | ~10% | ~10% | ~35% | ~45% |
| Introduction | ~10% | ~10% | ~40% | ~40% |
| Related Work | ~25% | ~25% | ~25% | ~25% |
| System Overview | ~15% | ~15% | ~20% | ~50% |
| Method (capsule fitting) | 0% | 0% | ~90% | ~10% |
| Validation Metrics | ~5% | ~5% | ~60% | ~30% |
| Experiments | ~15% | ~15% | ~55% | ~15% |
| Discussion | ~15% | ~15% | ~50% | ~20% |
| Conclusion | ~10% | ~10% | ~40% | ~40% |

**Overall capsule proportion across all sections: approximately 45-50%.** This is on the boundary of the 50% threshold but not a clear violation. Capsule fitting is _the_ main methodological contribution, so its dominance in the Method section is correct. Convex and sphere receive genuinely equal treatment in Related Work, System Overview, and Discussion.

**Verdict:** Narrative balance is **acceptable** -- capsule does not exceed 50% of total content. The paper frames itself as a three-mode toolchain with capsule as the primary technical contribution, which matches the outline's intent.

---

## Section-by-Section Review

### Abstract

**File:** `sections/abstract.tex`

**R3-001** [轻微] [writer] -- The abstract lists three modes "convex hulls, sphere trees, and capsule primitives" in the first sentence, which is good. But the final sentence "finding that no analytic-primitive configuration achieves complete vertex coverage on all links" is slightly ambiguous -- does "analytic-primitive" include convex? Convex hulls _do_ achieve full coverage (as the paper later confirms). Clarify that "analytic-primitive" excludes mesh-based convex output.

**Suggestion:** "...finding that no analytic-primitive (sphere or capsule) configuration achieves complete vertex coverage on all links---a fundamental representation trade-off."

**R3-002** [轻微] [writer] -- The phrase "the system is released as an open-source Python package" is a space-filler. For a 150-word abstract, every word counts. Either mention a specific differentiator (e.g., "with built-in validation gates and preset-based configuration") or cut the sentence shorter: "The open-source Python package includes a CLI, public API, validation metrics, and preset-based configuration."

---

### 1. Introduction

**File:** `sections/introduction.tex`

**R3-003** [轻微] [writer] -- The introduction correctly opens with the problem context (geometry approximation for collision) rather than leading with capsule. Paragraph 5 mentions all three modes. However, paragraph 3 ("Analytic collision primitives offer significant advantages") exclusively argues for capsules before the reader knows the toolchain has three modes. This creates a subtle capsule-priming effect.

**Suggestion:** At line 20-24, broaden the framing: "Analytic collision primitives---spheres, capsules, and boxes---support stable signed-distance evaluation... Capsules (swept spheres) are particularly well suited to robot links because..." This makes clear that the paragraph is about analytic primitives generally, with capsule as one example within that class.

**R3-004** [中等] [writer] -- Contribution bullet 4 (lines 47-48): "An experimental protocol and reference results on the Franka FR3 robot." This is a _protocol_, not a _result_. If the experiments exist, rephrase as a results/evidence claim (e.g., "Experimental evaluation on the Franka FR3 robot characterizing coverage, tightness, and runtime trade-offs across all three modes and a capsule ablation study"). The current wording sells the paper short by framing it as a methodology contribution when data exists.

---

### 2. Related Work

**File:** `sections/related_work.tex`

**R3-005** [轻微] [writer] -- Section II.C (Sphere-Tree Approximation) line 26: "Unlike Foam's spherical-only output, our toolchain additionally produces capsule and convex approximations with analytic parameter export through a JSON sidecar." This comparison is good, but it sits at the end of a paragraph. Consider giving Foam its own sentence earlier in the paragraph so the reader understands what Foam is before reading the comparison. As written, the Foam citation `coumar2025foam` is mentioned only tangentially.

**R3-006** [轻微] [writer] -- Section II.D line 31-33 references `larsen2000ssv` for swept-sphere volumes. The sentence references "capsule collision primitives have been used in several simulation and planning frameworks" and cites Drake and "the generation of these primitives from existing robot URDF meshes has remained a manual process." The transition from swept-sphere theory to manual-generation gap is abrupt. Add a bridging sentence: "Despite the utility of swept-sphere representations, tools for _automatically_ converting existing mesh-based robot descriptions into capsule primitives have not been available."

---

### 3. System Overview

**File:** `sections/system_overview.tex`

**R3-007** [严重] [writer] -- This entire section reads as a README/CLI manual rather than a peer-reviewed methods description. See the dedicated "Code Reference and Documentation Language" section below for the full accounting of `\texttt{}` violations. Specifically:

- Lines 49-74 (Section III-C Python and CLI Interface): This is the worst offender. Listing individual CLI commands and Python function signatures with their argument names is pure documentation. The reader does not need to know that `generate(mode, input_urdf, output_urdf, ...)` is the function signature; they need to understand _what capabilities_ the API provides.

**Suggestion:** Replace the CLI/Python subsections with a single paragraph:
> "The toolchain exposes three core operations: generation (producing URDF output for any mode), validation (computing coverage and tightness metrics from generated output), and comparison (evaluating quality differences between configurations). These are accessible both through a command-line interface and a Python API, with the latter supporting batch processing across all modes and presets."

Delete the itemized CLI command list entirely. It is academically inappropriate.

**R3-008** [中等] [writer] -- Line 23-24: "Xacro files must be pre-processed to standard URDF before use (`\texttt{python -m xacro robot.urdf.xacro -o robot.urdf}`)." This is the worst form of documentation language -- a literal shell command. Replace with: "Xacro files must be preprocessed to standard URDF before use (via the xacro macro processor)."

---

### 4. Capsule Approximation Method

**File:** `sections/method.tex`

**R3-009** [中等] [writer] -- The parameter `\texttt{NSections}` (line 67) appears in `\texttt{}` despite the surrounding text already defining $N$ as the section count. The pattern "$N$ ... controlled by the parameter \texttt{NSections}" introduces a code identifier for a concept already expressed in mathematical notation. Replace: "The section count $N$ controls the number of axial slicing planes; typical values range from 2 (coarse) to 8 (fine)."

**R3-010** [中等] [writer] -- Lines 81, 95: `\texttt{AdaptiveCircleCount}` and `\texttt{MaxCirclesPerSection}` should use math notation. Since these are boolean/numeric parameters, define a boolean flag $\alpha$ (adaptive flag) and $K_{\max}$ (maximum circles per section) and use them consistently.

**R3-011** [轻微] [writer] -- Lines 141-142, 154: `\texttt{MinSplitVolumeImprovement}`, `\texttt{capV/aabb}`, `\texttt{MaxCapVAabbRatio}`, `\texttt{MaxCapsulesPerLink}` all need the same treatment. The notation $K_{\max}$ is already introduced. Use $\tau_{\mathrm{split}}$ for the split improvement threshold and $\rho_{\max}$ for the capV/aabb ceiling.

**R3-012** [中等] [writer] -- Lines 162-165: `\texttt{<cylinder>}` and `\texttt{<sphere>}` are XML element names. These are acceptable as _one_ or _two_ isolated uses when explaining the URDF format constraint, but using them inside `\texttt{}` in consecutive list items is excessive. Write as inline code once ("cylinder and sphere XML elements") and move on. The repeated `\texttt` in a three-item list is jarring.

---

### 5. Validation Metrics and Tuning

**File:** `sections/validation_metrics.tex`

**R3-013** [严重] [writer] -- Lines 59-69: The parameter list uses `\texttt{}` for every single parameter name (`\texttt{NSections}`, `\texttt{MaxCapsulesPerLink}`, `\texttt{CoaThreshold}`, `\texttt{MaxCirclesPerSection}`, `\texttt{AdaptiveCircleCount}`, `\texttt{MaxRadiusBinRatio}`, `\texttt{MaxCapVAabbRatio}`, `\texttt{MinSplitVolumeImprovement}`, `\texttt{UnionVolumeSamplesPerAxis}`). That is 9 instances of `\texttt{}` in a single subsection. Combined with the preset names (`\texttt{single}`, `\texttt{default}`, `\texttt{high\_detail}`) at line 71, this becomes a code dump.

**Suggestion:** Replace the entire parameter list with a unified mathematical notation table:

```latex
\begin{table}[t]
\centering
\caption{Capsule fitting parameters and their default values.}
\label{tab:params}
\begin{tabular}{@{}lccl@{}}
\toprule
Symbol & Default & Description \\
\midrule
$N$ & 4 & Axial slicing planes \\
$K_{\max}$ & 12 & Max capsules per link \\
$\tau_{\mathrm{COA}}$ & 0.005 & Circle-outside-area threshold \\
$m_{\max}$ & 1 & Max circles per section \\
$\alpha$ & false & Adaptive circle count flag \\
$\rho_{\mathrm{bin}}$ & 1.45 & Max radius-inflation ratio \\
$\rho_{\max}$ & $-$1.0 & Max capV/aabb ceiling \\
$\tau_{\mathrm{split}}$ & 0.005 & Min split volume improvement \\
$S$ & 32 & Monte Carlo samples per axis \\
\bottomrule
\end{tabular}
\end{table}
```

This eliminates 9 `\texttt{}` instances at once and is far more readable.

**R3-014** [中等] [writer] -- Lines 7-8: "These metrics are computed by the `\texttt{validate}` CLI command." The validate command is a tool implementation detail. Replace: "These metrics are computed by the toolchain's validation module."

**R3-015** [轻微] [writer] -- Line 15: "The `\texttt{all\_covered}` flag is `\texttt{true}` when...". Use mathematical notation: "The all-covered condition $\mathcal{C}_{\mathrm{cov}}$ holds when..." instead of flag names.

---

### 6. Experiments

**File:** `sections/experiments.tex`

**R3-016** [严重] [implement] -- **No qualitative comparison figures exist.** The paper has exactly one figure (the method schematic). There are zero figures showing side-by-side visual comparison of the original mesh against the three approximation modes. The outline (Figure 2: "Qualitative visual/collision overlay for one representative link") and the artifact (`exp-fr3-mode-comparison/RESULT.md` lines 38, 68) both acknowledge this gap: "No overlay screenshots exist in the artifact."

This is a **severe** deficiency for a geometry processing paper. Without visual evidence, the reader cannot assess what "tightness" or "coverage" means in geometric terms. A table of numbers does not communicate shape fidelity.

**Action required:** Generate at least one figure showing a representative FR3 link (e.g., link3) with four panels: (a) original mesh overlay, (b) convex hull, (c) sphere tree (default), (d) capsule (default). Render these as transparent overlays on the original mesh using PyBullet or similar. Place this figure at the end of Section VI-E or the start of Section VI-A.

**R3-017** [中等] [implement] -- The `exp-fr3-mode-comparison` artifact lacks per-link metric files. The aggregate data supports worst-case claims but conceals per-link behavior. The paper acknowledges identical total primitive counts for `default` and `high_detail` (both = 10) and attributes this to "slightly different per-link distribution" (line 105-106), but this claim is unverifiable without per-link data.

**Action required:** Generate per-link metrics (primitive count per link, coverage per link, capV/aabb per link) for all 6 presets × 7 links and include them either as a supplementary table or a statement about which specific links differ.

**R3-018** [轻微] [writer] -- Lines 26-28: "The URDF model is bundled in the repository under `\texttt{resources/fr3/}` and includes both visual meshes (COLLADA `\texttt{.dae}` files)." This is a file path and file extension. Replace: "The FR3 URDF model includes both visual meshes (in COLLADA format, representing the true link geometry) and collision meshes (pre-simplified STL files)."

**R3-019** [中等] [writer] -- Line 66: "The pipeline is invoked via Docker (`\texttt{urdfapprox:1.5.0}`), using the Python API with `\texttt{mesh\_source="visual"}`." A Docker image name and a Python API keyword argument have no place in an academic paper. Replace: "All experiments used the default visual mesh source, with the pipeline running in a containerized environment for reproducibility."

**R3-020** [轻微] [writer] -- Lines 70-108: The terms `\texttt{capV/aabb}`, `\texttt{r/binMed}`, `\texttt{high\_detail}`, `\texttt{single}`, `\texttt{default}` appear repeatedly in `\texttt{}`. Once mathematical notation is established (see R3-013), use $\mathrm{capV/aabb}$ and $\rho_{\mathrm{bin}}$ consistently instead of code-verbatim names.

**R3-021** [中等] [implement] -- Line 116: Ablation table shows `adaptive_true` produces a spike in r/binMed to 2.05 (line 155), but the table only reports worst-case metrics. The claim "on link2" at line 155 is not verifiable from the aggregate artifact data -- per-link data would be needed to confirm which link produces the spike. Either provide per-link data or soften the claim to "some link."

**R3-022** [轻微] [writer] -- Line 160: "Mesh source (visual vs. collision) produces identical capsule fits on the FR3." The artifact supports this claim but the paper should add a caveat: "(FR3 collision meshes are convex envelopes of the visual meshes; this finding may not generalize to robots where collision meshes diverge more significantly from visual geometry.)" -- this caveat is already present at line 161, so no change needed. Good.

---

### 7. Discussion

**File:** `sections/discussion.tex`

**R3-023** [轻微] [writer] -- Section VII-A: The "When to Use Each Mode" section is the best balanced part of the paper. It gives equal treatment to all three modes with clear use-case guidance. No changes needed.

**R3-024** [中等] [writer] -- Lines 55-59: "The built-in validation gates allow users to define acceptance criteria... via the `\texttt{validate}` and `\texttt{compare}` CLI commands." Replace with: "The built-in validation gates allow users to define acceptance criteria (e.g., $\mathrm{capV/aabb} < 2.5$) and check them programmatically."

**R3-025** [轻微] [writer] -- Line 59: "The recommended workflow is to start with the `\texttt{default}` preset, inspect the metrics via `\texttt{validate}`, adjust parameters iteratively, and use `\texttt{compare}` to verify that the candidate preset improves upon the baseline." Three `\texttt{}` uses in one sentence. Replace: "The recommended workflow starts from the default configuration, inspects the metrics, adjusts parameters iteratively, and verifies improvement against the baseline." The words "default", "validate", and "compare" need no special formatting in prose.

**R3-026** [轻微] [writer] -- The Discussion mentions downstream uses (GPU pipelines, signed-distance fields, swept-volume computation) but does not explicitly name **collision detection**, **motion planning**, or **analytic distance queries** as concrete downstream applications. Add one sentence:
> "These analytic primitives support constant-time collision distance evaluation, making them suitable for collision-checking in motion planning, real-time simulation, and differentiable collision pipelines."

---

### 8. Conclusion

**File:** `sections/conclusion.tex`

**R3-027** [轻微] [writer] -- The Conclusion paragraph is dense and highly technical. It reiterates the full pipeline algorithm. Consider a more concise restatement. For example:
> "We have presented URDFApproxGeom, a toolchain for automatic conversion of mesh-based URDF collision geometry into convex, sphere-tree, and capsule approximations with a unified CLI and Python API. The main contribution is the automatic capsule-fitting pipeline, which produces coverage-preserving analytic primitives while maintaining URDF compatibility via cylinder-sphere decomposition and a JSON sidecar."

This cuts about 40% of the text while preserving all claims.

**R3-028** [轻微] [writer] -- "Tighter integration with collision libraries" (line 28-30) mentions FCL and Drake. This paragraph is the paper's only explicit bridge to downstream collision-checking use cases. It is good but slightly generic. Strengthen: "The JSON sidecar format enables native reading of capsule parameters ($p_0$, $p_1$, $r$) by collision-checking libraries such as FCL and the Drake geometry framework, eliminating the need to reconstruct primitives from URDF's decomposed representation."

---

## Qualitative Visualization Review

**Status: SEVERE GAP**

The paper has exactly **one figure**: Figure 1 (method schematic of capsule pipeline). Missing:

| Figure | Status | Required Action |
|---|---|---|
| Qualitative overlay: mesh vs convex vs sphere vs capsule for one link | **MISSING** | Implement must generate this |
| Qualitative overlay comparing presets (single vs default vs high_detail) | **MISSING** | Implement or writer (if screenshots are trivial) |
| Side-by-side: output geometry overlaid on original mesh with transparency | **MISSING** | Implement must generate this |

The exp-fr3-mode-comparison artifact acknowledges this gap (RESULT.md line 38: "Qualitative overlay screenshots -- not yet generated", line 68: "No overlay screenshots exist in the artifact"). This is a blocking issue for a geometry processing paper. **Without qualitative evidence, the paper is incomplete.**

**Recommendation:** The most impactful single figure would be a 2×2 grid showing FR3 link3 with:
- (a) Original visual mesh (opaque)
- (b) Convex hull overlay (transparent, colored)
- (c) Sphere tree overlay (transparent, colored)
- (d) Capsule overlay (transparent, colored)

This can be generated using PyBullet or a similar renderer that the toolchain already supports via `visualize` command.

---

## Downstream Applications Bridge Review

**Status: ADEQUATE but could be stronger**

- Discussion Section VII-A mentions: "GPU-based collision pipelines", "signed-distance fields via sphere-sums", "swept-volume computation", "analytical signed-distance fields for motion planning" -- these are concrete applications. Good.
- Discussion Section VII-B discusses: URDF compatibility limitation and JSON sidecar value. Good.
- Conclusion Section VIII-B mentions: FCL and Drake integration.
- **Missing:** No explicit mention of **collision detection** as the primary downstream application. The words "collision detection" appear only in the abstract context of the toolchain's function, not as a downstream domain that benefits from the output.
- **Missing:** The JSON sidecar's specific value (p0/p1/radius don't need to be reconstructed from URDF) is mentioned in System Overview (Section III-C) and Method (Section IV-G), but not synthesized in Discussion as a concrete benefit for downstream users.

**Suggestion:** Add 2-3 sentences to Section VII ("Discussion") explicitly addressing:
> "The JSON sidecar stores canonical capsule parameters $(p_0, p_1, r)$ that collision-checking libraries can read directly, avoiding reconstruction from decomposed URDF primitives. This enables downstream applications---collision detection in motion planning, signed-distance queries for trajectory optimization, and broad-phase culling in simulation---to use the analytic representation without simulator-specific parsing."

---

## Metric Consistency Review

**Status: MOSTLY GOOD with one gap**

- Coverage metric: reported for all modes (convex=yes, sphere=no, capsule=no). Consistent.
- Primitive count: reported for all modes. Consistent.
- capV/aabb: clearly marked as capsule-specific (table footnote). Consistent.
- r/binMed: clearly marked as capsule-specific. Consistent.

**Gap:** There is no volumetric tightness metric for convex or sphere modes. The paper uses "conservative coverage" for convex and "medial-axis approximation" for sphere but never quantifies "how tight" the convex hull is relative to the original mesh. Consider adding a mesh-to-mesh volume ratio for convex mode, even if it's not directly comparable to capV/aabb.

**Status:** Not a blocking issue but worth noting.

---

## Code Reference and Documentation Language Review

**Severity: SEVERE**

Total `\texttt{}` instances across the paper: **~75 instances** (estimated from count below)

Breakdown by file:

| File | `\texttt{}` count | Primary content |
|---|---|---|
| `system_overview.tex` | ~30 | CLI commands, Python function signatures, file paths, XML elements, flags, Docker reference |
| `method.tex` | ~10 | Parameter names, XML elements |
| `experiments.tex` | ~20 | Parameter names, file paths, Docker image name, preset names |
| `validation_metrics.tex` | ~15 | Parameter names, flag names, field names, CLI commands, preset names |
| `discussion.tex` | ~3 | CLI commands, preset names |
| `related_work.tex` | ~1 | Library name |

**Total: ~75 instances** -- far exceeding the 10-instance threshold for "severe" classification.

### Classification by type:

**TYPE 1: CLI commands and flags (SEVERE)** -- Must be removed:
- `\texttt{generate}`, `\texttt{presets}`, `\texttt{validate}`, `\texttt{compare}`, `\texttt{compare-all}`, `\texttt{visualize}` (system_overview.tex)
- `\texttt{--mode convex|sphere|capsule|all}`, `\texttt{--require-improvement}`, `\texttt{--config}` (system_overview.tex)
- `\texttt{--mesh-source visual}`, `\texttt{--mesh-source collision}` (system_overview.tex)
- `\texttt{python -m xacro robot.urdf.xacro -o robot.urdf}` (system_overview.tex)

**TYPE 2: Python API function signatures (SEVERE)** -- Must be removed:
- `\texttt{generate(mode, input\_urdf, output\_urdf, ...)}`, `\texttt{generate\_all(...)}`, `\texttt{generate\_capsule\_multi(...)}`, `\texttt{generate\_sphere\_pair(...)}` (system_overview.tex)
- `\texttt{GenerateResult}` (system_overview.tex, validation_metrics.tex)

**TYPE 3: Config parameter names (SEVERE in volume)** -- Must be replaced with math notation:
- `\texttt{NSections}`, `\texttt{MaxCapsulesPerLink}`, `\texttt{CoaThreshold}`, `\texttt{MaxCirclesPerSection}`, `\texttt{AdaptiveCircleCount}`, `\texttt{MaxRadiusBinRatio}`, `\texttt{MaxCapVAabbRatio}`, `\texttt{MinSplitVolumeImprovement}`, `\texttt{UnionVolumeSamplesPerAxis}`, `\texttt{MaxRadiusBinRatio}`, `\texttt{MinSplitVolumeImprovement}` (multiple files)

**TYPE 4: File paths and extensions (SEVERE)** -- Must be removed:
- `\texttt{resources/fr3/}` (experiments.tex)
- `\texttt{.dae}`, `\texttt{.json}`, `\texttt{.urdf}`, `\texttt{.obj}` (system_overview.tex, method.tex, experiments.tex)

**TYPE 5: Docker image name (SEVERE)** -- Must be removed:
- `\texttt{urdfapprox:1.5.0}` (experiments.tex)

**TYPE 6: Preset names (MILD)** -- Acceptable in limited use:
- `\texttt{single}`, `\texttt{default}`, `\texttt{high\_detail}` -- These are named scientific configurations, not code per se. But 10+ uses spread across files is excessive. Use `default` as prose without formatting; reserve `\texttt{}` only when first introducing the preset name.

**TYPE 7: XML element names (MILD)** -- Acceptable if minimal:
- `\texttt{<mesh>}`, `\texttt{<collision>}`, `\texttt{<visual>}`, `\texttt{<inertial>}`, `\texttt{<joint>}`, `\texttt{<cylinder>}`, `\texttt{<sphere>}`, `\texttt{<sphere>}` -- These are URDF format terminology. Acceptable in small doses but excessive in system_overview.tex (appear 8+ times). Use descriptive text: "cylinder and sphere XML elements" rather than repeated `\texttt`.

**TYPE 8: Internal flag names (MODERATE)** -- Should be removed:
- `\texttt{all\_covered}`, `\texttt{true}`, `\texttt{primitive\_count}`, `\texttt{false}` (validation_metrics.tex)
- `\texttt{0xC0FFEE}` (system_overview.tex -- random seed value)

**TYPE 9: Library name (ACCEPTABLE per code-reference rules)**:
- `\texttt{sphere\_tree}` (related_work.tex) -- This is the name of a vendored library; one instance is acceptable.

### Required Action for Writer:
1. Replace all TYPE 3 parameter names with the unified math notation table (see R3-013).
2. Rewrite Section III-C (CLI/Python interface) as prose, deleting all function signatures and CLI command listings.
3. Replace file paths (TYPE 4) with descriptive text.
4. Replace Docker image name (TYPE 5) with "the containerized pipeline."
5. Reduce preset-name `\texttt{}` to first-introduction only.

---

## Citation Calibration Status

All citations from the `citation-survey` artifact have been corrected in `ref.bib`:

| Citation Key | Calibration Status | Notes |
|---|---|---|
| `urdf_doc` | Verified | URL format acceptable |
| `xacro` | Verified | URL format acceptable |
| `todorov2012mujoco` | Verified | DOI present |
| `drake` | Verified | URL verified |
| `coumans2018pybullet` | Verified | Software reference |
| `pan2012fcl` | Verified | DOI present |
| `lien2008acd` | Verified | CAGD journal version, DOI present |
| `mamou2016vhacd` | Verified | GTC talk/software, acceptable |
| `barber1996quickhull` | Verified | DOI present |
| `cgal` | Verified | URL verified |
| `jacobson2018libigl` | Verified | Changed from phantom TOG to @misc |
| `huang2022manifoldplus` | Verified | Changed to 2020 arXiv, correct |
| `bradshaw2002sphere` | Verified | Trinity College Dublin, correct |
| `mlund_spheretree` | Verified | URL verified |
| `wu2018capsule` | Verified | **CRITICAL FIX APPLIED**: Changed from wrong (Zhirong Wu capsule network) to correct (Nannan Wu IEEE Access 2018) |
| `larsen2000ssv` | Verified | ICRA 2000, DOI present |
| `larsen1999ssv` | Verified | Tech report, uncited (bibliographic only) |
| `koptev2023neural` | Verified | RA-L with IROS note, DOI present |
| `eigen` | Verified | Software reference |
| `urdfdom` | Verified | URL verified |
| `welzl1991mec` | Verified | DOI present |
| `robot_viewer` | Verified | URL verified |
| `franka2020fr3` | Verified | Year corrected to 2023, URL corrected |
| `trimesh` | Verified | URL verified |
| `sdformat` | Verified | URL verified |
| `coumar2025foam` | Verified | Newly added (from survey recommendation) |
| `lauterbach2010gproximity` | Verified | Newly added (from survey recommendation), DOI present |

**Status: ALL 27 citations verified. No false, unverifiable, or incorrect citations remain.** The `mukadam2018trapezoidal` (unverifiable) has been fully removed from all tex files and ref.bib. The `wu2018capsule` critical fix has been correctly applied. All survey-recommended additions (Foam, gProximity) have been added.

---

## Loop Triage

| issue_id | severity | owner | blocking | required_action |
|---|---|---|---|---|
| R3-016 | 严重 | implement | yes | Generate qualitative overlay figure (mesh vs convex vs sphere vs capsule) for at least one representative link. Add to experiments section as Figure 2. |
| R3-007 | 严重 | writer | no | Rewrite Section III-C (CLI/Python interface) as descriptive prose. Delete all function signatures and CLI command listings. |
| R3-013 | 严重 | writer | no | Replace all `\texttt{}` parameter names with unified math notation table (see R3-013 for complete proposal). |
| R3-017 | 中等 | implement | no | Generate per-link metrics data for all 6 presets × 7 links to support per-link claims in experiments. |
| R3-021 | 中等 | implement | no | Provide per-link data or soften the "link2" claim in experiments.tex line 155. |
| R3-008 | 中等 | writer | no | Remove literal shell command from system_overview.tex. |
| R3-004 | 中等 | writer | no | Strengthen contribution bullet 4 to include experimental results. |
| R3-024 | 中等 | writer | no | Remove `\texttt{validate}` and `\texttt{compare}` from discussion.tex. |
| R3-026 | 轻微 | writer | no | Add explicit mention of collision detection and motion planning as downstream applications. |
| R3-027 | 轻微 | writer | no | Tighten conclusion paragraph. |

### Implementation Request Specification

**request_id:** fig-qualitative-overlay (new)
**paper_claim:** The three approximation modes produce geometrically distinct outputs with visibly different coverage patterns.
**task_type:** figure/generation
**required_outputs:** One 2×2 grid figure showing FR3 link3: (a) original mesh, (b) convex hull overlay, (c) sphere tree (default) overlay, (d) capsule (default) overlay. PNG at 300 DPI, suitable for IEEEtran single-column width. Original mesh rendered in gray, each approximation overlaid in a distinct translucent color.
**metrics:** Visual fidelity only -- no quantitative metrics needed.
**inputs:** FR3 URDF, generated outputs for all three modes (default presets), PyBullet or robot_viewer for rendering.
**acceptance_criteria:** Figure is legible at single-column width; overlay alignment is visually correct; colors are distinguishable in grayscale print.
**writer_integration_target:** `sections/experiments.tex` after line 93 (end of Section VI-E), with caption: "Qualitative comparison of approximation modes on FR3 link3. The original visual mesh is shown in gray; (a) convex hull, (b) sphere tree (default preset), and (c) capsule (default preset) are overlaid as translucent colored geometry."

---

## Summary of Priority Actions

1. **[BLOCKING] Generate qualitative overlay figure** -- Without this figure, the paper has no visual evidence of geometric quality for any mode. This is a blocking deficiency for a geometry processing paper.

2. **[HIGH] Eliminate documentation language** -- The paper has ~75 `\texttt{}` instances (severe by the reviewer's metric). The system overview section reads as a README. Section III-C must be completely rewritten. The parameter list in validation_metrics.tex must be converted to math notation.

3. **[HIGH] Generate per-link metrics** -- Several claims in the experiments section rely on per-link distribution information that does not exist in the artifacts.

4. **[MEDIUM] Strengthen downstream applicability** -- Add explicit mention of collision detection and motion planning as downstream beneficiaries.

5. **[LOW] Language polishing** -- Minor tightening of abstract and conclusion, fix the capsule-priming effect in introduction paragraph 3.
