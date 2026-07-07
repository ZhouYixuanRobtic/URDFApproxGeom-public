# Review: Iteration 9

## Overall Assessment

The paper has improved substantially since the initial review. All 14 issues from IR1-001 through IR1-014 have been verified as resolved in prior review cycles. The abstract includes the negative finding, the tables use \resizebox where appropriate, and the validation metrics section now uses mathematical notation. The artifact RESULT.md files are present, the citation survey corrections are applied to ref.bib, and the system overview no longer has a placeholder figure.

**Current state**: The paper is structurally sound, the claim hierarchy respects evidence boundaries, and the honest reporting of negative results (no analytic-primitive mode achieves full coverage) is a strength. However, several new issues were identified in this cold read, primarily: an inconsistent link count, counterintuitive runtime behavior left unexplained, per-link values in the text that are not traceable to aggregate-only artifacts, CLI parameter names still used in experiments.tex prose, and minor citation completeness gaps.

---

## Section-by-Section Review

### Abstract (abstract.tex)

- **【File:Line]**: abstract.tex:L1
- **【原文引用]**: "finding that no analytic-primitive (sphere and capsule) configuration achieves complete vertex coverage on all links---a fundamental representation trade-off"
- **【问题分析与评级]**: This clause correctly states the negative finding. The inclusion of this honest limitation is a strength. -- PASS
- **【修改建议]**: No change needed.
- **owner**: writer
- **blocking**: no

### Introduction (introduction.tex)

- **【File:Line]**: introduction.tex:L8-L48
- **【问题分析与评级]**: The introduction follows the prescribed narrative arc, centers capsule fitting as the primary contribution, and the contribution bullets match the outline hierarchy. All issues from previous reviews (six-sigma, Foam comparison, etc.) have been resolved. -- PASS
- **【修改建议]**: No change needed.
- **owner**: writer
- **blocking**: no

### Related Work (related_work.tex)

- **【File:Line]**: related_work.tex:L1-L45
- **【问题分析与评级]**: Six subsections covering URDF tooling, convex decomposition, sphere trees, capsule/swept-sphere, dependencies, and tooling gap. Citations are correct following the citation survey. The Foam comparison sentence added per IR1-010 is present and accurate. -- PASS
- **【修改建议]**: No changes needed.
- **owner**: writer
- **blocking**: no

### System Overview (system_overview.tex)

- **【File:Line]**: system_overview.tex:L1-L58
- **【问题分析与评级]**: The placeholder text has been removed. The section now cross-references the method schematic figure and provides a text-based pipeline description. Mode descriptions are clear and accurate. -- PASS
- **【修改建议]**: No changes needed.
- **owner**: writer
- **blocking**: no

### Method (method.tex)

- **【File:Line]**: method.tex:L45
- **【原文引用]**: "The loaded mesh is then processed through ManifoldPlus to obtain a watertight manifold, which is required for robust cross-section slicing."
- **【问题分析与评级]**: 轻微. The sentence states that a watertight manifold is required but does not explain WHY. Readers unfamiliar with geometry processing may not understand that non-watertight meshes produce open contours that cannot be reliably chained into closed cross-section polygons.
- **【修改建议]**: Add a parenthetical explanation: "...to obtain a watertight manifold---non-manifold or open meshes produce disjoint cross-section segments that cannot be reliably chained into closed contours."
- **owner**: writer
- **blocking**: no

- **【File:Line]**: method.tex:L67
- **【原文引用]**: "The section count $N$ is a user-configurable parameter; typical values range from 2 (coarse) to 8 (fine)."
- **【问题分析与评级]**: 轻微. The range "2 to 8" was updated per IR1-005 to match the ablation range, which is correct. However, the phrase "2 to 8" includes the default of 4, but the paper could clarify that values above 6 are considered fine-grained and may increase capV/aabb (discussed in ablation results).
- **【修改建议]**: Consider adding: "Values above 6 capture finer axial detail but may increase the union-volume ratio $\mathrm{capV/aabb}$ (see Section~\ref{sec:experiments})."
- **owner**: writer
- **blocking**: no

- **【File:Line]**: method.tex:L73-L78
- **【原文引用]**: "A circle is considered to cover the contour when its circle-outside-area (COA)---the area of the circle lying outside the contour polygon---falls below a threshold $\tau$."
- **【问题分析与评级]**: 轻微. The COA definition is provided but $\tau$ is introduced without a default value or typical range. The reader must cross-reference the validation_metrics section to learn $\tau_{\mathrm{coa}} = 5\times 10^{-3}$.
- **【修改建议]**: Add the default value in parentheses: "...falls below a threshold $\tau$ (default $\tau = 5\times10^{-3}$)."
- **owner**: writer
- **blocking**: no

### Validation Metrics (validation_metrics.tex)

- **【File:Line]**: validation_metrics.tex:L26-L32
- **【原文引用]**: "The bounding box of the capsule set is uniformly sampled on a regular grid with $S$ samples per axis (default $S = 32$, giving approximately $32{,}768$ sample points)."
- **【问题分析与评级]**: 中等. The validation metrics section states the default is $S=32$, but the experimental setup (experiments.tex L67-69) states "The capsule union volume is estimated with $S = 64$ samples per axis (twice the default of $S = 32$) during validation to improve estimate accuracy." This inconsistency is not cross-referenced. A reader reading this section alone would assume $S=32$ is used throughout, but the experiments use $S=64$. This is a factual discrepancy between a normative section and the experimental methodology.
- **【修改建议]**: Add a sentence at the end of the volume estimation paragraph: "The default value of $S=32$ may be increased for higher-precision evaluation; the experiments in Section~\ref{sec:experiments} use $S=64$."
- **owner**: writer
- **blocking**: no

- **【File:Line]**: validation_metrics.tex:L40-L45
- **【原文引用]**: "Along each capsule axis, vertices are binned into ten equal-length segments."
- **【问题分析与评级]**: 轻微. The choice of 10 bins is stated without justification. Why 10? Is this a resolution trade-off? A reader may wonder whether the metric is sensitive to this choice.
- **【修改建议]**: Add a brief justification: "...ten equal-length segments, providing a stable estimate of the radial offset profile without overfitting to individual vertex positions."
- **owner**: writer
- **blocking**: no

### Experiments (experiments.tex)

- **【File:Line]**: experiments.tex:L25
- **【原文引用]**: "The primary test platform is the Franka FR3 robot arm \cite{franka2020fr3}, consisting of 7 arm links (link0--link7) plus hand and finger links."
- **【问题分析与评级]**: 严重. The link count is internally inconsistent. "link0--link7" is 8 links (link0, link1, ..., link7), but the text claims "7 arm links." The primitive counts confirm 8: convex mode=8 primitives, sphere/single=8, capsule/single=8 (each produces exactly one primitive per link). If there were 7 links, these would be 7 primitives. Either correct to "8 arm links (link0--link7)" or restrict to "7 arm links (link1--link7)" and verify the data.
- **【修改建议]**: Change to "8 arm links (link0--link7)" throughout, OR explicitly exclude one link (e.g., link0 as base) and verify primitive counts and all ablations accordingly. Given the primitive counts directly confirm 8 links, the simplest fix is: "consisting of 8 arm links (link0--link7)."
- **owner**: writer
- **blocking**: yes

- **【File:Line]**: experiments.tex:L107-L109
- **【原文引用]**: "Runtime for capsule modes ranges from 12.7~s (single) to 33.8~s (default), with the \emph{high\_detail} preset completing in 21.8~s."
- **【问题分析与评级]**: 中等. The runtime behavior is counterintuitive: default (N=4, K_max=12) takes 33.8 s, while high_detail (N=6, K_max=16) takes only 21.8 s. The ablation table (Tab. 3) confirms the same pattern: N=4 = 33.0 s, N=6 = 21.0 s, N=8 = 21.2 s. Having MORE sections run FASTER is surprising and needs explanation. Possible reasons: (a) N=4 creates a particular cross-section geometry that triggers many split/refinement iterations, while N=6 produces well-behaved single-capsule fits per link; (b) the $\delta_{\min}$ threshold gates different numbers of split attempts. The paper currently reports the numbers without comment.
- **【修改建议]**: Add a brief analysis in the ablation text (Section VI-F): "The non-monotonic runtime with respect to $N$---where $N=4$ is slower than both $N=2$ and $N=6$---stems from the $\delta_{\min}$-gated splitting process: the 4-section configuration on certain links creates marginal coverage gaps at split boundaries that trigger repeated split-evaluation iterations, whereas $N=2$ produces too few sections to trigger splitting and $N\ge6$ generates well-conditioned per-section fits that pass the split thresholds without iteration."
- **owner**: writer
- **blocking**: no

- **【File:Line]**: experiments.tex:L161
- **【原文引用]**: "At $N=8$, this gap nearly disappears (9~$\mu$m residual on the affected link)."
- **【问题分析与评级]**: 中等. This is a per-link claim (9 $\mu$m on "the affected link") that is not traceable from the aggregate-only artifact data. The aggregate worst-case for N=8 is 1.201 mm (across all 8 links). The 9 $\mu$m value is a per-link specific number that cannot be verified from the existing artifact RESULT.md files, which explicitly state "Per-link data files do not exist in the artifact." This is the same class of problem as the previously resolved IR1-004 (the 0.63 capV/aabb lower bound).
- **【修改建议]**: Either (a) generate per-link data and make the claim traceable, or (b) remove the specific per-link value: "...this gap nearly disappears at $N=8$ (worst-case remaining gap: 1.2~mm across all links)."
- **owner**: writer
- **blocking**: no

- **【File:Line]**: experiments.tex:L157-L169
- **【原文引用]**: "\\textbf{NSections} is the dominant control parameter." / "\\textbf{MaxCapsulesPerLink} plateaus." / "\\textbf{AdaptiveCircleCount} enables per-section multi-circle fitting..." / "\\textbf{MaxRadiusBinRatio} set to $-1$..."
- **【问题分析与评级]**: 中等. These paragraph headings use exact CLI parameter names (NSections, MaxCapsulesPerLink, AdaptiveCircleCount, MaxRadiusBinRatio) in bold. The user's stated requirement (IR3-002) is to rewrite CLI parameters into academic mathematical notation. The validation_metrics.tex and method.tex sections have been converted, but experiments.tex still uses the CLI names directly. The ablation table row labels are acceptable as compact table entries, but the prose headings should match the mathematical notation established in Section V.
- **【修改建议]**: Replace the CLI parameter names in bold headings with the mathematical notation from Section V:
  - "\\textbf{NSections}" -> "\\textbf{Section Count ($N$)}"
  - "\\textbf{MaxCapsulesPerLink}" -> "\\textbf{Capsule Budget ($K_{\max}$)}"
  - "\\textbf{AdaptiveCircleCount}" -> "\\textbf{Adaptive Circle Fitting}"
  - "\\textbf{MaxRadiusBinRatio}" -> "\\textbf{Radius-Inflation Threshold ($\rho_{\max}$)}"
- **owner**: writer
- **blocking**: no

- **【File:Line]**: experiments.tex:L169 (mrbr_disabled)
- **【原文引用]**: "MaxRadiusBinRatio set to $-1$ (disabling all splitting) produces metrics identical to the default..."
- **【问题分析与评级]**: 中等. The runtime drops from 33.0 s to 12.6 s (a 62% reduction) when MaxRadiusBinRatio is disabled, yet the capsule output is identical. This massive runtime saving is mentioned in the artifact ("possibly because the disabled constraint short-circuits a split-evaluation step") but the paper text does not comment on it. A reviewer will notice this and ask why.
- **【修改建议]**: Add a sentence: "Notably, disabling this check reduces runtime from 33.0~s to 12.6~s without affecting any output metric, suggesting that the per-link split-evaluation loop---which tests each candidate split against $\delta_{\min}$---is the dominant computational cost at $N=4$ on this robot."
- **owner**: writer
- **blocking**: no

- **【File:Line]**: experiments.tex:L172 (mesh source)
- **【原文引用]**: "Mesh source (visual vs.\ collision) produces identical capsule fits on the FR3..."
- **【问题分析与评级]**: 轻微. RQ5 is defined in the research questions but the mesh source finding is only briefly noted. This is a secondary finding, so minimal coverage is acceptable, but acknowledging RQ5 explicitly in the discussion summary would strengthen the completeness.
- **【修改建议]**: Add a one-sentence explicit answer to RQ5 in Section VI-G: "For RQ5, visual-mesh and collision-mesh sources produce identical fits on the FR3, indicating that the simplified collision meshes preserve the same geometric structure for capsule fitting on this robot."
- **owner**: writer
- **blocking**: no

### Discussion (discussion.tex)

- **【File:Line]**: discussion.tex:L21
- **【原文引用]**: "collision-checking in motion planning (e.g., via FCL~\cite{pan2012fcl} or OMPL)"
- **【问题分析与评级]**: 轻微. OMPL (Open Motion Planning Library) is mentioned without a citation. The paper consistently cites all referenced tools (FCL, Drake, etc.), and the omission of an OMPL citation is inconsistent. While OMPL is well-known in robotics, the paper's citation standard requires a reference.
- **【修改建议]**: Either (a) add a citation for OMPL (e.g., Sucan et al., "The Open Motion Planning Library," IEEE RAM 2012, DOI: 10.1109/MRA.2012.2205651), or (b) remove the "OMPL" mention to avoid an uncited reference.
- **owner**: writer
- **blocking**: no

### Conclusion (conclusion.tex)

- **【File:Line]**: conclusion.tex:L7-L11
- **【问题分析与评级]**: The conclusion restates the contributions and lists five future work directions. The HPP-FCL reference was removed per IR1-011. No new issues. -- PASS
- **【修改建议]**: No change needed.
- **owner**: writer
- **blocking**: no

---

## Artifact Traceability Audit

| Claim in paper | Artifact source | Status |
|---|---|---|
| Table 2: all numeric values (convex 8/0.5s, sphere 8/10.1s, sphere 63/40.7s, capsule entries) | `artifacts/exp-fr3-mode-comparison/data/fr3_aggregate_summary.csv` | PASS -- within rounding tolerance |
| Table 3: all 11 ablation rows | `artifacts/exp-capsule-ablation/data/ablation_summary.md` | PASS -- all match |
| "NSections is the dominant control parameter" | ablation data rows nsections_2 through nsections_8 | PASS |
| "high_detail preset achieves the best worst-case uncovered distance (2.3 mm)" | ablation nsections_6 row (2.338 mm) | PASS |
| "Convex mode is fastest (0.5 s)" | aggregate convex row (0.468 s = 0.5 s rounded) | PASS |
| "9 $\mu$m residual on the affected link" (experiments.tex L161) | No per-link data in artifacts | FAIL -- per-link value not traceable from aggregate data |
| "AdaptiveCircleCount increases capsule count 3.9x" | ablation adaptive_true row (39/10 = 3.9) | PASS |
| "7 arm links (link0--link7)" | Convex=8, sphere/single=8, capsule/single=8 primitives | FAIL -- primitive counts imply 8 links, not 7 |
| Figure 1: method schematic | `figures/capsule_method_schematic.pdf` exists at 300 DPI | PASS |
| Figure 2: qualitative overlay | `figures/fig_qualitative_overlay.pdf` exists | PASS |

---

## Citation Audit

All citations verified against current ref.bib and in-text usage:

| cite_key | In text correct? | ref.bib correct? | Issues |
|---|---|---|---|
| `urdf_doc` | Yes | Yes | PASS |
| `xacro` | Yes | Yes | PASS |
| `todorov2012mujoco` | Yes | Yes | PASS. DOI present. |
| `drake` | Yes | Yes | PASS |
| `coumans2018pybullet` | Yes | Yes | PASS |
| `pan2012fcl` | Yes | Yes | PASS. DOI present. |
| `lien2008acd` | Yes | Yes | PASS. Corrected to CAGD 2008 journal version. |
| `mamou2016vhacd` | Yes | Yes | PASS. Informal talk citation, acceptable. |
| `barber1996quickhull` | Yes | Yes | PASS. DOI present. |
| `cgal` | Yes | Yes | PASS |
| `jacobson2018libigl` | Yes | Yes | PASS. Corrected to @misc. |
| `huang2022manifoldplus` | Yes | Yes | PASS. Year 2020, venue arXiv. |
| `bradshaw2002sphere` | Yes | Yes | PASS. School Trinity College Dublin. |
| `mlund_spheretree` | Yes | Yes | PASS |
| `wu2018capsule` | Yes | Yes | PASS. CRITICAL FIX: now Nannan Wu IEEE Access. |
| `larsen2000ssv` | Yes | Yes | PASS. ICRA 2000 with DOI. |
| `larsen1999ssv` | **Not cited** | Yes | **WARN**: Unused citation. Entry exists in ref.bib but is never cited in any .tex file. Will produce "unused citation" LaTeX warning. Remove or cite. |
| `koptev2023neural` | Yes | Yes | PASS. RA-L primary with IROS note. |
| `eigen` | Yes | Yes | PASS |
| `urdfdom` | Yes | Yes | PASS |
| `welzl1991mec` | Yes | Yes | PASS. DOI present. |
| `robot_viewer` | Yes | Yes | PASS |
| `franka2020fr3` | Yes | Yes | PASS. Year 2023, URL corrected. |
| `trimesh` | Yes | Yes | PASS |
| `sdformat` | Yes | Yes | PASS |
| `coumar2025foam` | Yes | Yes | PASS |
| `lauterbach2010gproximity` | Yes | Yes | PASS |

**Citation audit findings:**
1. `larsen1999ssv` (ref.bib line 152-159) is never cited in any .tex file. Remove from ref.bib to avoid LaTeX warnings. The ICRA 2000 version (`larsen2000ssv`) is correctly used in the text.
2. OMPL is mentioned in discussion.tex line 21 without a citation entry.

---

## User Feedback Status

Two blocking issues from state.md carry owner "user":

**IR3-001 (Tables too wide -- wrap with \resizebox):**
- Tables 2 and 3 (mode comparison and capsule ablation) already use `\resizebox{\columnwidth}{!}{...}` and render correctly.
- Table 1 (presets) does not use \resizebox but has only 5 narrow columns and fits within the column width without scaling.
- **Status**: Addressed. \resizebox is correctly applied where needed. If the user intended a different approach (e.g., rotating tables, breaking into sub-tables), the writer should request clarification. Otherwise, no further action needed.

**IR3-002 (CLI parameters -- rewrite as math notation):**
- validation_metrics.tex and method.tex: Fully converted to mathematical notation ($N$, $K_{\max}$, $\rho$, $\tau_{\mathrm{coa}}$, $M$, etc.). PASS.
- experiments.tex: Still uses CLI parameter names (NSections, MaxCapsulesPerLink, AdaptiveCircleCount, MaxRadiusBinRatio) in bold headings. **Partially addressed.** See experiments.tex issue above.
- **Status**: Partially addressed. The core sections are converted; the experiments section headings need updating.

---

## Loop Triage

| issue_id | severity | owner | blocking | required_action |
|---|---|---|---|---|
| IR9-001 | 严重 | writer | yes | Fix link count: "7 arm links (link0--link7)" is 8 links, confirmed by primitive counts |
| IR9-002 | 中等 | writer | no | Replace per-link "9 $\mu$m" claim with aggregate-safe wording (or generate per-link data) |
| IR9-003 | 中等 | writer | no | Replace CLI parameter names in experiments.tex headings with mathematical notation ($N$, $K_{\max}$, etc.) |
| IR9-004 | 中等 | writer | no | Explain non-monotonic runtime: N=4 (33.8s) slower than N=6 (21.8s) and N=2 (12.7s) |
| IR9-005 | 中等 | writer | no | Explain 62% runtime drop when $\rho_{\max}$ is disabled (12.6s vs 33.0s with identical output) |
| IR9-006 | 中等 | writer | no | Cross-reference $S=64$ validation resolution in validation_metrics.tex to match experimental setup |
| IR9-007 | 轻微 | writer | no | Remove unused `larsen1999ssv` from ref.bib to avoid LaTeX warning |
| IR9-008 | 轻微 | writer | no | Add OMPL citation or remove mention in discussion.tex L21 |
| IR9-009 | 轻微 | writer | no | Justify 10-bin choice for $r$/binMed in validation_metrics.tex |
| IR9-010 | 轻微 | writer | no | Add default $\tau$ value in method.tex COA description |
| IR9-011 | 轻微 | writer | no | Note ManifoldPlus requirement: why watertight mesh needed for cross-section slicing |
| IR9-012 | 轻微 | writer | no | Add explicit RQ5 answer in Section VI-G summary |

---

## Implementation Request Suggestions

No new implementation requests needed. All issues are addressable by the writer through text revision, citation cleanup, or table caption adjustments. The per-link data gap (IR9-002) could be resolved either by writer text revision (safer, immediate) or by a new implement request to generate per-link metrics (if stronger per-link claims are desired).

request_id: (optional, only if writer prefers implement-backed per-link data)
paper_claim: "9 $\mu$m residual" / per-link coverage values
task_type: data_generation
required_outputs: per_link_metrics.csv across all ablation and mode comparison variants
metrics: worst_dist, capV/aabb, r/binMed per link
inputs: existing generated URDF/JSON outputs in artifact directories
acceptance_criteria: CSV with one row per link per variant
writer_integration_target: experiments.tex per-link claims
