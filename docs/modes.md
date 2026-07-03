# Modes

URDFApproxGeom supports three collision-geometry approximation modes.

## Convex

Generates convex hull collision meshes from the original mesh geometry. The output URDF references the generated `.obj` convex hull files in its collision elements. This mode provides a tight mesh-based collision approximation with minimal authoring effort.

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli generate --mode convex -i robot.urdf -o out/robot_convex.urdf
```

## Sphere

Builds a sphere-tree approximation that covers the mesh surface with multiple spheres. The output URDF contains `<sphere>` collision primitives, and a JSON sidecar records the per-link sphere parameters. Best for simulators and planners that prefer simple primitives over mesh-based collision.

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli generate --mode sphere -i robot.urdf -o out/robot_spherized.urdf
```

## Capsule

Fits one or more covering capsules per link whose segment follows the mesh's principal axis (PCA). Each capsule is emitted as a `<cylinder>` plus two `<sphere>` end-caps in the output URDF (since urdfdom has no `<capsule>` element), and the capsule parameters are written to a JSON sidecar with `p0`, `p1`, and `radius` per entry. This mode provides smooth, low-count approximations ideal for collision atlas work.

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli generate --mode capsule -i robot.urdf -o out/robot_capsule.urdf --preset default
```
