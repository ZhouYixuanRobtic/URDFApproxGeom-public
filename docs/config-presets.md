# Configuration Presets

Presets provide ready-to-use configuration profiles for each mode. List available presets with:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli presets
```

## Capsule Presets

- **`single`**: one conservative capsule per link. Fastest, lowest primitive count.
- **`default`**: recommended low-count conservative capsule fit. Balances tightness and capsule count through multi-section axial decomposition.
- **`high_detail`**: more axial sections and a larger capsule budget. Produces tighter approximations with more capsules per link.

The capsule fitter uses Wu2018 cross-section decomposition: mesh planes perpendicular to the principal axis are sliced into contours, circles are fitted per plane, and circles are linked across adjacent planes into capsule chains. Presets control `NSections`, `MaxCapsulesPerLink`, and related parameters.

## Sphere Presets

- **`single`**: one conservative sphere per link. Minimal output size.
- **`default`**: sphere-tree fit. Builds a medial-axis sphere tree that covers the mesh with multiple spheres.

## Convex Presets

- **`default`**: convex hull collision mesh generation. Computes the convex hull of each mesh link using CGAL and writes the result as `.obj` files.
