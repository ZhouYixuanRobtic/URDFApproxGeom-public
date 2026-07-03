# Developer Guide

## Project Structure

```
include/                          # C++ headers
├── CapsuleFitter.h               # Capsule struct and covering fit
├── CapsuleCrossSection.h         # Wu2018 cross-section decomposition
├── CapsuleURDFGenerator.h        # Capsule pipeline generator
└── SphereTreeURDFGenerator.h     # Sphere-tree and convex generators
src/                              # C++ implementations
├── CapsuleFitter.cpp
├── CapsuleCrossSection.cpp
├── CapsuleURDFGenerator.cpp
└── SphereTreeURDFGenerator.cpp
app/                              # CLI entry points (C++ binaries)
├── capsuleized.cpp
├── convex.cpp
└── spherized.cpp
interface/                        # pybind11 bindings
└── urdf_approx_geom.cpp
python/                           # Python package
├── urdf_approx_geom/
├── tests/
└── conftest.py
config/                           # YAML configuration
├── capsule/
└── sphereTree/
```

## Build

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCOMPILE_URDFApproxGeom_PYBINDING=ON
cmake --build build -j$(nproc)
```

The `COMPILE_URDFApproxGeom_PYBINDING=ON` flag enables Python bindings. Without it, only the C++ binaries are built.

## Test

C++ tests use GoogleTest:

```bash
./build/test/test_capsule --gtest_color=no
./build/test/test_spheretree --gtest_color=no
```

Python tests use pytest:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m pytest python/tests -q
```

## Adding a New Mode

1. Add headers and sources under `include/` and `src/`
2. Add a CLI entry point in `app/` (or extend the existing CLI)
3. Add pybind11 bindings in `interface/urdf_approx_geom.cpp`
4. Register the mode in `python/urdf_approx_geom/cli.py`
5. Add C++ tests in `test/` and Python tests in `python/tests/`
6. Add a preset in the relevant `config/` directory

## Capsule Algorithm Overview

The capsule fitter uses Wu2018 cross-section decomposition: mesh planes perpendicular to the principal component axis (PCA) are sliced into 2D contours. Circles are fitted per plane using minimum enclosing circles (Welzl algorithm). Circles are then linked across adjacent planes into capsule chains, which are merged, grown to cover the original mesh, deduplicated, and pruned to respect the per-link budget.
