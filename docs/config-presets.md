# Configuration Presets

Presets provide ready-to-use configuration profiles for each mode. List available presets with:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli presets
```

## Capsule Presets

- **`single`**: one conservative capsule per link. Fastest, lowest primitive count.
- **`default`**: recommended low-count conservative capsule fit. Balances tightness and capsule count through multi-section axial decomposition.
- **`high_detail`**: more axial sections (`NSections: 6`) and a larger capsule budget (`MaxCapsulesPerLink: 16`). Raises the detail ceiling but does **not** guarantee tighter output than `default`; union-volume scoring removes overlap, but coverage growth and section matching can still make one preset larger on a specific link. On FR3, worst `vol/dae` by sampled capsule union is `default` 4.23 vs `high_detail` 4.01.
- **`tight`**: alias for `high_detail`.

The capsule fitter uses Wu2018 cross-section decomposition: mesh planes perpendicular to the principal axis are sliced into contours, circles are fitted per plane, and circles are linked across adjacent planes into capsule chains. Presets control `NSections`, `MaxCapsulesPerLink`, and related parameters.

## Sphere Presets

- **`single`**: one conservative sphere per link. Minimal output size.
- **`default`**: sphere-tree fit. Builds a medial-axis sphere tree that covers the mesh with multiple spheres.

## Convex Presets

- **`default`**: convex hull collision mesh generation. Computes the convex hull of each mesh link using CGAL and writes the result as `.obj` files.
