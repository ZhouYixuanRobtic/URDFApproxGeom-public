# Configuration Presets

Presets provide ready-to-use configuration profiles for each mode. List available presets with:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli presets
```

## Capsule Presets

- **`single`**: one conservative capsule per link. Fastest, lowest primitive count.
- **`default`**: recommended low-count conservative capsule fit. Balances tightness and capsule count through multi-section axial decomposition.
- **`high_detail`**: more axial sections (`NSections: 6`) and a larger capsule budget (`MaxCapsulesPerLink: 16`). Raises the detail ceiling but does **not** guarantee tighter output than `default` — the fitter only accepts splits that improve coverage, so on many meshes both presets converge to the same capsule count. On FR3 both emit 17 capsules; `default` has a marginally lower worst capV/aabb (1.77 vs 1.86) while `high_detail` lowers worst r/binMed (1.35 vs 1.48).
- **`tight`**: alias for `high_detail`.

The capsule fitter uses Wu2018 cross-section decomposition: mesh planes perpendicular to the principal axis are sliced into contours, circles are fitted per plane, and circles are linked across adjacent planes into capsule chains. Presets control `NSections`, `MaxCapsulesPerLink`, and related parameters.

## Sphere Presets

- **`single`**: one conservative sphere per link. Minimal output size.
- **`default`**: sphere-tree fit. Builds a medial-axis sphere tree that covers the mesh with multiple spheres.

## Convex Presets

- **`default`**: convex hull collision mesh generation. Computes the convex hull of each mesh link using CGAL and writes the result as `.obj` files.
