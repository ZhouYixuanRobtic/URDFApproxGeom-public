# Quickstart

This guide runs the FR3 example through all public modes.

## Build

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCOMPILE_URDFApproxGeom_PYBINDING=ON
cmake --build build -j$(nproc)
```

## Generate

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli generate --mode all -i resources/fr3/urdf/fr3.urdf --output-dir /tmp/fr3_approx
```

Expected outputs:

- `/tmp/fr3_approx/fr3_convex.urdf`
- `/tmp/fr3_approx/fr3_spherized.urdf`
- `/tmp/fr3_approx/fr3_spherized.json`
- `/tmp/fr3_approx/fr3_capsule.urdf`
- `/tmp/fr3_approx/fr3_capsule.json`

## Validate Capsules

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli validate --mode capsule --json /tmp/fr3_approx/fr3_capsule.json --urdf resources/fr3/urdf/fr3.urdf --max-capv-aabb 2.50 --max-r-binmed 1.50
```

## Visualize Capsules

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli visualize --mode capsule --urdf resources/fr3/urdf/fr3.urdf --json /tmp/fr3_approx/fr3_capsule.json --mjcf /tmp/fr3_approx/fr3_capsules.xml
```
