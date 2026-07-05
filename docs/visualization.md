# Visualization

The fastest way to judge approximation quality is to load every variant in [robot_viewer](https://github.com/fan-ziqi/robot_viewer) and flip between the high-precision `.dae` visual meshes and each collision approximation.

## Install robot_viewer

robot_viewer is a third-party web viewer and is **not** bundled with this repo. Install it once:

```bash
git clone https://github.com/fan-ziqi/robot_viewer.git
cd robot_viewer && pnpm install       # or npm install
pnpm run dev                           # serves http://localhost:5173
```

Set `ROBOT_VIEWER_ROOT` to the checkout if you want the CLI to launch the dev server for you; otherwise start it yourself.

## Compare all variants side by side

`compare-all` generates convex + sphere (single, default) + capsule (single, default, high_detail) from your input URDF, rewrites every mesh reference to a relative `meshes/<name>`, and copies the referenced meshes into one self-contained directory:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli compare-all \
  -i resources/fr3/urdf/fr3.urdf --bundle-dir out/compare_bundle
```

Drag `out/compare_bundle` onto robot_viewer's file tree. Each variant appears as its own URDF; click between them and toggle **Visual / Collision** to compare the `.dae` ground truth against the approximation. Use `--presets single,default` to limit which sphere/capsule presets are generated (default: `single,default,high_detail`).

## Single-mode visualize

For a single generated URDF, `visualize --viewer robot_viewer` (the default) bundles just that one:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli visualize \
  --mode capsule --urdf out/fr3_capsule.urdf
```

`--viewer mjcf` and `--viewer pybullet` keep the legacy backends (MuJoCo XML output, pybullet capsule overlay) for headless renders.

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
