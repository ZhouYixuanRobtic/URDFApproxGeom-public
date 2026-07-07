# Implementation Request: fig-capsule-method-schematic

request_id: fig-capsule-method-schematic
source: writer
status: open
owner: implement

## Paper Claim
Visual explanation of the capsule fitting method: PCA axis computation, cross-section slicing, circle fitting, and capsule chaining.

## Task Type
figure

## Required Outputs
A reproducible schematic (generated figure, not hand-drawn) showing the capsule fitting pipeline on a representative FR3 link (preferably link0 or link3). The figure should include sub-panels for:
1. Input mesh with PCA principal axis overlaid.
2. Cross-section planes perpendicular to the axis.
3. Fitted covering circles on selected cross-section planes (2D view).
4. Resulting capsule chain in 3D overlaid on the original mesh.

## Formats
- PDF or PNG at publication resolution (300+ DPI).
- Source script if generated programmatically (recommended).
- Place the output in `doc/paper/artifacts/fig-capsule-method-schematic/`.

## Inputs
- Repository root with visualization scripts: `scripts/viz_capsules.py`, `scripts/check_capsule_coverage.py`.
- Example: Use the capsule/default preset on the FR3 robot to generate intermediate data.
- The `visualize` CLI command with `--viewer pybullet` may be adapted for programmatic screenshot generation.

## Acceptance Criteria
- Figure shows at least 4 sub-panels as described above.
- Figure is reproducible (source script or notebook provided).
- Figure is stored as `doc/paper/artifacts/fig-capsule-method-schematic/capsule_method_schematic.pdf`.

## Writer Integration Target
- Section IV (Method), Figure 3.
