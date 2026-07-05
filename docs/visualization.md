# Visualization

URDFApproxGeom's `visualize` command ships three backends. The default, **`robot_viewer`**, is the recommended way to compare the high-precision `.dae` visual meshes against the generated capsule / sphere / convex collision approximation in the same window.

## robot_viewer (default)

`robot_viewer` is a browser-based URDF viewer (https://github.com/fan-ziqi/robot_viewer). It renders the `<visual>` and `<collision>` geometry with a toggle, so you can flip between the true shape and the approximation. The generated URDFs reference absolute `/workspace/...` mesh paths that do not exist on a host, so the CLI bundles the URDF and every referenced mesh into a self-contained directory with relative paths first:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli visualize \
  --mode capsule --urdf /tmp/fr3_approx/fr3_capsule.urdf --viewer robot_viewer
```

Output:

```
capsule: bundled /tmp/fr3_approx/fr3_capsule.urdf -> /tmp/urdf_approx_rv_XXXX/fr3_capsule.urdf
bundle ready: /tmp/urdf_approx_rv_XXXX
launched robot_viewer dev server at http://localhost:5173 (root=/home/admin1/ref/robot_viewer)
open http://localhost:5173 in a browser, then drag this directory onto the file tree:
  /tmp/urdf_approx_rv_XXXX
toggle Visual / Collision in the viewer to compare the .dae meshes against the approximation.
```

If `ROBOT_VIEWER_ROOT` is unset and no local `robot_viewer` checkout is found, the command falls back to the hosted demo (http://viewer.robotsfan.com) — drag the bundle directory onto the file tree and the model stays in your browser. Use `--no-launch` to skip starting the dev server and only write the bundle, e.g. when running inside the Docker container where `pnpm`/the checkout live on the host.

The same workflow works for sphere and convex output:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli visualize \
  --mode sphere --urdf /tmp/fr3_approx/fr3_spherized.urdf
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli visualize \
  --mode convex --urdf /tmp/fr3_approx/fr3_convex.urdf
```

## PyBullet and MJCF backends

For headless renders or capsule-only overlays, the legacy backends remain available:

```bash
# pybullet GUI (or PNG with --png) overlaying red capsule meshes on the link collision meshes
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli visualize \
  --mode capsule --urdf /tmp/fr3_approx/fr3_capsule.urdf --viewer pybullet \
  --json /tmp/fr3_approx/fr3_capsule.json

# MuJoCo MJCF scene (capsule primitives + gray link meshes)
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli visualize \
  --mode capsule --urdf /tmp/fr3_approx/fr3_capsule.urdf --viewer mjcf \
  --json /tmp/fr3_approx/fr3_capsule.json --mjcf /tmp/fr3_approx/fr3_capsules.xml
```

## Validation

A companion validation command checks capsule tightness against the original mesh using metrics like capsule-volume-to-AABB ratio and radius bin-median:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli validate --mode capsule --json /tmp/fr3_approx/fr3_capsule.json --urdf resources/fr3/urdf/fr3.urdf --max-capv-aabb 2.50 --max-r-binmed 1.50
```

## Compare Capsule Sidecars

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli compare --mode capsule --baseline-json /tmp/fr3_single.json --candidate-json /tmp/fr3_default.json --urdf resources/fr3/urdf/fr3.urdf --max-capv-aabb 2.50 --max-r-binmed 1.50
```

By default, `compare` summarizes both sidecars and validates only the candidate
against absolute ceilings. Add `--require-improvement` when a tuning run must
also improve or match the baseline on worst capV/aabb and r/binMed.

## Third-Party Viewers

The generated URDF files can be opened in any URDF-compatible viewer or simulator:

- **PyBullet**: `python3 -c "import pybullet as p; p.connect(p.GUI); p.loadURDF('out/robot_capsule.urdf')"`
- **MuJoCo**: convert via `make_mjcf.py` or open the URDF directly in MuJoCo 3.0+
- **ROS**: `roslaunch robot_state_publisher` with the generated URDF
- **MeshLab**: open the convex `.obj` files directly for mesh inspection
