# Paper Outline: URDFApproxGeom / SpherizedURDFGenerator

本文档用于指导后续在 `doc/paper` 中撰写英文 arXiv 论文。它不是最终论文正文，而是 writer agent 的章节蓝图、证据边界和 implement/reviewer 交接清单。

## 1. Working Thesis

**Working title:**
URDFApproxGeom: Automatic Primitive Collision Approximation for Robot URDF Models

**Core thesis:**
Robot URDF models often rely on mesh collision geometry that is expensive, inconsistent across simulators, or manually simplified. This work presents an automatic toolchain that converts mesh-based URDF collision or visual geometry into lightweight convex, sphere-tree, and especially capsule-based analytic approximations, while preserving URDF compatibility and exporting sidecar primitive parameters for downstream analytic collision and distance workflows.

**Academic anchor:**
The paper should center on URDFApproxGeom as a practical, reproducible URDF-to-primitive approximation toolchain. Capsule fitting remains the most distinctive mode, but the manuscript must not present the cross-section capsule fitting idea as a new theory. The defensible contribution is the integration of capsule, sphere, and convex outputs into a URDF-compatible workflow with JSON sidecar parameters, validation metrics, and artifact-backed empirical characterization.

**Tone constraint:**
Position the work as a practical, reproducible geometry-processing tool for robotics. Do not claim state-of-the-art accuracy, speed, or generality without experimental artifacts.

## 2. Repository Facts To Preserve

These points are supported by the repository documentation or source and can be used as factual basis:

- The tool converts mesh-based URDF geometry into three approximation modes: `convex`, `sphere`, and `capsule`.
- The Python package name is `urdf-approx-geom`; the CLI entry point is `urdf-approx-geom`.
- The public Python API exposes `generate` and `generate_all`, and returns a `GenerateResult` containing output URDF, sidecar JSON path, config path, and primitive count.
- Convex mode emits convex mesh collision geometry.
- Sphere mode emits URDF sphere primitives and a JSON sidecar with canonical `spheres` entries.
- Capsule mode emits URDF-compatible cylinder-plus-two-sphere representations and a JSON sidecar with per-link `capsules`, each containing `p0`, `p1`, and `radius`.
- Capsule fitting uses mesh sections perpendicular to a PCA principal axis, fits covering circles per section, chains circles across adjacent sections into capsules, grows radii to cover vertices, removes redundant/nested capsules, and applies budget/tightness-driven refinement.
- The CLI and validation utilities include capsule tightness checks based on coverage, `capV/aabb`, and `r/binMed`, plus comparison gates between baseline and candidate sidecars.
- The repository contains FR3 robot resources, preset configs, Docker/source quickstarts, visualization support through robot_viewer bundles, and public Python/C++ tests.

## 3. Contribution Claims

Use the following claim hierarchy. Claims marked **needs artifact** must not be stated as experimental conclusions until implement returns traceable results in `doc/paper/artifacts/<request_id>/RESULT.md`.

1. **End-to-end URDF primitive approximation toolchain**
   Supported by code and docs. The paper can claim the system ingests URDF mesh geometry and emits drop-in collision URDFs for convex, sphere, and capsule modes.

2. **Capsule fitting method for analytic robot-link collision primitives**
   Supported by source. The paper can describe the algorithmic pipeline and why capsule primitives are useful for smooth low-count analytic geometry.

3. **Sidecar JSON contract for downstream analytic use**
   Supported by docs and API. The paper can claim outputs include reusable primitive parameters, not only simulator-specific URDF tags.

4. **Validation and comparison workflow for defensible tuning**
   Supported by CLI/source. The paper can claim the tool exposes metrics and pass/fail gates. Any statement that one preset is better than another **needs artifact**.

5. **Empirical performance, tightness, runtime, and robustness**
   **Needs artifact.** Do not write quantitative claims until experiments are run and recorded.

## 4. Prompt Interaction Contract

`outline.md` is the shared planning artifact for the paper loop. It should tell every prompt what the paper is trying to argue, which claims are allowed, which claims still need evidence, and where future work should be routed. It is not a substitute for `state.md`, implementation requests, experiment artifacts, reviewer reports, or LaTeX section files.

### 4.1 Authority and Precedence

Use this precedence when files disagree:

1. **User instructions** define current scope, priority, and acceptable trade-offs.
2. **`doc/paper/state.md`** defines operational loop status: current iteration, next action, open requests, artifacts, blocking issues.
3. **`doc/paper/outline.md`** defines paper intent: thesis, contribution hierarchy, section plan, evidence map, writing constraints.
4. **`doc/paper/requests/*.md`** define implement tasks. A request overrides the outline only for that specific task.
5. **`doc/paper/artifacts/*/RESULT.md`** defines empirical facts. Artifacts override planned expectations in the outline.
6. **`doc/paper/reviews/*.md`** define critique and triage. Reviews do not rewrite the outline directly; they trigger writer/loop updates.
7. **`main.tex`, `sections/*.tex`, `ref.bib`, and figures/tables** are the draft outputs and must remain consistent with all files above.

If an artifact contradicts this outline, the writer must weaken or rewrite the affected claim and then update this outline's evidence map. The outline should not be edited to hide a negative or inconclusive result.

### 4.2 Writer Prompt Interaction

Writer must read this outline before drafting or revising any paper section.

Writer may edit:
- thesis, title, contribution hierarchy, section plan, writing rules, evidence map.
- implementation request backlog when a paper claim needs evidence.
- integration notes after an artifact becomes available.

Writer must not:
- insert unverified numerical results into the outline as if they were established findings.
- remove an evidence requirement merely because it is inconvenient.
- rewrite implement artifacts or reviewer reports.
- let the LaTeX draft drift away from the current thesis and contribution hierarchy.

Writer update triggers:
- a new or weakened paper claim.
- a new experiment, figure, table, or citation need.
- an implement artifact changes what can be claimed.
- reviewer identifies a structural, evidence, citation, or overclaiming issue.
- user changes target scope, target venue/platform, or acceptable experiment budget.

### 4.3 Implement Prompt Interaction

Implement should treat this outline as context, not as an executable request. It should execute only explicit request files under `doc/paper/requests/`.

Implement may read:
- Section 3 contribution claims to understand what evidence is needed.
- Section 5 experiment plan and Section 6 implementation request backlog to understand intended metrics and outputs.
- Relevant LaTeX draft sections only when the request asks for claim checking or result consistency.

Implement must write:
- `doc/paper/artifacts/<request_id>/RESULT.md`.
- supporting data, logs, scripts, and figures inside `doc/paper/artifacts/<request_id>/` or `doc/paper/tools/` when appropriate.

Implement must not:
- edit `outline.md` during normal experiment execution.
- write paper prose in `sections/*.tex`.
- convert preliminary or failed results into positive claims.
- silently skip metrics requested by the outline or request file.

If the request is underspecified, implement should return `status: blocked` in `RESULT.md` and list missing fields rather than guessing the paper goal.

### 4.4 Reviewer Prompt Interaction

Reviewer should read this outline before reviewing the draft. The review should evaluate whether the draft follows the intended thesis, contribution hierarchy, evidence boundaries, and writing rules.

Reviewer uses this outline to check:
- whether the introduction makes automatic capsule primitive fitting the central contribution.
- whether each contribution has matching evidence or an explicit evidence request.
- whether experiments answer the research questions listed here.
- whether citations required by related work have been verified.
- whether limitations match known method assumptions and artifact constraints.

Reviewer must not edit this outline directly. Reviewer should instead write findings to `doc/paper/reviews/review_iter_<n>.md` with owner tags: `writer`, `implement`, or `user`. If an outline update is needed, the reviewer should say exactly which section should change and why.

### 4.5 Loop Prompt Interaction

Loop should use this outline as the paper's intent map and `state.md` as the operational state machine.

Loop should:
- initialize or update `state.md` without changing paper claims.
- compare open requests against the implementation backlog in this outline.
- route evidence gaps to implement.
- route prose, structure, and citation issues to writer.
- route unresolved scope decisions to user.
- trigger reviewer after writer integrates artifacts or makes material claim/structure changes.

Loop may update this outline only for coordination metadata when explicitly acting as the coordinator, and should keep such edits minimal. Substantive paper argument changes should be assigned to writer.

### 4.6 Lifecycle Rules

Use this lifecycle for every claim that needs evidence:

1. **Plan in outline:** writer adds or confirms the claim, target section, required metrics, and request ID.
2. **Create request:** writer or loop creates `doc/paper/requests/<request_id>.md` using the required template.
3. **Produce artifact:** implement creates `doc/paper/artifacts/<request_id>/RESULT.md` plus data/logs/figures.
4. **Integrate:** writer imports only artifact-supported facts into the LaTeX draft and cites the artifact path near tables/figures or in comments.
5. **Review:** reviewer checks claim-evidence alignment and writes triage.
6. **Update outline if needed:** writer updates this outline when the paper thesis, section plan, evidence map, or implementation backlog changes.

### 4.7 Evidence Map

| Paper area | Planned claim | Evidence state | Responsible prompt | Artifact or request |
|---|---|---|---|---|
| Introduction / contributions | The project is an integrated URDF-to-primitive approximation toolchain | code/docs supported | writer | repository facts |
| Method | Capsule fitting uses PCA sectioning, covering circles, chained capsules, coverage growth, and pruning | code supported | writer | repository facts |
| System overview | CLI/Python APIs generate convex, sphere, and capsule outputs with sidecar JSON where applicable | code/docs supported | writer | repository facts |
| Validation metrics | `validate` and `compare` expose coverage/tightness gates | code supported; metric interpretation must be precise | writer + reviewer | repository facts |
| Experiments | Mode trade-offs on FR3 | needs artifact | implement | `exp-fr3-mode-comparison` |
| Experiments | Capsule parameter ablations | needs artifact | implement | `exp-capsule-ablation` |
| Figures | Capsule method schematic | needs artifact | implement or writer, depending on whether generated from data | `fig-capsule-method-schematic` |
| Related work | Verified citation context | needs citation check | writer or implement | `citation-survey` |
| Final artifact | LaTeX compiles | needs artifact | implement | `latex-compile-check` |

### 4.8 Outline Maintenance Rules

After every material loop iteration, check whether this outline needs a small update. A material iteration is one of:
- a new `RESULT.md` is available.
- reviewer reports a blocking issue.
- a claim is added, weakened, deleted, or moved.
- the experiment plan changes.
- user changes scope.

Keep changes append-only where possible for traceability, but remove stale planned claims when they would mislead future agents. Do not duplicate `state.md`; this outline should explain the paper plan, not track every loop status transition.

## 5. Proposed Paper Structure

### Abstract

Goal: One compact paragraph plus optional contribution sentence.

Include:
- Problem: robot URDF collision geometry is often mesh-heavy or manually simplified.
- Solution: automatic conversion into convex, sphere-tree, and capsule primitives.
- Core method: capsule fitting by cross-section decomposition and coverage-evaluated refinement.
- Output value: URDF-compatible collision models plus JSON primitive parameters.
- Evidence placeholder: mention evaluation only after artifacts exist.

Avoid:
- Unsupported speedups, accuracy percentages, or broad claims such as "state of the art".

### 1. Introduction

Narrative order:

1. Robot simulation, planning, control, and collision checking need collision geometry that is lighter than visual meshes but faithful enough for contact and clearance reasoning.
2. URDF supports common primitives and meshes, yet practical robot descriptions often mix high-detail visual meshes, simplified collision meshes, and manually authored approximations.
3. Manual primitive fitting is tedious and hard to reproduce across links, robots, and simulators.
4. Analytic primitives are attractive because they support closed-form distance, stable collision queries, and compact robot-link geometry.
5. This work introduces URDFApproxGeom, an automatic toolchain for generating convex, sphere-tree, and capsule approximations from URDF meshes.
6. The paper focuses on capsule fitting as the main method: sectioning meshes along a principal axis, fitting covering circles, chaining them into capsules, and refining for coverage/tightness under a primitive budget.

Contribution bullets:
- A URDF-to-primitive approximation pipeline that preserves visual/inertial/joint structure while replacing collision geometry.
- A capsule fitting pipeline that produces coverage-evaluated, low-count analytic primitives and exports their closed-form parameters.
- A reproducible CLI/Python workflow with presets, validation metrics, comparison gates, and visualization bundles.
- An experimental protocol for evaluating coverage, tightness, primitive count, runtime, and preset sensitivity. Convert to a results contribution only after artifacts exist.

### 2. Related Work

This section needs verified references before final writing. Do not invent citations.

Search targets:
- URDF and ROS robot description tooling.
- Collision geometry simplification for robotics simulation and planning.
- Convex decomposition and convex hull collision approximations.
- Sphere trees, medial-axis sphere fitting, and sphere-based collision models.
- Capsule or swept-sphere primitives for robot collision checking and distance computation.
- Cross-section decomposition methods for capsule fitting; writer prompt mentions Wu2018 and repository comments cite Wu2018-style sections, COA, and circle clustering.
- Geometry-processing dependencies and foundations: CGAL, libigl, ManifoldPlus, urdfdom, pybind11 where relevant.

Suggested organization:
- Robot collision geometry in URDF ecosystems.
- Primitive approximation methods: convex, sphere, capsule.
- Tooling gap: existing methods provide algorithms or library pieces, but not an integrated URDF-to-URDF pipeline with sidecar primitive parameters and validation gates.

### 3. System Overview

Purpose: Explain what the software does before diving into capsule math.

Subsections:

#### 3.1 Inputs and Outputs
- Input: URDF with mesh geometry in collision or visual elements.
- Mesh source selection: sphere/capsule default to visual geometry; collision mesh can be selected.
- Output: modified collision URDF plus sidecar JSON for sphere/capsule; convex mode emits generated mesh files.
- Non-targets: no xacro/SDF generation; xacro should be preprocessed externally.

#### 3.2 Modes
- Convex: CGAL/libigl path, tight mesh-based convex approximation.
- Sphere: sphere-tree approximation with single/default presets.
- Capsule: analytic capped cylinders emitted as cylinder+sphere primitives for URDF compatibility.

#### 3.3 Python and CLI Interface
- CLI commands: `generate`, `presets`, `validate`, `compare`, `compare-all`, `visualize`.
- Python functions: `generate`, `generate_all`, mode presets, sidecar primitive count.
- Docker/source workflows can be mentioned for reproducibility.

#### 3.4 Visualization and Inspection
- robot_viewer bundle generation supports visual/collision comparison.
- Use this as qualitative evaluation support, not quantitative proof.

### 4. Capsule Approximation Method

This should be the technical core of the paper.

#### 4.1 Problem Formulation
- Given a robot link mesh in the link frame, find a set of capsules whose union covers the mesh samples while keeping primitive count and excess volume low.
- Capsule parameterization: segment endpoints `p0`, `p1` and radius `r`.
- Coverage objective: every selected mesh vertex/sample lies inside at least one capsule.
- Tightness objective: reduce excess occupied volume and radius inflation under a maximum capsule budget.

Possible notation:
- Mesh vertices `V`, faces `F`.
- Capsule set `C = {c_i}`.
- Capsule distance as point-to-segment distance minus radius.
- Coverage condition: `min_i d(x, c_i) <= 0` for all sampled surface points `x`.

#### 4.2 Mesh Preparation and Frame Handling
- Resolve mesh source from URDF visual or collision elements.
- Convert unsupported visual meshes such as `.dae` to `.obj` in the Python pre-pass when needed.
- Load mesh into the C++ geometry pipeline.
- Apply mesh origin transforms into the link frame.
- Use ManifoldPlus/watertight processing before fitting where applicable, while growing against original vertices when needed for coverage.

#### 4.3 Principal-Axis Sectioning
- Compute a PCA principal axis for the link mesh.
- Slice the mesh with planes perpendicular to that axis.
- Chain triangle-plane intersections into 2D cross-section contours.
- Section count is controlled by `NSections`.

#### 4.4 Cross-Section Circle Fitting
- Fit one or more covering circles to each cross-section contour.
- Use circle outside area / COA-style thresholding as the conceptual error measure.
- `AdaptiveCircleCount`, `MaxCirclesPerSection`, and `CoaThreshold` control section complexity.
- Emphasize deterministic, coverage-oriented fitting rather than purely visual matching.

#### 4.5 Capsule Construction Across Sections
- Convert fitted 2D circle centers back to 3D section planes.
- Match neighboring section circles by nearest center to create active chains.
- Emit capsule segments between matched circles with radius equal to the larger neighboring circle radius.
- Extend endpoint sections to the mesh axial extrema to avoid uncovered ends.

#### 4.6 Coverage-Preserving Refinement
- Merge collinear capsules where possible.
- Grow capsule radii so all relevant vertices are covered.
- Shrink endpoint spans while preserving coverage.
- Remove nested/degenerate redundant capsules.
- If tightness constraints apply, split inflated capsules based on assigned vertices and accept only improvements that reduce the targeted metric.
- Respect `MaxCapsulesPerLink` by pruning with coverage-evaluated candidate selection.

#### 4.7 URDF-Compatible Emission
- URDF has no native capsule element.
- Emit each capsule as one cylinder plus two sphere end caps in the collision tree.
- Store the canonical analytic capsule in JSON (`p0`, `p1`, `radius`) so downstream users are not forced to reconstruct it from URDF primitives.

### 5. Validation Metrics and Tuning

Purpose: Explain how users decide whether an approximation is acceptable.

Metrics:
- `all_covered`: whether fitted capsules cover the evaluated link samples.
- `worst_signed_distance`: maximum uncovered violation or margin representation, based on implementation semantics.
- `capV/aabb`: capsule union volume relative to link AABB volume.
- `r/binMed`: radius inflation profile along capsule bins.
- Primitive count per link and total primitives.
- `vol/dae` or approximation-to-reference volume ratio, only when generated by experiments.

Config levers:
- `NSections`: more axial slices for changing cross-sections.
- `MaxCapsulesPerLink`: primitive budget.
- `CoaThreshold`: cross-section fit tightness.
- `MaxRadiusBinRatio`: pressure to split inflated capsules.
- `MinSplitVolumeImprovement`: reject marginal splits.
- `MaxCapVAabbRatio`: fail-fast looseness ceiling.
- `AdaptiveCircleCount` and `MaxCirclesPerSection`: section complexity control.

This section should connect metrics to paper claims: coverage supports correctness, volume/tightness supports geometric quality, primitive count supports compactness, runtime supports practicality.

### 6. Experiments

Do not write final experimental conclusions until implement artifacts exist.

#### 6.1 Research Questions

RQ1: Does the generated geometry preserve coverage of robot link meshes?
RQ2: How do convex, sphere, and capsule modes trade off primitive count and geometric tightness?
RQ3: How sensitive is capsule quality to section count, capsule budget, adaptive circles, and splitting thresholds?
RQ4: What runtime cost does the full URDF approximation pipeline impose on representative robot models?
RQ5: How do visual-mesh and collision-mesh sources affect fitting quality?

#### 6.2 Datasets / Robot Models

Initial dataset:
- FR3 resources bundled in `resources/fr3`.

Preferred expansion:
- Add 2-4 additional open robot URDFs with varied morphology if licensing and setup are practical.
- If additional robots are unavailable, state the limitation clearly and frame FR3 as a case study rather than broad benchmark.

#### 6.3 Baselines

Internal baselines:
- Original mesh collision / visual mesh reference.
- Convex mode.
- Sphere `single`.
- Sphere `default`.
- Capsule `single`.
- Capsule `default`.
- Capsule `high_detail`.

Capsule ablations:
- fixed vs adaptive circle count.
- different `NSections`.
- different `MaxCapsulesPerLink`.
- splitting thresholds enabled/disabled.
- visual mesh source vs collision mesh source.

#### 6.4 Required Metrics

For each robot, link, mode, and preset:
- primitive count.
- coverage boolean and worst signed distance.
- approximation volume / reference mesh volume if computable.
- capsule union volume / AABB for capsule outputs.
- `r/binMed` for capsule outputs.
- runtime per mode and full pipeline.
- output validity: URDF parses, JSON schema matches expected fields.

#### 6.5 Tables and Figures

Table 1: Feature comparison of output modes.
Table 2: Quantitative FR3 mode comparison.
Table 3: Capsule ablation results.
Figure 1: System pipeline diagram from URDF input to approximated URDF/JSON.
Figure 2: Qualitative visual/collision overlay for one representative link.
Figure 3: Capsule fitting method schematic: PCA axis, cross-sections, circles, chained capsules.
Figure 4: Trade-off plot: primitive count vs tightness/volume ratio.
Figure 5: Optional runtime breakdown by mode.

### 7. Discussion

Topics:
- Why capsule primitives are useful for analytic distance/collision workflows.
- Why URDF compatibility requires cylinder+sphere emission even though JSON stores canonical capsules.
- Trade-off between tightness and primitive budget.
- Difference between fitting visual meshes and existing collision meshes.
- When users should prefer convex, sphere, or capsule mode.
- Practical reproducibility: presets, Docker image, CLI validation, robot_viewer bundles.

Limitations:
- Capsules may bridge concavities and over-cover shapes that sphere trees can approximate more tightly.
- PCA-axis sectioning is a strong assumption for highly branched or non-elongated geometry.
- URDF-native capsule support is absent; URDF output is an equivalent primitive decomposition, while JSON remains canonical.
- Quantitative generality depends on benchmark diversity.
- Some dependencies and mesh preprocessing choices may affect runtime and reproducibility.

### 8. Conclusion

Restate:
- The paper presents an integrated URDF primitive approximation toolchain.
- The main method automatically fits coverage-evaluated capsule primitives from robot link meshes.
- The outputs are immediately usable in URDF-compatible toolchains and reusable as analytic primitive JSON.
- Future work: broader robot benchmarks, more topology-aware capsule decomposition, direct support for more robot description formats, tighter integration with collision-checking libraries.

## 6. Implementation Requests To Create Later

Create these only when the writer reaches the corresponding section and needs evidence. Once a request file exists, keep the request ID stable and update the evidence map above.

### `exp-fr3-mode-comparison`

Task type: experiment/table/figure
Claim supported: mode trade-offs among convex, sphere, and capsule approximations on FR3.
Outputs needed:
- CSV/JSON table by link and mode.
- Aggregate table with primitive counts, coverage, volume/tightness metrics, runtime.
- Qualitative screenshots or render paths.

### `exp-capsule-ablation`

Task type: experiment/table
Claim supported: capsule parameters control tightness/compactness trade-offs.
Outputs needed:
- Results for varying `NSections`, `MaxCapsulesPerLink`, `AdaptiveCircleCount`, `MaxRadiusBinRatio`, and `MaxCapVAabbRatio`.
- Per-link and aggregate metrics.

### `fig-capsule-method-schematic`

Task type: figure
Claim supported: visual explanation of capsule fitting method.
Outputs needed:
- A reproducible schematic or generated figure showing mesh sectioning, fitted circles, and chained capsules.

### `citation-survey`

Task type: citation_check
Claim supported: related-work positioning.
Outputs needed:
- Verified BibTeX entries for URDF/ROS, convex geometry, sphere trees, capsule collision, cross-section decomposition, and geometry-processing dependencies.

### `latex-compile-check`

Task type: latex_compile
Claim supported: final paper artifact is buildable.
Outputs needed:
- Build command, log, generated PDF path, and unresolved warnings.

## 7. Writing Rules For The Draft

- Paper prose must be English; planning and handoff files may be Chinese.
- Keep "capsule primitive", "sphere tree", "collision approximation", "sidecar JSON", and "URDF-compatible emission" terminologically consistent.
- Do not write unsupported numerical claims.
- When using README sample numbers, either trace them to an artifact or mark them as repository-reported preliminary values until verified.
- Every table or figure containing measurements must cite the artifact path that produced it.
- Distinguish clearly between:
  - mesh-based convex output,
  - URDF sphere primitives,
  - canonical capsules stored in JSON,
  - URDF-compatible capsule decomposition into cylinders and spheres.
- Avoid presenting the project as a theoretical optimization breakthrough unless the experiments and derivation support that framing. The stronger defensible framing is an integrated, reproducible, robotics-oriented geometry approximation tool with a capsule fitting pipeline.
- When writer, implement, reviewer, or loop disagree, preserve the disagreement in the relevant artifact/review/state file and resolve the paper claim through writer revision rather than editing evidence retroactively.

## 8. Suggested `doc/paper` File Layout

```text
doc/paper/
  outline.md
  state.md
  main.tex
  ref.bib
  sections/
    abstract.tex
    introduction.tex
    related_work.tex
    system_overview.tex
    method.tex
    validation_metrics.tex
    experiments.tex
    discussion.tex
    conclusion.tex
  figures/
    README.md
  tables/
    README.md
  requests/
    <request_id>.md
  artifacts/
    <request_id>/
      RESULT.md
  reviews/
    review_iter_<n>.md
```

If the reference paper format from `/home/admin1/projects/CFDF/doc/paper` is available, mirror its LaTeX structure and style macros, but never reuse its content.

## 9. Reviewer Checklist

Before asking reviewer to review a draft, check:

- Does every quantitative claim point to an artifact?
- Are references real and present in `ref.bib`?
- Does the introduction make capsule fitting the central contribution?
- Does the method section describe code behavior rather than an idealized algorithm not implemented here?
- Does the experiment section separate completed results from planned experiments?
- Are limitations explicit enough to prevent overclaiming?
- Does the paper explain why JSON sidecars matter beyond URDF emission?
- Are terms consistent across sections?

## Gatekeeper Revision Additions (2026-07-06)

### New Evidence Map Rows

| Experiments / metrics | Surface-sampled coverage beyond vertex-only checks | needs artifact | implement | `surface-coverage-metrics` |
| Experiments / limitations | PCA-section capsule fitting works best for elongated links and degrades on branched or flat shapes | needs artifact | implement | `synthetic-morphology-benchmark` |
| Experiments / downstream utility | Sidecar analytic primitives support fast point-distance evaluation | needs artifact | implement | `distance-query-benchmark` |

### Related-Work Rule

The revised paper must avoid broad "no existing tool" language. Any novelty statement must be scoped to the combination of URDF-compatible generation, capsule sidecar export, validation gates, and the specific implementation released in this repository.
