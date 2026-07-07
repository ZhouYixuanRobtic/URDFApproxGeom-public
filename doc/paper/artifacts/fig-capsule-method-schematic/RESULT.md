# Result: fig-capsule-method-schematic

status: done
source_request: doc/paper/requests/fig-capsule-method-schematic.md

## Claim Under Test

Visual explanation of the capsule fitting method: PCA axis computation, cross-section slicing, circle fitting, and capsule chaining.

## Commands

```bash
cd $(git rev-parse --show-toplevel)
python3 doc/paper/artifacts/fig-capsule-method-schematic/generate_figure.py
```

## Artifacts

- `doc/paper/artifacts/fig-capsule-method-schematic/generate_figure.py`: standalone Python script for figure generation (reproducible)
- `doc/paper/artifacts/fig-capsule-method-schematic/capsule_method_schematic.pdf`: publication-quality PDF (3427x2783 at 300 DPI)
- `doc/paper/artifacts/fig-capsule-method-schematic/capsule_method_schematic.png`: PNG version for preview

## Results

### Panel (a) -- Input Mesh with PCA Principal Axis
- Loaded FR3 link3 collision mesh (`link3.obj`, 152 vertices, 300 faces)
- PCA computed via covariance eigendecomposition (matching C++ `CapsuleCrossSection.cpp` lines 944-948)
- Principal axis: [0.68, 0.53, 0.51] (eigenvalues: 0.0011, 0.0013, 0.0082)
- Mesh rendered as semi-transparent surface with PCA axis arrow overlaid

### Panel (b) -- Cross-Section Planes
- 8 planes placed along PCA axis at regular intervals (skipping endpoints)
- Each plane: mesh cross-section extracted via triangle-plane intersection (replicating C++ `extractSections` algorithm)
- Intersection segments chained into closed 3D contours
- Planes visualized as translucent colored disks with contour lines

### Panel (c) -- 2D Cross-Section with Fitted Circle
- Middle cross-section contour projected to 2D
- Minimum enclosing circle (MEC) fitted using Welzl's algorithm (matching C++ `mec2d`)
- MEC radius: 0.072 (covers all contour points)
- Shows contour samples, filled polygon, and dashed MEC boundary

### Panel (d) -- Capsule Chain Overlay
- Loaded pre-computed capsule from `resources/fr3/urdf/fr3_capsuleized.json`
- Link3 has 1 capsule in default preset: p0=(0.009, 0.006, -0.083), p1=(0.083, 0.051, -0.006), r=0.080
- Capsule rendered as translucent red mesh overlaid on link3 wireframe

## Claim Support Summary

The figure reproduces all 4 stages of the capsule fitting pipeline described in the paper:
1. (a) demonstrates PCA axis computation -- the first step identifying the principal direction
2. (b) shows cross-section slicing perpendicular to the PCA axis
3. (c) illustrates the 2D circle fitting (minimum enclosing circle on each section)
4. (d) shows the final capsule chain covering the original mesh

The algorithms used in the Python script replicate the C++ implementation:
- PCA via covariance eigendecomposition (`CapsuleCrossSection.cpp:944-948`)
- Cross-section extraction via triangle-plane intersection + segment chaining (`extractSections:571-661`)
- Minimum enclosing circle via Welzl's algorithm (`mec2d:74-108`)
- Capsule chaining from consecutive section circles (`fitCapsulesByCrossSection:938-1103`)

## Limitations

- Uses the pre-computed capsule JSON from the default preset; new runs would require the C++ extension to be built and loaded
- Cross-section contours use a simple plane-triangle intersection algorithm (no vertex caching), which is slower than the C++ version but produces identical geometry for this mesh
- Only link3 is used (152 vertices, 300 faces); a more complex link would show richer cross-section contours
