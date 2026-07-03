# Visualization

URDFApproxGeom provides a visualization command that generates an MJCF (MuJoCo) file from capsule approximation data, enabling visual inspection of the fitted capsules against the original mesh.

## Capsule Visualization

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli visualize --mode capsule --urdf resources/fr3/urdf/fr3.urdf --json /tmp/fr3_approx/fr3_capsule.json --mjcf /tmp/fr3_approx/fr3_capsules.xml
```

This command reads the original URDF meshes and the capsule JSON sidecar, then writes an MJCF XML file that overlays the capsule primitives on the original collision geometry. Open the resulting `.xml` in a MuJoCo viewer to inspect fit quality per link.

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
