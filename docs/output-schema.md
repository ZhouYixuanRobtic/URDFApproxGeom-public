# Output Schema

URDFApproxGeom produces different output artifacts depending on the approximation mode.

## Convex Output

A URDF file where each link's `<collision>` element references a generated convex `.obj` mesh file. No JSON sidecar is produced.

## Sphere Output

`sphere` writes an output URDF and a JSON sidecar. The canonical per-link field is `spheres`, where each entry has `center` and `radius`. Some legacy sphere-tree outputs may also include `BiggestSphere` and `SubSpheres`; new consumers should read `spheres`.

```json
{
  "link_name": {
    "spheres": [
      {"center": [x, y, z], "radius": r}
    ]
  }
}
```

## Capsule Output

A URDF file with `<cylinder>` plus two `<sphere>` end-cap collision primitives (urdfdom has no `<capsule>` element), and a JSON sidecar containing per-link capsule parameters. Capsule endpoints `p0` and `p1` are sphere centers in the link frame, not mesh axial extrema:

```json
{
  "link_name": {
    "capsules": [
      {"p0": [x, y, z], "p1": [x, y, z], "radius": r}
    ]
  }
}
```

Each capsule entry describes a capped cylinder: `p0` and `p1` define the segment endpoints (sphere centers), and `radius` is the capsule radius. The segment is the capsule's central axis, and the two hemispherical end-caps are centered at `p0` and `p1`.

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli generate --mode capsule -i robot.urdf -o out/robot_capsule.urdf
# Output: out/robot_capsule.urdf + out/robot_capsule.json
```
