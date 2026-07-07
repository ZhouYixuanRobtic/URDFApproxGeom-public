# Result: related-work-positioning-audit

status: done
source_request: doc/paper/requests/related-work-positioning-audit.md

## Claim Under Test

The related-work section needs verified prior-art positioning for five sources: Foam, Tesseract URDF extensions, MathWorks capsule approximation, Wu et al. 2018, and Larsen et al. 2000. Specifically, the paper must avoid the phrase "no existing tool" unless scoped precisely, and needs correct positioning of Tesseract's capsule tag support and MathWorks' capsuleApproximation.

## Commands

All results obtained via web search. No local commands needed.

## Artifacts

- `RESULT.md`: This file, the primary audit output.

## Results

### Citation Verification Table

| Source | Type | Verified URL/DOI | In ref.bib? | Status | Claim Relevance | Recommended Wording |
|--------|------|------------------|-------------|--------|-----------------|---------------------|
| Foam `coumar2025foam` | Academic (arXiv, submitted IROS 2025) | arXiv:2503.13704, https://arxiv.org/abs/2503.13704, GitHub: https://github.com/CoMMALab/foam (MIT) | Yes (correct entry) | verified | Spherical-only URDF approximation. Shares goal of automated primitive generation. Uses AMAA, outputs spheres only (no capsules, no convex meshes). | Current text is accurate: "Unlike Foam's spherical-only output, our toolchain additionally produces capsule and convex approximations." Can strengthen: "Foam generates sphere-only approximations via Adaptive Medial Axis Approximation; our tool extends this paradigm to capsules and convex hulls with analytic parameter export." |
| Tesseract URDF Package | Nonacademic tool documentation | https://tesseract-docs.readthedocs.io/en/latest/_source/core/packages/tesseract_urdf_doc.html | No | nonacademic_but_relevant | Tesseract's `tesseract_urdf` parser supports `<capsule>`, `<convex_mesh>`, `<cone>`, `<sdf_mesh>`, `<octomap>` tags not in standard URDF. This is a custom parser extension, not a generation pipeline. | "Tesseract's URDF parser extends the standard with capsule and convex_mesh geometry tags, but loading these shapes requires its custom parser. Our approach works within standard URDF primitives, maintaining compatibility with any URDF-compliant simulator." |
| MathWorks capsuleApproximation | Nonacademic proprietary tool | https://kr.mathworks.com/help/robotics/ref/capsuleapproximation.html (R2022b+) | No | nonacademic_but_relevant | Proprietary MATLAB function that fits capsules to rigidBodyTree. Algorithm is undocumented; no published paper. | Mention in limitations only: "Commercial tools such as MathWorks' capsuleApproximation (R2022b) provide capsule fitting for rigid body trees, but their closed-source fitting methodology is not publicly documented, algorithmically reproducible, or extensible." |
| Wu et al. 2018 `wu2018capsule` | Academic (IEEE Access) | DOI: 10.1109/ACCESS.2018.2837013 | Yes (correct entry) | verified | Cross-section-based capsule fitting for mannequin meshes. Uses circle-outside-area (COA) metric. Conceptual foundation for our method. | Current text is accurate. Can add quantitative precision: "Wu et al.'s cross-section-based method extracts 2D contours along a principal axis, fits circles via Lloyd clustering with an outside-area error metric, and chains circles into capsules. Our pipeline adapts this cross-sectioning approach to robot link meshes and adds PCA-based axis estimation for non-canonical orientation." |
| Larsen et al. 2000 `larsen2000ssv` | Academic (ICRA 2000) | DOI: 10.1109/ROBOT.2000.845311 | Yes (correct entry) | verified | Introduces rectangle-swept sphere (RSS) bounding volume hierarchies for distance queries. Establishes swept-sphere distance computation theory. | Current text is accurate. The capsule distance function (point-to-line-segment minus radius) is correctly identified as a special case of the swept-sphere family. No change needed. |

### Source Statistics

| Metric | Value |
|--------|-------|
| number_sources_checked | 5 |
| number_sources_verified (academic) | 3 (Foam, Wu 2018, Larsen 2000) |
| number_sources_nonacademic_but_relevant | 2 (Tesseract URDF docs, MathWorks capsuleApproximation) |
| number_sources_to_add_to_ref.bib | 1 (Tesseract URDF Package) |
| number_claims_to_weaken_or_scope | 2 (see below) |

### Claims Requiring Weakening or Scoping

**Claim 1 (related_work.tex line 9):** "URDF ... lacks native capsule elements, requiring capsule-based approximations to be decomposed into cylinders and spheres."

- **Finding**: This is correct for standard URDF (urdfdom). However, Tesseract's custom parser does support `<capsule>` tags. The statement should be scoped to "standard OSRF urdfdom" or "the ROS URDF specification."
- **Recommended change**: Scope to standard URDF: "The ROS URDF specification supports a fixed set of collision primitives---spheres, cylinders, boxes, and triangular meshes---but does not include a native capsule element. (Custom parsers such as Tesseract's add capsule support, but at the cost of parser-specific compatibility.)"

**Claim 2 (related_work.tex lines 36-37):** "no existing open-source toolchain integrates them into a single URDF-to-URDF pipeline with sidecar primitive parameter export, validation metrics, and presets for reproducible tuning."

- **Finding**: This is accurate when considering the specific combination of all features (URDF-to-URDF pipeline + capsules + convex + sidecar JSON + validation + presets). Foam does URDF-to-URDF for spheres only; Tesseract parses capsules but does not generate them from meshes. However, the phrase "no existing" is strong and should be scoped precisely.
- **Recommended change**: Replace with: "To the best of our knowledge, no existing open-source toolchain simultaneously produces capsule, sphere-tree, and convex approximations from URDF meshes within a single pipeline, while also exporting analytic primitive parameters as structured sidecar data alongside the URDF output."

### Tesseract URDF Capsule Support: Impact on URDF Compatibility Discussion

**Does Tesseract's capsule tag support change the URDF compatibility discussion?** Yes, moderately.

Key facts:
1. Tesseract's `tesseract_urdf` parser (documented at the URL above) accepts `<capsule radius="..." length="..."/>`, `<convex_mesh filename="..."/>`, and other non-standard tags.
2. The standard URDF parser (`urdfdom`) does NOT recognize these tags and will reject them.
3. Tesseract also supports URDF versioning (version 1 vs. 2 on `<robot>`) to control mesh interpretation.

Impact on the paper:
- The paper should NOT claim URDF universally lacks capsule support. It should say "the standard URDF specification" or "urdfdom" lacks it.
- Tesseract demonstrates that extending URDF parsers with capsule tags is feasible, but this approach fragments compatibility.
- Our approach (decomposing capsules into standard primitives) maintains compatibility with ALL URDF parsers, which is a legitimate advantage to highlight.
- **Recommended addition**: A brief mention in related work or discussion: "Custom URDF parsers such as Tesseract's add native capsule geometry tags, but these extensions are parser-specific. Our approach targets standard URDF compatibility, requiring no parser modifications."

### MathWorks capsuleApproximation: Citation or Limitations?

**Should MathWorks' capsuleApproximation be cited or mentioned only in limitations?** Limitations only, with a brief mention in discussion.

Rationale:
- It is a proprietary, closed-source MATLAB function with no published algorithm, no DOI, and no author attribution.
- The fitting methodology is undocumented (the `fitCollisionCapsule` helper reveals only the residual metric formula, not the optimization method).
- It cannot be cited as academic prior art. It does not meet the standard for a scholarly reference.
- **Recommended use**: Mention briefly in the Discussion/Limitations section to acknowledge commercial alternatives: "While commercial toolboxes such as MathWorks Robotics System Toolbox include capsule fitting functionality (capsuleApproximation, introduced in R2022b), their closed-source implementation prevents algorithmic comparison or reproducibility."

### BibTeX Entries for Addition

One entry should be added to `ref.bib`:

**Tesseract URDF Package (software documentation):**

```bibtex
@misc{tesseract_urdf,
  title        = {{Tesseract URDF Package}},
  author       = {{Tesseract Robotics}},
  year         = {2025},
  url          = {https://tesseract-docs.readthedocs.io/en/latest/_source/core/packages/tesseract_urdf_doc.html},
  note         = {Accessed: 2026-07-06}
}
```

MathWorks capsuleApproximation should NOT be added to ref.bib. It should be mentioned as an uncitable commercial reference in the Discussion section only.

## Claim Support Summary

- **supported**: The existing ref.bib entries for Foam, Wu 2018, and Larsen 2000 are correct and verified. Their treatment in related_work.tex is accurate.
- **partially_supported**: The claim that URDF "lacks native capsule elements" needs scoping to standard URDF. The "no existing open-source toolchain" claim needs precise qualification.
- **partially_supported**: The related work does not currently mention Tesseract's URDF extensions, which is a gap that should be filled for accurate positioning.
- **supported**: MathWorks capsuleApproximation should not be cited as academic prior art. Limitations mention is the appropriate treatment.

## Limitations

- Web search only; no full-text PDF review of Foam or Wu 2018 was performed beyond abstracts and secondary sources.
- Tesseract documentation was consulted via the ReadTheDocs site, which may not cover all version branches or extension capabilities.
- MathWorks fitting algorithm details are not publicly documented; conclusions about its closed-source nature are based on absence of published methodology.

## Next Actions For Writer

1. **Scope URDF capsule claim**: In related_work.tex line 9, change "URDF" to "The standard ROS URDF specification" or similar, to acknowledge Tesseract's parser extension without undermining the point.
2. **Add Tesseract mention**: Insert a sentence in section 4 (Capsule and Swept-Sphere Primitives) or section 5 (Tooling Gap) acknowledging Tesseract's custom URDF capsule tags as an alternative parser-extension approach.
3. **Scope the "no existing toolchain" claim**: In related_work.tex lines 36-37, replace categorical language with a scoped statement about the specific feature combination (capsule + sphere-tree + convex + sidecar + validation + presets) not being available in a single open-source pipeline.
4. **Add `@misc{tesseract_urdf`** entry to ref.bib (see BibTeX above).
5. **Do NOT add MathWorks to ref.bib**. Instead, add a brief mention in `discussion.tex` or `limitations.tex` about commercial closed-source alternatives.
6. **Foam description enhancement** (optional): The current description is accurate. Could add the specific algorithm name (AMAA) for precision.

## Loop Handoff

IMPLEMENT_DONE related-work-positioning-audit doc/paper/artifacts/related-work-positioning-audit/RESULT.md
