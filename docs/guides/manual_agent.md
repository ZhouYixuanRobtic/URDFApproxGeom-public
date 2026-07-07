# SpherizedURDFGenerator — Agent Manual

For Claude Code / AI tools. Code blocks copy-paste ready. No prose where a command suffices.

## Quick Reference

```bash
# Configure
cmake -B build . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Presets
cmake --preset dev        # Debug + tests + pybind
cmake --preset release    # Release + LTO
cmake --preset asan       # Debug + sanitizers
cmake --preset coverage   # Debug + coverage
```

## Key options

| Option | Default | Effect |
|--------|---------|--------|
| `ENABLE_TEST` | ON | GoogleTest + CTest |
| `COMPILE_URDFApproxGeom_PYBINDING` | OFF | pybind11 Python module |
| `ENABLE_COVERAGE` | OFF | gcov instrumentation |
| `ENABLE_SANITIZER` | OFF | ASan + UBSan (Debug only) |
| `ENABLE_DOXYGEN` | OFF | API docs |
| `ENABLE_STATIC` | OFF | Static (ON) vs shared (OFF) library |
| `ENABLE_WARNINGS` | ON | Compiler warning flags |

## Build

```bash
# Release
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCOMPILE_URDFApproxGeom_PYBINDING=ON
cmake --build build -j$(nproc)

# Debug
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TEST=ON
cmake --build build -j$(nproc)
```

## Test

```bash
cmake -B build -DENABLE_TEST=ON && cmake --build build -j$(nproc)
cd build && ctest --output-on-failure

# Single test binary
./build/test/test_capsule --gtest_color=no
./build/test/test_spheretree --gtest_color=no
```

## Coverage

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TEST=ON -DENABLE_COVERAGE=ON
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
gcovr -r .. --html --html-details -o coverage.html
```

## Python bindings

```bash
cmake -B build -DCOMPILE_URDFApproxGeom_PYBINDING=ON
cmake --build build -j$(nproc)

# Smoke test
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m pytest python/tests -q

# CLI
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom --help
```

## Docker

```bash
# Build image from repo root
docker build -t urdfapprox -f docker/Dockerfile .

# Pull published image
docker pull irmv-docker-hub-registry.cn-shanghai.cr.aliyuncs.com/manipulation/urdfapprox:1.5.0

# Dev container
docker compose -f docker/docker-compose.yml up -d
docker exec -it spherized-dev bash

# Build and test in container
docker run --rm -v $PWD:/workspace spherized-dev bash -lc '
  cmake -B build -DCMAKE_BUILD_TYPE=Release -DCOMPILE_URDFApproxGeom_PYBINDING=ON
  cmake --build build -j$(nproc)
  ./build/test/test_capsule --gtest_color=no
'
```

## Sanitizers

```bash
cmake --preset asan && cmake --build --preset asan && ctest --preset asan
```

## Package

```bash
# .deb package
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)
cd build && cpack -G DEB

# Python wheel (with pybinding)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCOMPILE_URDFApproxGeom_PYBINDING=ON
cmake --build build -j$(nproc)
cd python && pip3 wheel .
```

## Pre-commit

```bash
pip install pre-commit && pre-commit install
# Baseline format: find . -name "*.cpp" -o -name "*.h" | xargs clang-format -i
# Commit: git add -A && git commit -m "style: apply clang-format baseline"
```

## File map (what to edit for each task)

| Task | Files |
|------|-------|
| Add dep | `CMakeLists.txt` (find_package + link + THIS_PACKAGE_BINARY_DEPENDS) |
| Add src file | `CMakeLists.txt` (add_library sources) |
| Add header dir | `CMakeLists.txt` (THIS_PACKAGE_INCLUDE_DIRS) |
| Add test | `test/<name>.cpp` + `test/CMakeLists.txt` |
| Add python binding | `interface/bindings/<name>_bindings.cpp` |
| Add third-party src | `CMakeLists.txt` (THIS_PACKAGE_TARGETS + add_subdirectory) |
| Add third-party header | Drop in `third_party/header_only/` — no code change |
| Change build default | `CMakeLists.txt` (option defaults) |
| CI config | `.github/workflows/ci.yml` or `.code.yml` |
