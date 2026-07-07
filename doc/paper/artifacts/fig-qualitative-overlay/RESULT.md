# Result: fig-qualitative-overlay

status: done
source_request: doc/paper/requests/fig-qualitative-overlay.md

## Claim Under Test

Qualitative visual comparison of approximation modes on a representative FR3 link shows the geometric trade-offs among mesh, convex hull, sphere tree, and capsule primitives.

## Prerequisite Data

The overlay figure requires generated URDF/JSON data from the FR3 mode comparison experiment. This was re-generated as a prerequisite:

```bash
# Generate FR3 mode comparison data (prerequisite)
cd /home/admin1/projects/SpherizedURDFGenerator
export PYTHONPATH="$PWD/python:$PWD/scripts"
python3 scripts/run_fr3_mode_comparison.py \
  --artifacts doc/paper/artifacts/exp-fr3-mode-comparison
```

## Commands

All commands run from the repository root.

```bash
cd /home/admin1/projects/SpherizedURDFGenerator

# Generate the 2x2 qualitative overlay figure
# The script re-execs itself with -S to work around a system/user matplotlib
# version conflict on mpl_toolkits
python3 doc/paper/artifacts/fig-qualitative-overlay/render_overlay_grid.py
```

## Script

The custom rendering script is included as an artifact for reproducibility:

- `doc/paper/artifacts/fig-qualitative-overlay/render_overlay_grid.py`

It performs:
1. Load FR3 link3 visual mesh from `resources/fr3/meshes/fr3/visual/link3.dae` (13402 vertices, 21566 faces)
2. Compute convex hull via trimesh (908 vertices, 1812 faces)
3. Load sphere tree from `doc/paper/artifacts/exp-fr3-mode-comparison/work/fr3_sphere_default.json` (6 spheres)
4. Load capsule from `doc/paper/artifacts/exp-fr3-mode-comparison/work/fr3_capsule_default.json` (1 capsule)
5. Extract unique edges (35325) for efficient wireframe rendering
6. Render a 2x2 matplotlib 3D grid with consistent view angle (elev=25, azim=-60)

## Artifacts

| Artifact | Description |
|---|---|
| `fig_qualitative_overlay.png` | 2x2 grid figure, 2104x1952 px, 200 DPI (1.46 MB) |
| `fig_qualitative_overlay.pdf` | Vector figure for paper inclusion (3.25 MB) |
| `render_overlay_grid.py` | Reproducible rendering script |

## Results

### Panel Layout

| Panel | Label | Content | Color |
|---|---|---|---|
| (a) Top-left | Original visual mesh | Gray wireframe of link3.dae (13402 verts) | gray |
| (b) Top-right | Convex hull overlay | Convex hull (908 verts) in blue over wireframe | #457B9D |
| (c) Bottom-left | Sphere tree overlay | 6 spheres (default preset) in red over wireframe | #E63946 |
| (d) Bottom-right | Capsule primitives overlay | 1 capsule (default preset) in teal over wireframe | #2A9D8F |

### Key Observations

- **Convex hull** (panel b): Tightly follows the mesh silhouette. Overlay aligns well with the wireframe since the hull encloses all vertices by construction. Note: displayed overlay is the hull computed from the visual mesh; the actual convex pipeline may produce a different mesh depending on mesh-source selection.
- **Sphere tree** (panel c): 6 spheres of varying radii cover the link body. The overlay shows spheres protruding beyond the mesh surface in some regions (expected for coverage-oriented sphere fitting).
- **Capsule primitive** (panel d): A single capsule (1 primitive) traces the elongated axis of link3. Visible volume extends beyond the mesh in thinner sections but provides a smooth, single-primitive analytic envelope. capV/aabb for this link is 0.63 (tight).
- View angle (elev=25, azim=-60) highlights the link's elongated shape along the arm axis.

## Claim Support Summary

- **supported**: The figure provides qualitative visual comparison across all four representations. The trade-off between primitive count and geometric fidelity is visually evident: convex hull (watertight mesh, many facets), sphere tree (6 discrete spheres), capsule (1 smooth primitive).
- The capsule's single-primitive coverage is clearly visible, supporting the paper's claim that capsule fitting produces low-count analytic envelopes.

## Limitations

- The convex hull shown is computed from the visual mesh by trimesh, not from the tool's convex pipeline (which uses CGAL/libigl and outputs different meshes). The visual comparison is representative but not pixel-identical to the tool's convex output.
- Single view angle (elev=25, azim=-60) -- other angles may reveal different coverage characteristics.
- Only FR3 link3 is shown; generalization to other links and robots is qualitative.
- Only the `default` preset is shown for sphere and capsule modes. Other presets (single, high_detail) would produce different numbers of primitives.

## Next Actions For Writer

- The figure directly supports the qualitative comparison claim in the experimental section (Section 6, Figure 2 per outline).
- Reference the figure as "Fig. 2: Qualitative overlay comparison of approximation modes on FR3 link3."
- The capsule panel (d) with a single primitive supporting capV/aabb ~0.63 can be referenced alongside the quantitative ablation results.
- Note that convex hull panel uses trimesh-computed hull, not CGAL/libigl pipeline output -- clarify in the caption if exact equivalence matters.
