# Implementation Request: fig-qualitative-overlay

request_id: fig-qualitative-overlay
source: reviewer_iter_3
status: open
owner: implement

## Paper Claim
The three approximation modes (convex, sphere, capsule) produce geometrically distinct outputs with visibly different coverage patterns and tightness characteristics.

## Task Type
figure/generation

## Required Outputs
- One 2×2 grid figure showing FR3 link3 (or another representative link with rich geometry):
  - (a) Original visual mesh (opaque gray)
  - (b) Convex hull overlay (translucent colored, overlaid on original mesh)
  - (c) Sphere tree overlay (default preset, translucent colored, overlaid on original mesh)
  - (d) Capsule overlay (default preset, translucent colored, overlaid on original mesh)
- PNG at 300 DPI, suitable for IEEEtran single-column width
- `doc/paper/artifacts/fig-qualitative-overlay/RESULT.md` with: status (done/blocked/failed), file path to generated figure, rendering commands used, notes on visual observations

## Metrics
Visual fidelity only — no quantitative metrics needed.

## Inputs
- FR3 URDF model (`resources/fr3/`)
- Generated outputs for all three modes (default presets): convex, sphere (default), capsule (default)
- PyBullet or robot_viewer for rendering (already available in the pipeline)

## Acceptance Criteria
- Figure is legible at single-column width (~3.5 inches)
- Overlay alignment is visually correct (approximation geometry registered to original mesh)
- Colors are distinguishable in grayscale print (use distinct line styles or patterns as fallback)
- Each subfigure is clearly labeled

## Writer Integration Target
`sections/experiments.tex` after the mode comparison results (Section VI-E), with caption:
> "Qualitative comparison of approximation modes on FR3 link3. The original visual mesh is shown in gray (background); (a) convex hull, (b) sphere tree (default preset), and (c) capsule primitives (default preset) are overlaid as translucent colored geometry."
