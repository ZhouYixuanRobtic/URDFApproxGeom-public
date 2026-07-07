# Implementation Request: related-work-positioning-audit

request_id: related-work-positioning-audit
source: gatekeeper-review
status: open
owner: implement

## Paper Claim
URDFApproxGeom fills a narrow integration gap, not a blank-field algorithmic gap. The related-work section needs verified prior-art positioning for Foam, Tesseract URDF extensions, capsule approximation workflows, and swept-sphere fitting.

## Task Type
citation_check

## Required Outputs
- `doc/paper/artifacts/related-work-positioning-audit/RESULT.md`
- A citation table with source, verified URL/DOI, claim relevance, and recommended wording.
- BibTeX entries for sources that should be added to `doc/paper/ref.bib`.

## Metrics
- number_sources_checked
- number_sources_verified
- number_sources_to_add
- number_claims_to_weaken

## Inputs
- `doc/paper/sections/related_work.tex`
- `doc/paper/ref.bib`
- Current known sources:
  - Foam: arXiv:2503.13704
  - Tesseract URDF package documentation for capsule and convex mesh extensions
  - MathWorks capsule approximation workflow for rigid body trees
  - Wu et al. 2018, Variational Mannequin Approximation Using Spheres and Capsules
  - Larsen et al. 2000, Fast distance queries with rectangular swept sphere volumes

## Acceptance Criteria
- Each source is marked `verified`, `not_verified`, or `nonacademic_but_relevant`.
- RESULT.md recommends exact related-work wording that avoids the phrase "no existing tool" unless scoped precisely.
- RESULT.md identifies whether Tesseract support for capsule tags changes the URDF compatibility discussion.
- RESULT.md identifies whether MathWorks should be cited as a nonacademic workflow or mentioned only in limitations.

## Writer Integration Target
- `doc/paper/sections/related_work.tex`
- `doc/paper/sections/discussion.tex`
- `doc/paper/ref.bib`
