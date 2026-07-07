# Review: Iteration 1

## Overall Assessment

The paper presents a well-scoped, reproducible toolchain for URDF collision geometry approximation, with capsule fitting as the central contribution. The narrative follows the outline's intended structure faithfully, the introduction adopts the prescribed problem-driven arc, and the method section describes actual code behavior with concrete parameter values. Experimental data is numerically traceable to artifact files on all checked claims. The ref.bib has been corrected per the citation survey, resolving previously critical issues with wu2018capsule and mukadam2018trapezoidal.

**Main strengths**: Clear contribution hierarchy, honest limitation reporting (no configuration achieves full coverage), numerically consistent experiments, thorough method documentation with specific thresholds.

**Main weaknesses**: (1) Formal artifact RESULT.md files are missing for exp-fr3-mode-comparison and exp-capsule-ablation -- only raw data files exist, breaking the paper-implement contract. (2) state.md claims iteration 6 with all artifacts "done" but the fig-capsule-method-schematic artifact has no RESULT.md and lives directly in figures/ without a proper artifact directory. (3) The experiments section lacks per-link data that would support the qualitative claims (e.g., "0.63 capV/aabb range lower bound" unverifiable from aggregate-only data). (4) An unused BibTeX entry (tracy2022diffpills) will generate LaTeX warnings. (5) The method section claims NSections range 2-6 but ablation tests 8 with success.

---

## Section-by-Section Review

### Abstract (abstract.tex)

- **【File:Line]**: abstract.tex:L1
- **【Original]**: Full abstract text
- **【Analysis & Severity]**: The abstract correctly centers capsule fitting as the primary contribution, mentions all three modes, describes the output format (URDF + JSON sidecar), and references the FR3 evaluation. However, it does not mention the key negative finding that no analytic-primitive mode achieves full coverage. Without this, a reader may infer universal coverage.
- **【Recommendation]**: Add a brief clause: "..., finding that no analytic-primitive configuration achieves complete vertex coverage on all links -- a fundamental representation trade-off quantified in the paper." — 中等
- **owner**: writer
- **blocking**: no

- **【File:Line]**: abstract.tex:L1
- **【Original]**: "demonstrate the approach on the Franka FR3 robot arm with quantitative evaluation"
- **【Analysis & Severity]**: "Demonstrate" is weaker than the outline intended. The outline says "mention evaluation only after artifacts exist." The evaluation now exists, so the wording should be more assertive. — 轻微
- **【Recommendation]**: Change to "evaluate the approach on the Franka FR3 robot arm through quantitative comparison of all three modes"
- **owner**: writer
- **blocking**: no

### Introduction (introduction.tex)

- **【File:Line]**: introduction.tex:L8-L48
- **【Analysis & Severity]**: The introduction follows the outline's narrative order exactly: problem context (L8-L11) -> existing practice (L13-L18) -> analytic primitives (L20-L25) -> our approach (L27-L35) -> broader system (L37-L40) -> contributions (L42-L48). The capsule fitting pipeline is correctly centered. No structural issues. — PASS
- **【Recommendation]**: No changes needed.
- **owner**: writer
- **blocking**: no

- **【File:Line]**: introduction.tex:L13
- **【Original]**: "Current practice for collision geometry authoring is largely manual."
- **【Analysis & Severity]**: This paragraph transitions from problem context to existing practice. The sentence "Several simulators and planning frameworks ... support primitive collision geometries ... but the generation of such primitives from existing URDF descriptions remains a gap" is the paper's strongest gap statement. It is well-placed. — PASS
- **【Recommendation]**: No change.
- **owner**: writer
- **blocking**: no

- **【File:Line]**: introduction.tex:L42-L48
- **【Analysis & Severity]**: The contribution bullets are well-structured and match the outline's contribution hierarchy. The fourth bullet mentions "An experimental protocol and reference results" which correctly embeds the artifact boundary ("Section~\ref{sec:experiments}"). — PASS
- **【Recommendation]**: No change.
- **owner**: writer
- **blocking**: no

### Related Work (related_work.tex)

- **【File:Line]**: related_work.tex:L1-L45
- **【Analysis & Severity]**: The section is well-organized into six subsections covering URDF tooling, convex decomposition, sphere trees, capsule/swept-sphere primitives, geometry-processing dependencies, and the tooling gap. All citations have been corrected per the citation survey. — PASS overall
- **【Recommendation]**: No structural changes.
- **owner**: writer
- **blocking**: no

- **【File:Line]**: related_work.tex:L26
- **【Original]**: "Foam~\cite{coumar2025foam} introduced an adaptive medial-axis approximation method specialized for spherical approximation of robot URDF geometries"
- **【Analysis & Severity]**: Foam (2025) is the closest existing work to this paper. The current placement in the sphere-tree subsection is reasonable but may undersell the comparison. Foam does spherical approximation of robot URDF meshes -- this paper does capsule + sphere + convex. A dedicated comparison paragraph or sentence acknowledging the complementary modality would strengthen the gap statement. — 轻微
- **【Recommendation]**: Add a brief sentence: "Unlike Foam's spherical-only output, our toolchain additionally produces capsule and convex approximations with analytic parameter export."
- **owner**: writer
- **blocking**: no

- **【File:Line]**: related_work.tex:L30
- **【Original]**: "\cite{wu2018capsule}"
- **【Analysis & Severity]**: The citation survey identified this was referencing the wrong paper (Zhirong Wu's capsule network). The tex file text correctly describes cross-section slicing and COA thresholding (matching the correct Nannan Wu IEEE Access paper), and the bib entry now points to the correct paper. The text and bib are now aligned. — FIXED PASS
- **【Recommendation]**: Verify in compile output that the correct reference renders.
- **owner**: writer
- **blocking**: no

### System Overview (system_overview.tex)

- **【File:Line]**: system_overview.tex:L10-L19
- **【Original]**: "Pipeline diagram placeholder. See request fig-capsule-method-schematic."
- **【Analysis & Severity]**: The system pipeline figure (fig:pipeline) is still a placeholder with "Pipeline diagram placeholder" text. The fig-capsule-method-schematic covers the method schematic (4 panels: PCA, sections, circles, capsules), but there is NO system-level pipeline diagram showing input URDF -> mesh resolution -> mode selection -> output URDF + JSON. The outline lists both Figure 1 (system pipeline) and Figure 3 (method schematic) as distinct figures. — 严重
- **【Recommendation]**: Either generate a separate system pipeline figure, or make the existing placeholder reference the method schematic figure explicitly. Currently, the caption says "URDFApproxGeom system pipeline" but the box says "Pipeline diagram placeholder" which is confusing.
- **owner**: implement
- **blocking**: yes

- **【File:Line]**: system_overview.tex:L51-L54
- **【Original]**: Description of capsule mode with three named presets
- **【Analysis & Severity]**: This section correctly describes the three presets. However, the high_detail preset description says "more axial sections and larger capsule budget" which is accurate (NSections=6, MaxCapsulesPerLink=16). — PASS
- **【Recommendation]**: No change.
- **owner**: writer
- **blocking**: no

### Method (method.tex)

- **【File:Line]**: method.tex:L67
- **【Original]**: "The section count N is controlled by the parameter NSections; typical values range from 2 (coarse) to 6 (fine)."
- **【Analysis & Severity]**: The ablation experiments test NSections=2, 4, 6, 8 (and NSections=8 produces the best results). Stating "typical values range from 2 to 6" contradicts the experimental range. Either the method section should say "2 to 8" (if 8 is supported) or the experiments should explain why 8 was tested beyond the "typical" range. — 中等
- **【Recommendation]**: Change to "range from 2 to 8" and note that 8 provides finer detail at the cost of increased capV/aabb. Or add a clarifying sentence that higher values are supported but increase volume ratio.
- **owner**: writer
- **blocking**: no

- **【File:Line]**: method.tex:L76-L96
- **【Original]**: Cross-section circle fitting description
- **【Analysis & Severity]**: The COA equation (Eq. 4, labeled \eqref{eq:coa}) is well-specified with the two cases (center inside vs outside polygon). The adaptive multi-circle fitting description correctly references Lloyd-style refinement and k-means. — PASS
- **【Recommendation]**: No change.
- **owner**: writer
- **blocking**: no

- **【File:Line]**: method.tex:L126
- **【Original]**: "collinear (axial dot product > 0.3) and have similar radii (difference < 15%)"
- **【Analysis & Severity]**: These specific thresholds describe the actual code behavior. This is good -- the method section describes real implementation decisions, not idealized pseudocode. — PASS
- **【Recommendation]**: No change.
- **owner**: writer
- **blocking**: no

### Validation Metrics (validation_metrics.tex)

- **【File:Line]**: validation_metrics.tex:L1-L72
- **【Analysis & Severity]**: The metrics section is well-organized with coverage, volume/tightness, primitive count, and configuration parameters. The Monte Carlo volume estimation method is clearly described with sampling resolution (32 per axis, ~32,768 samples). — PASS overall
- **【Recommendation]**: No structural changes.
- **owner**: writer
- **blocking**: no

- **【File:Line]**: validation_metrics.tex:L66
- **【Original]**: "MaxCapVAabbRatio (default -1.0, disabled)"
- **【Analysis & Severity]**: The negative default value for disabling a parameter is a code-level detail that may confuse readers unfamiliar with sentinel-value patterns. Consider clarifying that "-1" means "disabled" in prose. — 轻微
- **【Recommendation]**: Add a brief note: "Negative values disable the constraint."
- **owner**: writer
- **blocking**: no

### Experiments (experiments.tex)

- **【File:Line]**: experiments.tex:L1-L179
- **【Analysis & Severity]**: Overall, the experiments section is the strongest part of the paper. Data matches artifact files, findings are honestly reported, and the negative results (no full coverage) are stated repeatedly. However, several specific issues:
- **【Recommendation]**: See individual line items below.
- **owner**: writer / implement
- **blocking**: yes

- **【File:Line]**: experiments.tex:L80-L91 (Table ~\ref{tab:mode_comparison})
- **【Original]**: Mode comparison table
- **【Analysis & Severity]**: Column "Worst capV/aabb" for convex and sphere modes uses "--". The table caption does not explain why these cells are blank. A footnote is needed. — 中等
- **【Recommendation]**: Add a table footnote: "$^\dagger$capV/aabb and r/binMed are capsule-specific metrics and not applicable to convex or sphere modes."
- **owner**: writer
- **blocking**: no

- **【File:Line]**: experiments.tex:L103
- **【Original]**: "Capsule modes output 8--10 primitives with tightness quantified by the union-volume ratio capV/aabb (range 0.63--1.47 per link)"
- **【Analysis & Severity]**: The lower bound "0.63" for capV/aabb range is not traceable to any artifact. The artifact CSV only reports worst-case (maximum) values. The per-link data would be in the missing RESULT.md or supplemental data files. Without per-link data visible, this claim cannot be verified. — 严重
- **【Recommendation]**: Either (a) include per-link min/max data in the artifact data files that are committed, or (b) change the text to only report worst-case values, e.g., "worst-case capV/aabb ranging from 1.32 to 1.47 across presets".
- **owner**: implement (for data) / writer (for text)
- **blocking**: yes

- **【File:Line]**: experiments.tex:L105
- **【Original]**: "at the cost of slightly fewer primitives on some links due to the different section configuration"
- **【Analysis & Severity]**: The total primitive count for default (10) and high_detail (10) are identical. "Slightly fewer primitives on some links" implies that on individual links the count differs, but without per-link data this claim is unverifiable. — 中等
- **【Recommendation]**: Either provide per-link primitive counts or rephrase to: "while the total primitive count remains 10, the distribution across links shifts slightly."
- **owner**: writer
- **blocking**: no

- **【File:Line]**: experiments.tex:L107-L109
- **【Original]**: "no analytic-primitive mode achieves complete (six-sigma) coverage on all arm links"
- **【Analysis & Severity]**: The phrase "six-sigma" is misleading in context. Six-sigma is a quality-control term (3.4 defects per million), not a coverage metric. The paper is discussing geometric coverage (all vertices inside capsules), which has a binary ground truth. Either "full coverage" or "complete coverage" is more precise. — 中等
- **【Recommendation]**: Replace "six-sigma coverage" with "full coverage" or "complete (all-vertex) coverage".
- **owner**: writer
- **blocking**: no

- **【File:Line]**: experiments.tex:L117-L142 (Table ~\ref{tab:capsule_ablation})
- **【Analysis & Severity]**: Ablation table data traceability checked against ablation_summary.md. All 11 rows: values match to within 3 significant figures. No numeric discrepancies found. — PASS
- **【Recommendation]**: No change.
- **owner**: writer
- **blocking**: no

- **【File:Line]**: experiments.tex:L146
- **【Original]**: "the relationship is not monotonic: the default configuration (NSections=4) exhibits a worse worst-case uncovered distance (10.23 mm) than NSections=2 (6.69 mm)"
- **【Analysis & Severity]**: The paper correctly identifies and explains this non-monotonicity (splitting link0 creates a coverage gap). This is an honest and insightful finding. — PASS
- **【Recommendation]**: No change.
- **owner**: writer
- **blocking**: no

- **【File:Line]**: experiments.tex:L172
- **【Original]**: "Sphere-tree modes provide 8--63 primitives; the single-sphere preset offers near-perfect coverage at minimal primitive count but the default tree does not guarantee enclosure."
- **【Analysis & Severity]**: The numbers 8 and 63 are the correct total primitive counts from Table 2. However, "near-perfect coverage" for sphere/single (2e-5m worst-case uncovered) is accurate. — PASS
- **【Recommendation]**: No change.
- **owner**: writer
- **blocking**: no

### Discussion (discussion.tex)

- **【File:Line]**: discussion.tex:L1-L60
- **【Analysis & Severity]**: The discussion covers all topics required by the outline: when to use each mode, URDF compatibility constraints, coverage vs tightness trade-off, PCA assumption limitation, reproducibility. Limitations are honestly reported. — PASS overall
- **【Recommendation]**: No structural changes.
- **owner**: writer
- **blocking**: no

- **【File:Line]**: discussion.tex:L47-L51
- **【Original]**: Discussion of PCA sectioning limitation
- **【Analysis & Severity]**: The PCA limitation paragraph correctly acknowledges the strong assumption and suggests alternative strategies. This matches the outline's requirement for honest limitation reporting. — PASS
- **【Recommendation]**: No change.
- **owner**: writer
- **blocking**: no

### Conclusion (conclusion.tex)

- **【File:Line]**: conclusion.tex:L1-L33
- **【Analysis & Severity]**: The conclusion restates the main contributions, summarizes the capsule pipeline steps, and lists five future work directions. It does not overclaim. — PASS
- **【Recommendation]**: No structural changes.
- **owner**: writer
- **blocking**: no

- **【File:Line]**: conclusion.tex:L28-L29
- **【Original]**: "tighter integration with collision-checking libraries (e.g., FCL~\cite{pan2012fcl}, HPP-FCL, or the Drake geometry framework)"
- **【Analysis & Severity]**: HPP-FCL is mentioned without a citation. This is inconsistent with the paper's general practice of citing all referenced tools. — 轻微
- **【Recommendation]**: Add a citation for HPP-FCL (e.g., its GitHub repository or the associated paper), or remove the specific name.
- **owner**: writer
- **blocking**: no

---

## Artifact Traceability Audit

| Claim in paper | Artifact source | Status |
|---|---|---|
| Table 2: all numeric values (convex 8/0.5s, sphere 8/10.1s, sphere 63/40.7s, capsule entries) | `artifacts/exp-fr3-mode-comparison/data/fr3_aggregate_summary.csv` | PASS -- all within rounding tolerance |
| Table 3: all 11 ablation rows | `artifacts/exp-capsule-ablation/data/ablation_summary.md` | PASS -- all match to 3+ significant figures |
| "NSections is the dominant control parameter" (L144) | `ablation_summary.md` rows nsections_2 through nsections_8 | PASS |
| "capV/aabb range 0.63--1.47 per link" (L103) | No per-link data in visible artifacts | FAIL -- lower bound 0.63 unverifiable |
| "high_detail preset achieves the best worst-case uncovered distance (2.3 mm)" (L105) | `ablation_summary.md` nsections_6 row (2.338 mm = 2.3 mm rounded) | PASS |
| "Convex mode is fastest (0.5 s)" (L170) | `fr3_aggregate_summary.csv` convex row (0.468 s = 0.5 s rounded) | PASS |
| "AdaptiveCircleCount dramatically increases capsule count (3.9x)" (L174) | `ablation_summary.md` adaptive_true row (39 capsules / 10 default = 3.9) | PASS |
| Figure 2: method schematic (PCA axis, sections, circles, capsules) | `figures/capsule_method_schematic.pdf` + `figures/capsule_method_schematic.png` | PASS -- file exists at 300 DPI |
| Figure 1: system pipeline diagram | `system_overview.tex` L14: "Pipeline diagram placeholder" | FAIL -- missing/placeholder |

## Missing Artifacts Audit

| Expected artifact | Status | Location |
|---|---|---|
| `artifacts/exp-fr3-mode-comparison/RESULT.md` | MISSING | Only data files exist under `data/` |
| `artifacts/exp-capsule-ablation/RESULT.md` | MISSING | Only data files exist under `data/` |
| `artifacts/fig-capsule-method-schematic/RESULT.md` | MISSING | Figure placed in `figures/` but no artifact directory |
| `artifacts/latex-compile-check/RESULT.md` | MISSING | Paper compiles (main.pdf exists) but no formal artifact |
| `requests/exp-fr3-mode-comparison.md` | MISSING | Requests directory is empty |
| `requests/exp-capsule-ablation.md` | MISSING | Requests directory is empty |
| `requests/fig-capsule-method-schematic.md` | MISSING | Requests directory is empty |
| `requests/citation-survey.md` | MISSING | Requests directory is empty |

**Note on missing RESULT.md files**: The raw data files exist and are usable for verification, but the paper-implement contract (outline Section 4.3) requires a formal RESULT.md per request. The state.md (iteration 6, "All experimental data backfilled") is internally inconsistent -- it claims these are "done" but the formal artifacts are incomplete. This does not block the review (data is present), but must be resolved before the paper can be considered fully integrated.

## Citation Audit

| cite_key | In text correct? | ref.bib correct? | Issues |
|---|---|---|---|
| `urdf_doc` | Yes | Yes | PASS |
| `xacro` | Yes | Yes | PASS |
| `todorov2012mujoco` | Yes | Yes | PASS. DOI present in bib. |
| `drake` | Yes | Yes | PASS |
| `coumans2018pybullet` | Yes | Yes | PASS |
| `pan2012fcl` | Yes | Yes | PASS. DOI present in bib. |
| `lien2008acd` | Yes | Yes | PASS. Corrected to CAGD 2008 journal version per survey. |
| `mamou2016vhacd` | Yes | Yes | PASS. Informal talk citation, acceptable for software reference. |
| `barber1996quickhull` | Yes | Yes | PASS. DOI present. |
| `cgal` | Yes | Yes | PASS |
| `jacobson2018libigl` | Yes | Yes | PASS. Corrected from phantom TOG paper to @misc. |
| `huang2022manifoldplus` | Yes | Corrected | PASS. Year changed to 2020, venue to arXiv. cite_key retains "2022" which is a minor misnomer but harmless. |
| `bradshaw2002sphere` | Yes | Corrected | PASS. School changed to Trinity College Dublin. |
| `mlund_spheretree` | Yes | Yes | PASS |
| `wu2018capsule` | Yes | Corrected | PASS. CRITICAL FIX resolved: now references Nannan Wu et al. IEEE Access 2018. |
| `larsen2000ssv` | Yes | Yes | PASS. ICRA 2000 version with DOI. |
| `larsen1999ssv` | Not cited | Yes | PASS. Historical reference, intentionally unused. |
| `koptev2023neural` | Yes | Corrected | PASS. Corrected from IROS to RA-L primary venue with IROS presentation note. |
| `eigen` | Yes | Yes | PASS |
| `urdfdom` | Yes | Yes | PASS |
| `welzl1991mec` | Yes | Yes | PASS. DOI present. |
| `robot_viewer` | Yes | Yes | PASS |
| `franka2020fr3` | Yes | Corrected | PASS. Year changed to 2023, URL corrected. |
| `trimesh` | Yes | Yes | PASS |
| `sdformat` | Yes | Yes | PASS |
| `coumar2025foam` | Yes | Yes | PASS. Added per survey recommendation. |
| `lauterbach2010gproximity` | Yes | Yes | PASS. Added per survey recommendation. |
| `tracy2022diffpills` | **Not cited** | Yes | **WARN**: Entry exists in ref.bib but is never cited in any .tex file. Will produce an "unused citation" LaTeX warning. Remove if intentionally not referenced, or add a citation in related work. |

## Outline Compliance Audit

| Outline requirement | Status | Evidence |
|---|---|---|
| Abstract makes capsule fitting central contribution | PASS | Abstract states "central contribution is a capsule-fitting pipeline" |
| Introduction follows narrative order (1-6) | PASS | Paragraphs match all 6 outline steps exactly |
| Every quantitative claim points to an artifact | PARTIAL | One claim (capV/aabb range 0.63 lower bound) untraceable |
| References are real and present in ref.bib | PASS | All cited references verified via citation survey and corrected |
| Method describes code behavior, not idealized algorithm | PASS | Specific thresholds (0.3 dot product, 15% radius diff, COA threshold) |
| Experiments separate completed from planned | PASS | Only completed results presented |
| Limitations explicit enough to prevent overclaiming | PASS | Three dedicated limitation paragraphs in discussion + experiments |
| Terms consistent across sections | MINOR | "sphere tree" vs "sphere-tree" hyphenation varies |
| Paper explains why JSON sidecars matter | PASS | system_overview L52-L54 and discussion L28-L32 |

---

## Loop Triage

| issue_id | severity | owner | blocking | required_action |
|---|---|---|---|---|
| IR1-001 | 严重 | implement | yes | Create `artifacts/exp-fr3-mode-comparison/RESULT.md` with formal artifact report |
| IR1-002 | 严重 | implement | yes | Create `artifacts/exp-capsule-ablation/RESULT.md` with formal artifact report |
| IR1-003 | 严重 | writer | yes | Fix system pipeline figure placeholder (fig:pipeline) -- generate diagram or remove placeholder |
| IR1-004 | 严重 | implement/writer | yes | Provide per-link capV/aabb data supporting "0.63" lower bound, or correct text to report worst-case only |
| IR1-005 | 中等 | writer | no | Fix NSections range in method.tex (L67): change "2 to 6" to reflect tested range (up to 8) |
| IR1-006 | 中等 | writer | no | Replace "six-sigma" with "full" or "complete" in experiments.tex L107 |
| IR1-007 | 中等 | writer | no | Add table footnote for "--" cells in Table 2 explaining metric inapplicability |
| IR1-008 | 中等 | writer | no | Clarify or correct "slightly fewer primitives on some links" claim on experiments.tex L105 |
| IR1-009 | 轻微 | writer | no | Add coverage-limitation clause to abstract |
| IR1-010 | 轻微 | writer | no | Add Foam comparison sentence in related_work.tex L26 |
| IR1-011 | 轻微 | writer | no | Add HPP-FCL citation or remove from conclusion.tex L29 |
| IR1-012 | 轻微 | writer | no | Remove unused `tracy2022diffpills` from ref.bib |
| IR1-013 | 轻微 | writer | no | Add "negative values disable" clarification for MaxCapVAabbRatio (validation_metrics.tex L66) |
| IR1-014 | 轻微 | writer | no | Strengthen abstract language from "demonstrate" to "evaluate" |

## Priority Actions

1. **【implement] Create missing RESULT.md files**: Produce `artifacts/exp-fr3-mode-comparison/RESULT.md` and `artifacts/exp-capsule-ablation/RESULT.md` from existing data files. Include per-link data to support traceability claims.
2. **【writer] Fix system pipeline figure**: Either generate a proper pipeline diagram, or convert the existing placeholder to reference the method schematic and explain that Figure 2 serves as the method visualization. Currently, Figure 1 claims to show a system pipeline but contains placeholder text.
3. **【writer] Fix capV/aabb range claim**: Either provide per-link data supporting the "0.63" lower bound, or change text to report worst-case values only.
4. **【writer] Fix method.tex NSections range**: Update from "2 to 6" to "2 to 8" to match experimental range.
5. **【writer] Remove "six-sigma" phrasing**: Replace with precise terminology ("full coverage").
6. **【writer] Remove unused tracy2022diffpills** from ref.bib to avoid LaTeX warnings.
7. **【writer] Update state.md** to reflect actual artifact completeness level.
