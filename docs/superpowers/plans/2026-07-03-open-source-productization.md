# Open Source Productization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn the current branch into a usable URDF collision-approximation toolkit where a user provides one mesh-based URDF and can generate three supported outputs: convex mesh, sphere, and capsule.

**Architecture:** Keep the existing C++ generators as the computation core. Add a stable Python package and CLI as the public product layer, add named configuration presets for reproducible behavior, generalize validation/visualization around output contracts, and rewrite documentation around the user journey rather than implementation history.

**Tech Stack:** C++17, CMake, pybind11, Python 3.10+, argparse, dataclasses, GoogleTest, pytest, trimesh, optional pybullet/ManifoldPlus/sphere_tree dependencies, Docker container `spherized-development`.

---

## Scope

This plan intentionally excludes license changes. It covers:

- Public naming and README cleanup.
- Stable Python API and command-line entry point.
- Named presets for convex, sphere, and capsule modes, including single-sphere and single-capsule baselines.
- Validation and comparison commands that do not encode stale assumptions such as "tight must always have more capsules than sparse".
- Generalized visualization entry points.
- CI/release hygiene for C++ and Python tests.
- Targeted architecture documentation without a broad C++ refactor.

All code/test commands must run inside the existing container:

```bash
docker exec spherized-development bash -lc 'cd /workspace && <command>'
```

## File Structure

- Modify `interface/urdf_approx_geom.cpp`: rename the pybind extension module to `_urdf_approx_geom` and allow sphere generation to receive a config path.
- Modify `interface/CMakeLists.txt`: build `_urdf_approx_geom` into `build/python`.
- Create `python/urdf_approx_geom/__init__.py`: public package exports.
- Create `python/urdf_approx_geom/_extension.py`: extension discovery and import boundary.
- Create `python/urdf_approx_geom/presets.py`: named preset resolution for all modes.
- Create `python/urdf_approx_geom/api.py`: stable `generate()` and `generate_all()` API returning structured results.
- Create `python/urdf_approx_geom/cli.py`: public CLI with `generate`, `validate`, `compare`, and `visualize` subcommands.
- Modify `python/urdf_approx_geom_cli.py`: compatibility wrapper that forwards to the package CLI/API.
- Modify `python/conftest.py`: prefer the source package path and CMake extension path during tests.
- Modify `python/pyproject.toml`: version and entry-point metadata aligned with CMake.
- Add tests under `python/tests/`: API contract, CLI contract, preset resolution, validation, and visualization command construction.
- Modify `app/Sphereized.cpp`: accept optional `-c/--config` for sphere-tree config.
- Modify `include/SphereTreeURDFGenerator.h` and `src/SphereTreeURDFGenerator.cpp`: support a config-controlled single-sphere baseline.
- Create/modify preset files under `config/capsule/`, `config/sphereTree/`, and `config/convex/`.
- Create `scripts/urdf_approx_validate.py`: shared validation implementation called by CLI and legacy scripts.
- Create `scripts/urdf_approx_visualize.py`: shared visualization implementation called by CLI and legacy scripts.
- Modify `scripts/check_capsule_tightness.py`, `scripts/compare_capsule_presets.py`, `scripts/viz_capsules.py`, `scripts/make_mjcf.py`: keep compatibility wrappers thin.
- Rewrite `README.md`.
- Create `docs/quickstart.md`, `docs/modes.md`, `docs/config-presets.md`, `docs/python-api.md`, `docs/output-schema.md`, `docs/visualization.md`, `docs/developer-guide.md`.
- Create `.github/workflows/ci.yml`.
- Modify `.gitignore`: ignore build/test/Python generated files that are currently dirtying the worktree.

Do not intentionally commit generated `build/`, `build_dkr/`, `__pycache__/`, `.pytest_cache/`, or generated FR3 output unless a task explicitly says the file is a tracked example artifact.

---

### Task 1: Stabilize the Python Package Boundary

**Files:**
- Modify: `interface/urdf_approx_geom.cpp`
- Modify: `interface/CMakeLists.txt`
- Create: `python/urdf_approx_geom/__init__.py`
- Create: `python/urdf_approx_geom/_extension.py`
- Modify: `python/conftest.py`
- Test: `python/tests/test_extension_import.py`

- [ ] **Step 1: Write the failing import contract test**

Create `python/tests/test_extension_import.py`:

```python
import importlib


def test_public_package_imports_private_extension():
    pkg = importlib.import_module("urdf_approx_geom")
    assert hasattr(pkg, "capsuleized")
    assert hasattr(pkg, "convex")
    assert hasattr(pkg, "spherized")


def test_private_extension_is_not_the_public_package():
    pkg = importlib.import_module("urdf_approx_geom")
    ext = importlib.import_module("_urdf_approx_geom")
    assert pkg is not ext
    assert hasattr(ext, "capsuleized")
```

- [ ] **Step 2: Run the test and verify it fails before implementation**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake -B build -DCOMPILE_URDFApproxGeom_PYBINDING=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build --target urdf_approx_geom -j$(nproc) && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_extension_import.py -q'
```

Expected: FAIL because `_urdf_approx_geom` does not exist and the public package directory does not exist.

- [ ] **Step 3: Rename the pybind module to `_urdf_approx_geom`**

In `interface/urdf_approx_geom.cpp`, replace:

```cpp
PYBIND11_MODULE(urdf_approx_geom, m) {
```

with:

```cpp
PYBIND11_MODULE(_urdf_approx_geom, m) {
```

In `interface/CMakeLists.txt`, replace the current module target block with:

```cmake
# pybind11 extension exposing the C++ generators. The public Python package is
# python/urdf_approx_geom; this compiled module stays private.
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(_urdf_approx_geom urdf_approx_geom.cpp)
target_link_libraries(_urdf_approx_geom PRIVATE ${PROJECT_NAME})

set_target_properties(_urdf_approx_geom PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python")
```

- [ ] **Step 4: Add the public package and extension loader**

Create `python/urdf_approx_geom/_extension.py`:

```python
"""Load the private CMake-built pybind11 extension."""

from __future__ import annotations

import importlib
import pathlib
import sys


def _repo_root() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[2]


def load_extension():
    try:
        return importlib.import_module("_urdf_approx_geom")
    except ImportError:
        build_python = _repo_root() / "build" / "python"
        if build_python.is_dir() and str(build_python) not in sys.path:
            sys.path.insert(0, str(build_python))
        return importlib.import_module("_urdf_approx_geom")
```

Create `python/urdf_approx_geom/__init__.py`:

```python
"""Public Python API for URDF approximate collision geometry generation."""

from __future__ import annotations

from ._extension import load_extension

_ext = load_extension()

capsuleized = _ext.capsuleized
convex = _ext.convex
spherized = _ext.spherized

__all__ = ["capsuleized", "convex", "spherized"]
```

- [ ] **Step 5: Update pytest bootstrap**

Replace `python/conftest.py` with:

```python
"""Pytest bootstrap for source-tree package plus CMake-built extension."""

from __future__ import annotations

import pathlib
import sys

_HERE = pathlib.Path(__file__).resolve().parent
_REPO = _HERE.parent
_EXT = _REPO / "build" / "python"

for path in (str(_HERE), str(_EXT)):
    if path not in sys.path:
        sys.path.insert(0, path)
```

- [ ] **Step 6: Build and verify the import contract**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake -B build -DCOMPILE_URDFApproxGeom_PYBINDING=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build --target _urdf_approx_geom -j$(nproc) && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_extension_import.py -q'
```

Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add interface/urdf_approx_geom.cpp interface/CMakeLists.txt python/conftest.py python/urdf_approx_geom python/tests/test_extension_import.py
git commit -m "feat: expose stable python package boundary"
```

---

### Task 2: Add Structured Python API for One or All Modes

**Files:**
- Create: `python/urdf_approx_geom/api.py`
- Modify: `python/urdf_approx_geom/__init__.py`
- Modify: `python/urdf_approx_geom_cli.py`
- Test: `python/tests/test_api_contract.py`

**Prerequisite:** Execute Task 3 first so `urdf_approx_geom.presets.resolve_preset()` exists before this API imports it.

- [ ] **Step 1: Write the failing API tests**

Create `python/tests/test_api_contract.py`:

```python
import json
import pathlib

import pytest

from urdf_approx_geom import GenerateResult, generate, generate_all


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_generate_capsule_returns_structured_result(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    result = generate("capsule", FR3_URDF, out, preset="default")
    assert isinstance(result, GenerateResult)
    assert result.mode == "capsule"
    assert result.output_urdf == out
    assert result.json_path == pathlib.Path(str(out).replace(".urdf", ".json"))
    assert result.json_path.exists()
    assert result.primitive_count > 0
    assert json.loads(result.json_path.read_text())


def test_generate_convex_has_no_required_json_sidecar(tmp_path):
    out = tmp_path / "fr3_convex.urdf"
    result = generate("convex", FR3_URDF, out)
    assert result.mode == "convex"
    assert result.output_urdf == out
    assert result.json_path is None
    assert result.primitive_count == 0


def test_generate_all_runs_requested_modes(tmp_path):
    results = generate_all(FR3_URDF, tmp_path, modes=["convex", "capsule"])
    assert [r.mode for r in results] == ["convex", "capsule"]
    assert (tmp_path / "fr3_convex.urdf").exists()
    assert (tmp_path / "fr3_capsule.urdf").exists()


def test_generate_rejects_unknown_mode(tmp_path):
    with pytest.raises(ValueError, match="unknown mode"):
        generate("meshlets", FR3_URDF, tmp_path / "out.urdf")
```

- [ ] **Step 2: Run the test and verify it fails**

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_api_contract.py -q'
```

Expected: FAIL because `GenerateResult`, `generate`, and `generate_all` are not exported.

- [ ] **Step 3: Add the structured API**

Create `python/urdf_approx_geom/api.py`:

```python
"""Structured Python API over the C++ URDF approximation generators."""

from __future__ import annotations

from dataclasses import dataclass
import json
import pathlib
from typing import Iterable, Sequence

from ._extension import load_extension
from .presets import resolve_preset

_ext = load_extension()

Mode = str


@dataclass(frozen=True)
class GenerateResult:
    mode: str
    input_urdf: pathlib.Path
    output_urdf: pathlib.Path
    json_path: pathlib.Path | None
    config_path: pathlib.Path | None
    message: str
    primitive_count: int


def _normal_mode(mode: Mode) -> str:
    value = mode.lower().replace("_", "-")
    if value in {"sphere", "spheres", "sphere-tree", "spherized"}:
        return "sphere"
    if value in {"capsule", "capsules", "capsuleized"}:
        return "capsule"
    if value in {"convex", "convex-mesh", "convex_mesh"}:
        return "convex"
    raise ValueError(f"unknown mode {mode!r}; expected convex, sphere, or capsule")


def _sidecar_json(output_urdf: pathlib.Path) -> pathlib.Path:
    if output_urdf.suffix == ".urdf":
        return output_urdf.with_suffix(".json")
    return pathlib.Path(str(output_urdf) + ".json")


def _count_json_primitives(json_path: pathlib.Path | None, key: str) -> int:
    if json_path is None or not json_path.exists():
        return 0
    data = json.loads(json_path.read_text())
    count = 0
    for body in data.values():
        values = body.get(key, [])
        count += len(values)
    return count


def generate(
    mode: Mode,
    input_urdf: str | pathlib.Path,
    output_urdf: str | pathlib.Path,
    *,
    preset: str = "default",
    config: str | pathlib.Path | None = None,
    replace_pairs: Iterable[tuple[str, str]] | None = None,
    simplify: bool = True,
) -> GenerateResult:
    normal = _normal_mode(mode)
    input_path = pathlib.Path(input_urdf)
    output_path = pathlib.Path(output_urdf)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    pairs = [(str(a), str(b)) for a, b in (replace_pairs or [])]
    config_path = pathlib.Path(config) if config else resolve_preset(normal, preset)

    if normal == "capsule":
        message = _ext.capsuleized(str(input_path), str(output_path), str(config_path), pairs)
        json_path = _sidecar_json(output_path)
        primitive_count = _count_json_primitives(json_path, "capsules")
    elif normal == "sphere":
        message = _ext.spherized(str(input_path), str(output_path), str(config_path), pairs, bool(simplify))
        json_path = _sidecar_json(output_path)
        primitive_count = _count_json_primitives(json_path, "spheres")
    else:
        message = _ext.convex(str(input_path), str(output_path), pairs)
        json_path = None
        primitive_count = 0

    return GenerateResult(
        mode=normal,
        input_urdf=input_path,
        output_urdf=output_path,
        json_path=json_path if json_path and json_path.exists() else json_path,
        config_path=config_path,
        message=message,
        primitive_count=primitive_count,
    )


def generate_all(
    input_urdf: str | pathlib.Path,
    output_dir: str | pathlib.Path,
    *,
    modes: Sequence[str] = ("convex", "sphere", "capsule"),
    preset: str = "default",
    replace_pairs: Iterable[tuple[str, str]] | None = None,
    simplify: bool = True,
) -> list[GenerateResult]:
    input_path = pathlib.Path(input_urdf)
    out_dir = pathlib.Path(output_dir)
    stem = input_path.stem
    results: list[GenerateResult] = []
    for mode in modes:
        normal = _normal_mode(mode)
        suffix = "spherized" if normal == "sphere" else normal
        out = out_dir / f"{stem}_{suffix}.urdf"
        results.append(
            generate(
                normal,
                input_path,
                out,
                preset=preset,
                replace_pairs=replace_pairs,
                simplify=simplify,
            )
        )
    return results
```

- [ ] **Step 4: Export the API and preserve compatibility imports**

Replace `python/urdf_approx_geom/__init__.py` with:

```python
"""Public Python API for URDF approximate collision geometry generation."""

from __future__ import annotations

from ._extension import load_extension
from .api import GenerateResult, generate, generate_all

_ext = load_extension()

capsuleized = _ext.capsuleized
convex = _ext.convex
spherized = _ext.spherized

__all__ = [
    "GenerateResult",
    "capsuleized",
    "convex",
    "generate",
    "generate_all",
    "spherized",
]
```

Replace `python/urdf_approx_geom_cli.py` with:

```python
"""Backward-compatible module wrapper.

New code should import from ``urdf_approx_geom`` or run
``python -m urdf_approx_geom.cli``.
"""

from __future__ import annotations

from urdf_approx_geom import GenerateResult, capsuleized, convex, generate, generate_all, spherized
from urdf_approx_geom.cli import main

__all__ = [
    "GenerateResult",
    "capsuleized",
    "convex",
    "generate",
    "generate_all",
    "main",
    "spherized",
]


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 5: Run the API tests**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build --target _urdf_approx_geom -j$(nproc) && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_api_contract.py -q'
```

Expected: PASS. If Python still crashes inside `capsuleized()`, stop and fix the pybind/generator lifetime bug before continuing; the public API cannot be shipped over a crashing extension.

- [ ] **Step 6: Commit**

```bash
git add python/urdf_approx_geom python/urdf_approx_geom_cli.py python/tests/test_api_contract.py
git commit -m "feat: add structured python generation api"
```

---

### Task 3: Add Named Presets for Convex, Sphere, and Capsule

**Files:**
- Create: `python/urdf_approx_geom/presets.py`
- Create: `config/capsule/single.yml`
- Create: `config/capsule/default.yml`
- Create: `config/capsule/high_detail.yml`
- Modify: `config/capsule/capsuleConfig.yml`
- Modify: `config/capsule/capsuleConfig_tight.yml`
- Create: `config/sphereTree/single.yml`
- Create: `config/sphereTree/default.yml`
- Modify: `config/sphereTree/sphereTreeConfig.yml`
- Create: `config/convex/default.yml`
- Test: `python/tests/test_presets.py`

- [ ] **Step 1: Write preset resolution tests**

Create `python/tests/test_presets.py`:

```python
import pathlib

import pytest

from urdf_approx_geom.presets import available_presets, resolve_preset


def test_resolve_capsule_presets():
    assert resolve_preset("capsule", "single").name == "single.yml"
    assert resolve_preset("capsule", "default").name == "default.yml"
    assert resolve_preset("capsule", "high_detail").name == "high_detail.yml"


def test_resolve_sphere_presets():
    assert resolve_preset("sphere", "single").name == "single.yml"
    assert resolve_preset("sphere", "default").name == "default.yml"


def test_convex_default_preset_exists_for_uniform_cli():
    path = resolve_preset("convex", "default")
    assert path.name == "default.yml"
    assert path.exists()


def test_unknown_preset_error_lists_choices():
    with pytest.raises(ValueError, match="available presets"):
        resolve_preset("capsule", "maximum")


def test_available_presets_are_paths():
    presets = available_presets("capsule")
    assert isinstance(presets["default"], pathlib.Path)
    assert presets["default"].exists()
```

- [ ] **Step 2: Run the preset tests and verify they fail**

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_presets.py -q'
```

Expected: FAIL because `urdf_approx_geom.presets` and the new config files do not exist.

- [ ] **Step 3: Add the preset resolver**

Create `python/urdf_approx_geom/presets.py`:

```python
"""Named configuration presets for public generation modes."""

from __future__ import annotations

import pathlib


def repo_root() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[2]


_PRESETS = {
    "capsule": {
        "single": "config/capsule/single.yml",
        "default": "config/capsule/default.yml",
        "high_detail": "config/capsule/high_detail.yml",
        "tight": "config/capsule/high_detail.yml",
    },
    "sphere": {
        "single": "config/sphereTree/single.yml",
        "default": "config/sphereTree/default.yml",
    },
    "convex": {
        "default": "config/convex/default.yml",
    },
}


def _normal_mode(mode: str) -> str:
    value = mode.lower().replace("_", "-")
    if value in {"capsule", "capsules", "capsuleized"}:
        return "capsule"
    if value in {"sphere", "spheres", "sphere-tree", "spherized"}:
        return "sphere"
    if value in {"convex", "convex-mesh", "convex_mesh"}:
        return "convex"
    raise ValueError(f"unknown mode {mode!r}; expected convex, sphere, or capsule")


def available_presets(mode: str) -> dict[str, pathlib.Path]:
    normal = _normal_mode(mode)
    root = repo_root()
    return {name: root / rel for name, rel in _PRESETS[normal].items()}


def resolve_preset(mode: str, preset: str = "default") -> pathlib.Path:
    normal = _normal_mode(mode)
    presets = available_presets(normal)
    if preset not in presets:
        choices = ", ".join(sorted(presets))
        raise ValueError(f"unknown {normal} preset {preset!r}; available presets: {choices}")
    path = presets[preset]
    if not path.exists():
        raise FileNotFoundError(f"{normal} preset {preset!r} points to missing file: {path}")
    return path
```

- [ ] **Step 4: Add capsule presets**

Create `config/capsule/single.yml`:

```yaml
# Single-capsule baseline: exactly one conservative capsule per mesh link.
NSections: 2
CoaThreshold: 0.0
MaxCirclesPerSection: 1
MaxCapsulesPerLink: 1
AdaptiveCircleCount: false
MaxRadiusBinRatio: -1.0
MaxCapVAabbRatio: -1.0
MinSplitVolumeImprovement: 1.0
```

Create `config/capsule/default.yml`:

```yaml
# Default capsule preset: conservative coverage with low primitive count.
NSections: 4
CoaThreshold: 0.005
MaxCirclesPerSection: 1
MaxCapsulesPerLink: 12
AdaptiveCircleCount: false
MaxRadiusBinRatio: 1.45
MaxCapVAabbRatio: -1.0
MinSplitVolumeImprovement: 0.005
```

Create `config/capsule/high_detail.yml`:

```yaml
# Higher-detail capsule preset: more axial sections and a larger capsule budget.
# This preset is not allowed to pass by merely increasing count; validation uses
# absolute coverage and tightness ceilings.
NSections: 6
CoaThreshold: 0.005
MaxCirclesPerSection: 1
MaxCapsulesPerLink: 16
AdaptiveCircleCount: false
MaxRadiusBinRatio: 1.45
MaxCapVAabbRatio: 2.25
MinSplitVolumeImprovement: 0.005
```

Replace `config/capsule/capsuleConfig.yml` with the same content as `config/capsule/default.yml` and this first comment:

```yaml
# Compatibility alias for config/capsule/default.yml.
```

Replace `config/capsule/capsuleConfig_tight.yml` with the same content as `config/capsule/high_detail.yml` and this first comment:

```yaml
# Compatibility alias for config/capsule/high_detail.yml.
```

- [ ] **Step 5: Add sphere and convex presets**

Create `config/sphereTree/single.yml`:

```yaml
# Single-sphere baseline: one conservative sphere per mesh link.
SingleSphere: true
SimplifyRatio: 1.0
```

Create `config/sphereTree/default.yml`:

```yaml
# Default sphere-tree preset.
SingleSphere: false
Method: 2
Grid:
  TesterLevers: -1
  Branch: 8
  Depth: 1
  NumCoverPts: 10000
  MinCoverPts: 5
  Verify: false
  Nopause: false
  Eval: false
Medial:
  TesterLevers: -1
  Branch: 8
  Depth: 1
  NumCoverPts: 20000
  MinCoverPts: 5
  InitSpheres: 1000
  ErFact: 2
  SpheresPerNode: 1000
  Verify: false
  Nopause: true
  Eval: false
  UseMerge: true
  UseBurst: false
  UseExpand: false
  Optimise: 1
  BalExcess: 0.0
  MaxOptLevel: 1
Spawn:
  TesterLevers: -1
  Branch: 8
  Depth: 1
  NumCoverPts: 5000
  MinCoverPts: 5
  Verify: false
  Nopause: false
  Eval: false
Octree:
  Depth: 1
  Verify: false
  Nopause: false
  Eval: false
SimplifyRatio: 0.02
```

Replace `config/sphereTree/sphereTreeConfig.yml` with the same content as `config/sphereTree/default.yml` and this first comment:

```yaml
# Compatibility alias for config/sphereTree/default.yml.
```

Create `config/convex/default.yml`:

```yaml
# Convex mode currently has no tunable runtime parameters.
# The file exists so the public CLI can expose a uniform preset contract.
Mode: convex
```

- [ ] **Step 6: Run preset tests**

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_presets.py -q'
```

Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add config/capsule config/sphereTree config/convex python/urdf_approx_geom/presets.py python/tests/test_presets.py
git commit -m "config: add named approximation presets"
```

---

### Task 4: Support Sphere Config Paths and Single-Sphere Baseline

**Files:**
- Modify: `interface/urdf_approx_geom.cpp`
- Modify: `app/Sphereized.cpp`
- Modify: `include/SphereTreeURDFGenerator.h`
- Modify: `src/SphereTreeURDFGenerator.cpp`
- Test: `python/tests/test_sphere_single_preset.py`
- Test: `test/test_spheretree.cpp`

- [ ] **Step 1: Write Python regression for single-sphere mode**

Create `python/tests/test_sphere_single_preset.py`:

```python
import json

from urdf_approx_geom import generate


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_single_sphere_preset_emits_one_sphere_per_link(tmp_path):
    out = tmp_path / "fr3_single_sphere.urdf"
    result = generate("sphere", FR3_URDF, out, preset="single", simplify=False)
    assert result.mode == "sphere"
    assert result.json_path is not None
    data = json.loads(result.json_path.read_text())
    assert data
    assert all(len(body.get("spheres", [])) == 1 for body in data.values())
```

- [ ] **Step 2: Add a C++ unit test for the config flag**

In `test/test_spheretree.cpp`, add:

```cpp
#include <yaml-cpp/yaml.h>

TEST(SphereTreeConfig, SingleSpherePresetHasExplicitFlag) {
    YAML::Node config = YAML::LoadFile("/workspace/config/sphereTree/single.yml");
    ASSERT_TRUE(config["SingleSphere"]);
    EXPECT_TRUE(config["SingleSphere"].as<bool>());
}
```

- [ ] **Step 3: Run tests and verify failure before implementation**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_spheretree --gtest_color=no --gtest_filter=SphereTreeConfig.SingleSpherePresetHasExplicitFlag && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_sphere_single_preset.py -q'
```

Expected: C++ config test passes after Task 3. Python test fails because pybind `spherized()` does not accept a config path and C++ generator does not branch on `SingleSphere`.

- [ ] **Step 4: Extend pybind sphere signature**

In `interface/urdf_approx_geom.cpp`, replace the `spherized` binding with:

```cpp
    m.def(
        "spherized",
        [](const std::string& input, const std::string& output, const std::string& config,
           replace_pairs_t replace_pairs, bool simplify) {
            std::string cfg = config.empty()
                                  ? std::string(URDFApproxGeom_CONFIG_PATH) + "/sphereTree/sphereTreeConfig.yml"
                                  : config;
            SphereTreeURDFGenerator g(cfg, simplify);
            return g.run(input, output, replace_pairs).message();
        },
        py::arg("input"), py::arg("output"), py::arg("config") = std::string(""),
        py::arg("replace_pairs") = replace_pairs_t{}, py::arg("simplify") = true);
```

- [ ] **Step 5: Add `--config` support to the sphere CLI**

In `app/Sphereized.cpp`, add:

```cpp
    std::string sphereConfig;
```

next to `outputPath`, and add this parser branch:

```cpp
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            sphereConfig = argv[++i];
```

before the unknown-argument branch. Before constructing `SphereTreeURDFGenerator`, add:

```cpp
    if (sphereConfig.empty()) {
        sphereConfig = configPath + "/sphereTree/sphereTreeConfig.yml";
    }
```

Replace the constructor call with:

```cpp
    auto spherized_generator = std::make_shared<SphereTreeURDFGenerator>(sphereConfig, simplify);
```

Update the usage string to:

```cpp
        IRMV_ERROR("Usage: {} -i <input_urdf_path> -o <output_urdf_path> [-r <key> <value> ...] [-c <sphere_config.yml>] [--simplify <0|1>]", argv[0]);
```

- [ ] **Step 6: Add single-sphere branch in the C++ generator**

In `include/SphereTreeURDFGenerator.h`, add this protected method declaration:

```cpp
    irmv_core::bot_common::ErrorInfo buildSingleSphereModel(
        const std::string& urdf_path,
        const std::vector<std::pair<std::string, std::string>>& replace_pairs);
```

In `src/SphereTreeURDFGenerator.cpp`, in the constructor or `run()` path where YAML is loaded, read:

```cpp
bool single_sphere = false;
YAML::Node config = YAML::LoadFile(config_path_);
if (config["SingleSphere"]) {
    single_sphere = config["SingleSphere"].as<bool>();
}
```

At the start of `SphereTreeURDFGenerator::run`, before `buildSphereModel`, branch:

```cpp
    YAML::Node config = YAML::LoadFile(config_path_);
    if (config["SingleSphere"] && config["SingleSphere"].as<bool>()) {
        auto ret = buildSingleSphereModel(urdf_path, replace_pairs);
        if (!ret.isOk()) {
            return ret;
        }
        return writeURDF(output_path);
    }
```

Implement `buildSingleSphereModel()` by following the same URDF loading, mesh loading, collision-origin transform, model mutation, and `spheres_json_` write path used by `buildSphereModel()`, but compute each sphere as:

```cpp
Eigen::Vector3d center = V.colwise().mean();
double radius = 0.0;
for (int i = 0; i < V.rows(); ++i) {
    radius = std::max(radius, (V.row(i).transpose() - center).norm());
}
```

Use the existing sphere collision element writing pattern from `buildSphereModel()` so the generated URDF contains one `<sphere>` collision per mesh link and the JSON sidecar stores:

```json
{"spheres": [{"center": [x, y, z], "radius": r}]}
```

- [ ] **Step 7: Run sphere tests**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake -B build -DCOMPILE_URDFApproxGeom_PYBINDING=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc) && ./build/test/test_spheretree --gtest_color=no --gtest_filter=SphereTreeConfig.SingleSpherePresetHasExplicitFlag && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_sphere_single_preset.py -q'
```

Expected: PASS.

- [ ] **Step 8: Commit**

```bash
git add interface/urdf_approx_geom.cpp app/Sphereized.cpp include/SphereTreeURDFGenerator.h src/SphereTreeURDFGenerator.cpp test/test_spheretree.cpp python/tests/test_sphere_single_preset.py
git commit -m "feat: support configurable sphere presets"
```

---

### Task 5: Add Unified CLI Commands

**Files:**
- Create: `python/urdf_approx_geom/cli.py`
- Modify: `python/pyproject.toml`
- Modify: `python/urdf_approx_geom_cli.py`
- Test: `python/tests/test_cli.py`

- [ ] **Step 1: Replace CLI tests with subcommand contract**

Replace `python/tests/test_cli.py` with:

```python
import subprocess
import sys


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def run_cli(*args):
    return subprocess.run(
        [sys.executable, "-m", "urdf_approx_geom.cli", *args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def test_generate_capsule_cli(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    proc = run_cli("generate", "--mode", "capsule", "-i", FR3_URDF, "-o", str(out), "--preset", "default")
    assert proc.returncode == 0, proc.stdout
    assert out.exists()
    assert out.with_suffix(".json").exists()
    assert "capsule" in proc.stdout


def test_generate_all_cli(tmp_path):
    proc = run_cli("generate", "--mode", "all", "-i", FR3_URDF, "--output-dir", str(tmp_path), "--preset", "default")
    assert proc.returncode == 0, proc.stdout
    assert (tmp_path / "fr3_convex.urdf").exists()
    assert (tmp_path / "fr3_spherized.urdf").exists()
    assert (tmp_path / "fr3_capsule.urdf").exists()


def test_list_presets_cli():
    proc = run_cli("presets")
    assert proc.returncode == 0, proc.stdout
    assert "capsule: default" in proc.stdout
    assert "sphere: single" in proc.stdout
```

- [ ] **Step 2: Run CLI tests and verify failure**

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_cli.py -q'
```

Expected: FAIL because `urdf_approx_geom.cli` does not exist.

- [ ] **Step 3: Add CLI implementation**

Create `python/urdf_approx_geom/cli.py`:

```python
"""Command line interface for URDF approximate collision geometry."""

from __future__ import annotations

import argparse
import pathlib

from .api import generate, generate_all
from .presets import available_presets


def _add_replace_arg(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "-r",
        "--replace",
        nargs=2,
        action="append",
        default=[],
        metavar=("KEY", "VALUE"),
        help="mesh filename replacement pair; repeat for multiple replacements",
    )


def _print_result(result) -> None:
    json_text = f", json={result.json_path}" if result.json_path else ""
    cfg_text = f", config={result.config_path}" if result.config_path else ""
    print(
        f"{result.mode}: output={result.output_urdf}{json_text}{cfg_text}, "
        f"primitives={result.primitive_count}, message={result.message}"
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="urdf-approx-geom",
        description="Generate convex, sphere, or capsule collision approximations from a mesh URDF.",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    gen = sub.add_parser("generate", help="generate one mode or all modes")
    gen.add_argument("--mode", required=True, choices=["convex", "sphere", "spherized", "capsule", "all"])
    gen.add_argument("-i", "--input", required=True, help="input mesh URDF")
    gen.add_argument("-o", "--output", help="output URDF for a single mode")
    gen.add_argument("--output-dir", help="output directory when --mode all is used")
    gen.add_argument("--preset", default="default", help="named preset for sphere/capsule")
    gen.add_argument("--config", default=None, help="explicit config path for a single mode")
    gen.add_argument("--simplify", type=int, default=1, help="sphere mode mesh simplification flag 0/1")
    _add_replace_arg(gen)

    sub.add_parser("presets", help="list built-in named presets")

    val = sub.add_parser("validate", help="validate generated output metrics")
    val.add_argument("--mode", required=True, choices=["capsule", "sphere"])
    val.add_argument("--json", required=True, help="generated JSON sidecar")
    val.add_argument("--urdf", default="resources/fr3/urdf/fr3.urdf", help="source URDF for mesh metrics")
    val.add_argument("--max-capv-aabb", type=float, default=2.50)
    val.add_argument("--max-r-binmed", type=float, default=1.45)

    cmp_parser = sub.add_parser("compare", help="compare two generated JSON sidecars")
    cmp_parser.add_argument("--mode", required=True, choices=["capsule"])
    cmp_parser.add_argument("--baseline-json", required=True)
    cmp_parser.add_argument("--candidate-json", required=True)
    cmp_parser.add_argument("--max-capv-aabb", type=float, default=2.50)
    cmp_parser.add_argument("--max-r-binmed", type=float, default=1.45)

    viz = sub.add_parser("visualize", help="visualize generated geometry")
    viz.add_argument("--mode", required=True, choices=["capsule"])
    viz.add_argument("--urdf", required=True)
    viz.add_argument("--json", required=True)
    viz.add_argument("--png", default="")
    viz.add_argument("--mjcf", default="")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.command == "presets":
        for mode in ("convex", "sphere", "capsule"):
            names = sorted(available_presets(mode))
            print(f"{mode}: {', '.join(names)}")
        return 0

    if args.command == "generate":
        if args.mode == "all":
            if not args.output_dir:
                parser.error("--output-dir is required when --mode all is used")
            results = generate_all(
                args.input,
                args.output_dir,
                preset=args.preset,
                replace_pairs=args.replace,
                simplify=bool(args.simplify),
            )
            for result in results:
                _print_result(result)
            return 0
        if not args.output:
            parser.error("-o/--output is required for single-mode generation")
        result = generate(
            args.mode,
            args.input,
            args.output,
            preset=args.preset,
            config=args.config,
            replace_pairs=args.replace,
            simplify=bool(args.simplify),
        )
        _print_result(result)
        return 0

    if args.command == "validate":
        from scripts.urdf_approx_validate import validate_capsule_file

        if args.mode == "capsule":
            return validate_capsule_file(args.json, args.urdf, args.max_capv_aabb, args.max_r_binmed)
        parser.error("sphere validation is not implemented in this release")

    if args.command == "compare":
        from scripts.urdf_approx_validate import compare_capsule_files

        return compare_capsule_files(
            args.baseline_json,
            args.candidate_json,
            args.max_capv_aabb,
            args.max_r_binmed,
        )

    if args.command == "visualize":
        from scripts.urdf_approx_visualize import visualize_capsules

        visualize_capsules(args.urdf, args.json, png=args.png, mjcf=args.mjcf)
        return 0

    parser.error(f"unknown command {args.command}")


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 4: Update package metadata**

Replace `python/pyproject.toml` with:

```toml
[project]
name = "urdf-approx-geom"
version = "1.5.0"
description = "URDF collision-geometry approximator for convex mesh, sphere, and capsule outputs"
requires-python = ">=3.10"
authors = [{ name = "YX.E.Z", email = "yixuanzhou@sjtu.edu.cn" }]

[project.scripts]
urdf-approx-geom = "urdf_approx_geom.cli:main"

[tool.pytest.ini_options]
testpaths = ["tests"]
pythonpath = ["."]

[tool.urdf-approx-geom]
extension_build_path = "build/python"
```

- [ ] **Step 5: Run CLI tests**

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_cli.py -q'
```

Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add python/urdf_approx_geom/cli.py python/pyproject.toml python/urdf_approx_geom_cli.py python/tests/test_cli.py
git commit -m "feat: add unified generation cli"
```

---

### Task 6: Productize Validation and Comparison

**Files:**
- Create: `scripts/urdf_approx_validate.py`
- Modify: `scripts/check_capsule_tightness.py`
- Modify: `scripts/compare_capsule_presets.py`
- Test: `python/tests/test_validation_cli.py`

- [ ] **Step 1: Write validation CLI tests**

Create `python/tests/test_validation_cli.py`:

```python
import subprocess
import sys

from urdf_approx_geom import generate


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def run_cli(*args):
    return subprocess.run(
        [sys.executable, "-m", "urdf_approx_geom.cli", *args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def test_validate_capsule_cli_passes_default_preset(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    result = generate("capsule", FR3_URDF, out, preset="default")
    proc = run_cli(
        "validate",
        "--mode",
        "capsule",
        "--json",
        str(result.json_path),
        "--urdf",
        FR3_URDF,
        "--max-capv-aabb",
        "2.50",
        "--max-r-binmed",
        "1.50",
    )
    assert proc.returncode == 0, proc.stdout
    assert "all_covered" in proc.stdout


def test_compare_capsule_cli_uses_absolute_candidate_contract(tmp_path):
    out_a = tmp_path / "a.urdf"
    out_b = tmp_path / "b.urdf"
    baseline = generate("capsule", FR3_URDF, out_a, preset="single")
    candidate = generate("capsule", FR3_URDF, out_b, preset="default")
    proc = run_cli(
        "compare",
        "--mode",
        "capsule",
        "--baseline-json",
        str(baseline.json_path),
        "--candidate-json",
        str(candidate.json_path),
        "--max-capv-aabb",
        "2.50",
        "--max-r-binmed",
        "1.50",
    )
    assert proc.returncode == 0, proc.stdout
    assert "candidate" in proc.stdout
```

- [ ] **Step 2: Run validation tests and verify failure**

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_validation_cli.py -q'
```

Expected: FAIL because `scripts/urdf_approx_validate.py` does not exist and comparison still encodes sparse/tight relative assumptions.

- [ ] **Step 3: Add shared validation implementation**

Create `scripts/urdf_approx_validate.py`:

```python
#!/usr/bin/env python3
"""Shared validation helpers for generated approximation outputs."""

from __future__ import annotations

import json
import subprocess
import sys


def _capsule_metrics(caps_json: str, urdf: str = "resources/fr3/urdf/fr3.urdf") -> dict:
    proc = subprocess.run(
        [
            sys.executable,
            "scripts/check_capsule_coverage.py",
            "--caps-json",
            caps_json,
            "--urdf",
            urdf,
            "--json",
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if proc.returncode != 0:
        print(proc.stdout)
        raise SystemExit(proc.returncode)
    return json.loads(proc.stdout)


def _worst(metrics: dict, key: str) -> float:
    rows = metrics.get("links", [])
    return max((float(row[key]) for row in rows), default=0.0)


def _count(metrics: dict) -> int:
    return sum(int(row["capsules"]) for row in metrics.get("links", []))


def validate_capsule_metrics(metrics: dict, max_capv_aabb: float, max_r_binmed: float) -> list[str]:
    failures: list[str] = []
    if not metrics.get("all_covered", False):
        failures.append("not all links are covered")
    for row in metrics.get("links", []):
        if float(row["capV_aabb"]) > max_capv_aabb:
            failures.append(f"{row['link']}: capV/aabb {row['capV_aabb']:.2f} > {max_capv_aabb:.2f}")
        if float(row["r_binMed"]) > max_r_binmed:
            failures.append(f"{row['link']}: r/binMed {row['r_binMed']:.2f} > {max_r_binmed:.2f}")
    return failures


def validate_capsule_file(caps_json: str, urdf: str, max_capv_aabb: float, max_r_binmed: float) -> int:
    metrics = _capsule_metrics(caps_json, urdf)
    print(json.dumps(metrics, indent=2, sort_keys=True))
    failures = validate_capsule_metrics(metrics, max_capv_aabb, max_r_binmed)
    if failures:
        print("validation failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


def compare_capsule_files(
    baseline_json: str,
    candidate_json: str,
    max_capv_aabb: float,
    max_r_binmed: float,
) -> int:
    baseline = _capsule_metrics(baseline_json)
    candidate = _capsule_metrics(candidate_json)
    summary = {
        "baseline_count": _count(baseline),
        "candidate_count": _count(candidate),
        "baseline_worst_capV_aabb": _worst(baseline, "capV_aabb"),
        "candidate_worst_capV_aabb": _worst(candidate, "capV_aabb"),
        "baseline_worst_r_binMed": _worst(baseline, "r_binMed"),
        "candidate_worst_r_binMed": _worst(candidate, "r_binMed"),
    }
    print(json.dumps(summary, indent=2, sort_keys=True))
    failures = validate_capsule_metrics(candidate, max_capv_aabb, max_r_binmed)
    if failures:
        print("candidate validation failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0
```

- [ ] **Step 4: Convert legacy scripts to wrappers**

In `scripts/compare_capsule_presets.py`, replace the relative checks:

```python
    if tight_count <= sparse_count:
        print("tight preset did not add detail", file=sys.stderr)
        return 1
    if tight_capv > sparse_capv:
        print("tight preset worsened worst capV/aabb", file=sys.stderr)
        return 1
    if tight_ratio > sparse_ratio:
        print("tight preset worsened worst r/binMed", file=sys.stderr)
        return 1
```

with:

```python
    print("comparison is informational; pass/fail uses absolute tight preset ceilings")
```

Keep the existing absolute ceiling checks.

Keep `scripts/check_capsule_tightness.py` as the detailed per-link gate; do not remove its per-link options.

- [ ] **Step 5: Run validation tests and legacy scripts**

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_validation_cli.py -q && ./build/app/capsuleized -i resources/fr3/urdf/fr3.urdf -o /tmp/fr3_capsule_default.urdf --config config/capsule/default.yml && python3 scripts/check_capsule_tightness.py --caps-json /tmp/fr3_capsule_default.json --urdf resources/fr3/urdf/fr3.urdf --max-capv-aabb 2.50 --max-r-binmed 1.50'
```

Expected: PASS and exit code 0.

- [ ] **Step 6: Commit**

```bash
git add scripts/urdf_approx_validate.py scripts/compare_capsule_presets.py scripts/check_capsule_tightness.py python/tests/test_validation_cli.py
git commit -m "feat: add product validation contract"
```

---

### Task 7: Generalize Visualization Entry Points

**Files:**
- Create: `scripts/urdf_approx_visualize.py`
- Modify: `scripts/viz_capsules.py`
- Modify: `scripts/make_mjcf.py`
- Test: `python/tests/test_visualize_cli.py`

- [ ] **Step 1: Write headless visualization tests**

Create `python/tests/test_visualize_cli.py`:

```python
import subprocess
import sys

from urdf_approx_geom import generate


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_visualize_capsule_mjcf_cli(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    result = generate("capsule", FR3_URDF, out, preset="default")
    mjcf = tmp_path / "fr3_capsules.xml"
    proc = subprocess.run(
        [
            sys.executable,
            "-m",
            "urdf_approx_geom.cli",
            "visualize",
            "--mode",
            "capsule",
            "--urdf",
            FR3_URDF,
            "--json",
            str(result.json_path),
            "--mjcf",
            str(mjcf),
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert mjcf.exists()
    assert "<mujoco" in mjcf.read_text()
```

- [ ] **Step 2: Run the test and verify failure**

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_visualize_cli.py -q'
```

Expected: FAIL because `scripts/urdf_approx_visualize.py` does not exist.

- [ ] **Step 3: Add shared visualization module**

Create `scripts/urdf_approx_visualize.py`:

```python
#!/usr/bin/env python3
"""Shared visualization helpers for generated URDF approximation outputs."""

from __future__ import annotations

import os
import pathlib


def visualize_capsules(urdf: str, caps_json: str, *, png: str = "", mjcf: str = "") -> None:
    if mjcf:
        from scripts.make_mjcf import write_capsule_mjcf

        write_capsule_mjcf(urdf, caps_json, mjcf)
        print(f"wrote {mjcf}")
        return
    from scripts.viz_capsules import render_capsule_overlay

    render_capsule_overlay(urdf, caps_json, png=png)
    if png:
        print(f"rendered {png}")
    else:
        print("opened capsule overlay viewer")


def default_output_path(path: str, suffix: str) -> str:
    p = pathlib.Path(path)
    return str(p.with_suffix(suffix))
```

- [ ] **Step 4: Refactor `make_mjcf.py` into a callable function**

In `scripts/make_mjcf.py`, extract the current body of `main()` into:

```python
def write_capsule_mjcf(urdf_path=FR3_URDF, caps_json=CAPS_JSON, out_mjcf=OUT_MJCF):
    import pybullet as p

    caps = json.load(open(caps_json))
    urdf_txt = open(urdf_path).read().replace(
        "/workspace/resources/fr3", os.path.join(REPO, "resources/fr3"))
    tmp = "/tmp/urdf_approx_geom_host.urdf"
    open(tmp, "w").write(urdf_txt)

    # Keep the existing pybullet loading, MJCF asset creation, mesh context,
    # and capsule geom emission logic unchanged. Replace references to
    # FR3_URDF, CAPS_JSON, and OUT_MJCF inside the extracted body with the
    # function arguments urdf_path, caps_json, and out_mjcf.
```

Then make `main()` call:

```python
def main():
    write_capsule_mjcf(FR3_URDF, CAPS_JSON, OUT_MJCF)
```

The extracted function must still write:

```python
    tree.write(out_mjcf, encoding="utf-8", xml_declaration=True)
```

- [ ] **Step 5: Refactor `viz_capsules.py` into a callable function**

In `scripts/viz_capsules.py`, extract the current body of `main()` into:

```python
def render_capsule_overlay(urdf_path=FR3_URDF, caps_json=CAPS_JSON, *, png=""):
    import pybullet as p
    import pybullet_data

    caps = json.load(open(caps_json))
    urdf_txt = open(urdf_path).read()
    urdf_txt = urdf_txt.replace("/workspace/resources/fr3",
                                os.path.join(REPO, "resources/fr3"))
    tmp_urdf = "/tmp/urdf_approx_geom_host.urdf"
    open(tmp_urdf, "w").write(urdf_txt)

    # Keep the existing pybullet scene construction and capsule mesh overlay
    # logic unchanged. Replace args.png with the function argument png.
```

Then make `main()` parse arguments and call:

```python
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--urdf", default=FR3_URDF)
    ap.add_argument("--json", default=CAPS_JSON)
    ap.add_argument("--png", default="", help="if set, render to this PNG instead of GUI")
    args = ap.parse_args()
    render_capsule_overlay(args.urdf, args.json, png=args.png)
```

- [ ] **Step 6: Run visualization tests**

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_visualize_cli.py -q'
```

Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add scripts/urdf_approx_visualize.py scripts/viz_capsules.py scripts/make_mjcf.py python/tests/test_visualize_cli.py
git commit -m "feat: add reusable visualization entry points"
```

---

### Task 8: Rewrite User-Facing Documentation

**Files:**
- Modify: `README.md`
- Create: `docs/quickstart.md`
- Create: `docs/modes.md`
- Create: `docs/config-presets.md`
- Create: `docs/python-api.md`
- Create: `docs/output-schema.md`
- Create: `docs/visualization.md`
- Create: `docs/developer-guide.md`
- Modify: `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`

- [ ] **Step 1: Replace README with product-oriented structure**

Replace `README.md` with this structure:

```markdown
# URDFApproxGeom

URDFApproxGeom converts mesh-based URDF collision geometry into lighter collision approximations:

| Mode | Output | Best For |
|------|--------|----------|
| `convex` | convex collision meshes in URDF | tight mesh-based collision with low authoring effort |
| `sphere` | sphere primitives in URDF plus JSON sidecar | simulators and planners that prefer simple primitives |
| `capsule` | capsule parameters in JSON sidecar and URDF-compatible primitive representation | smooth low-count robot-link approximations |

## Quickstart

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCOMPILE_URDFApproxGeom_PYBINDING=ON
cmake --build build -j$(nproc)
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli generate --mode all -i resources/fr3/urdf/fr3.urdf --output-dir /tmp/fr3_approx
```

## Command Line

Generate all modes:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli generate --mode all -i robot.urdf --output-dir out
```

Generate one mode:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli generate --mode capsule -i robot.urdf -o out/robot_capsule.urdf --preset default
```

List presets:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli presets
```

## Python API

```python
from urdf_approx_geom import generate, generate_all

result = generate("capsule", "robot.urdf", "out/robot_capsule.urdf", preset="default")
print(result.json_path, result.primitive_count)

all_results = generate_all("robot.urdf", "out", modes=["convex", "sphere", "capsule"])
```

## Presets

Capsule presets:

- `single`: one conservative capsule per link.
- `default`: recommended low-count conservative capsule fit.
- `high_detail`: more axial sections and a larger capsule budget.

Sphere presets:

- `single`: one conservative sphere per link.
- `default`: sphere-tree fit.

Convex presets:

- `default`: convex hull collision mesh generation.

## Output Contracts

- `convex`: writes an output URDF whose collision meshes point to generated convex meshes.
- `sphere`: writes an output URDF and a JSON sidecar with per-link `spheres`.
- `capsule`: writes an output URDF and a JSON sidecar with per-link `capsules`.

Capsule JSON entries use link-frame sphere-center endpoints:

```json
{
  "link_name": {
    "capsules": [
      {"p0": [0, 0, 0], "p1": [0.1, 0, 0], "radius": 0.03}
    ]
  }
}
```

## Documentation

- `docs/quickstart.md`
- `docs/modes.md`
- `docs/config-presets.md`
- `docs/python-api.md`
- `docs/output-schema.md`
- `docs/visualization.md`
- `docs/developer-guide.md`

## Citation

```latex
@misc{ZHOU2024SpherizedURDF,
    title={{SpherizedURDF: An automatic C++ tool for generating spherized / convex version of collision geometry and write URDF automatically}},
    author={Zhou, Yixuan and Wang, Hesheng},
    year={2024},
    url={https://github.com/IRMV-Manipulation-Group/SpherizedURDFGenerator}
}
```
```

- [ ] **Step 2: Add quickstart document**

Create `docs/quickstart.md`:

```markdown
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
```

- [ ] **Step 3: Add focused docs**

Create `docs/modes.md`, `docs/config-presets.md`, `docs/python-api.md`, `docs/output-schema.md`, `docs/visualization.md`, and `docs/developer-guide.md` with concise sections matching the README links. Each file must include one runnable command or code snippet and one paragraph explaining the relevant output contract.

Use these exact headings:

```markdown
# Modes
# Configuration Presets
# Python API
# Output Schema
# Visualization
# Developer Guide
```

- [ ] **Step 4: Move handoff out of the user path**

At the top of `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`, add:

```markdown
> Historical engineering handoff. User-facing usage lives in `README.md` and `docs/`.
```

Update stale references in the handoff so it does not claim a failing sparse-vs-tight relative gate is required for release. Keep measured metrics as historical observations.

- [ ] **Step 5: Verify docs contain the public contract**

```bash
docker exec spherized-development bash -lc 'cd /workspace && grep -R "generate --mode all" README.md docs/quickstart.md && grep -R "single" docs/config-presets.md README.md && grep -R "p0" docs/output-schema.md README.md'
```

Expected: each `grep` prints at least one matching line.

- [ ] **Step 6: Commit**

```bash
git add README.md docs doc/handoffs/2026-07-02-capsule-geometry-handoff.md
git commit -m "docs: document public approximation workflow"
```

---

### Task 9: Add CI and Repository Hygiene

**Files:**
- Create: `.github/workflows/ci.yml`
- Modify: `.gitignore`

- [ ] **Step 1: Update `.gitignore`**

Append to `.gitignore`:

```gitignore

# CMake build trees
build/
build_dkr/

# Python caches
__pycache__/
*.py[cod]
.pytest_cache/
python/__pycache__/
python/tests/__pycache__/
scripts/__pycache__/

# Generated local approximation outputs
*_capsuleized.json
*_capsuleized.urdf
*_capsules.xml
```

Do not remove tracked example files in this task. If ignored files are already tracked, leave them tracked and handle cleanup in a separate explicit cleanup commit.

- [ ] **Step 2: Add CI workflow**

Create `.github/workflows/ci.yml`:

```yaml
name: CI

on:
  pull_request:
  push:
    branches: [main, master]

jobs:
  build-test:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install system dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake g++ python3 python3-pip libeigen3-dev libyaml-cpp-dev liburdfdom-dev libtinyxml2-dev libgmp-dev libgtest-dev

      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release -DCOMPILE_URDFApproxGeom_PYBINDING=ON

      - name: Build
        run: cmake --build build -j2

      - name: C++ capsule tests
        run: ./build/test/test_capsule --gtest_color=no

      - name: Python tests
        run: PYTHONPATH=$PWD/python:$PWD/build/python python3 -m pytest python/tests -q
```

If CI cannot install `irmv_core` from public sources, replace the workflow with a Docker-based workflow that builds the project image from `docker/Dockerfile` and runs the same commands inside that image. Do not leave CI permanently red.

- [ ] **Step 3: Run local verification in container**

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake -B build -DCOMPILE_URDFApproxGeom_PYBINDING=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_color=no && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests -q && git diff --check'
```

Expected: C++ tests PASS, Python tests PASS, and `git diff --check` prints no whitespace errors.

- [ ] **Step 4: Commit**

```bash
git add .gitignore .github/workflows/ci.yml
git commit -m "ci: add productization verification workflow"
```

---

### Task 10: Final Release Readiness Pass

**Files:**
- Modify only files needed to fix verification failures from this task.

- [ ] **Step 1: Run the full product workflow**

```bash
docker exec spherized-development bash -lc 'cd /workspace && rm -rf /tmp/fr3_product_check && mkdir -p /tmp/fr3_product_check && cmake -B build -DCOMPILE_URDFApproxGeom_PYBINDING=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc) && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m urdf_approx_geom.cli generate --mode all -i resources/fr3/urdf/fr3.urdf --output-dir /tmp/fr3_product_check && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m urdf_approx_geom.cli validate --mode capsule --json /tmp/fr3_product_check/fr3_capsule.json --urdf resources/fr3/urdf/fr3.urdf --max-capv-aabb 2.50 --max-r-binmed 1.50 && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m urdf_approx_geom.cli visualize --mode capsule --urdf resources/fr3/urdf/fr3.urdf --json /tmp/fr3_product_check/fr3_capsule.json --mjcf /tmp/fr3_product_check/fr3_capsules.xml && test -f /tmp/fr3_product_check/fr3_convex.urdf && test -f /tmp/fr3_product_check/fr3_spherized.urdf && test -f /tmp/fr3_product_check/fr3_capsule.urdf && test -f /tmp/fr3_product_check/fr3_capsules.xml'
```

Expected: exit code 0 and all generated files exist.

- [ ] **Step 2: Run all tests**

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_capsule --gtest_color=no && ./build/test/test_spheretree --gtest_color=no && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests -q'
```

Expected: PASS. If Python crashes, inspect the failing stack and fix the extension/generator lifetime issue before making any release-readiness claim.

- [ ] **Step 3: Check repo cleanliness for generated files**

```bash
docker exec spherized-development bash -lc 'cd /workspace && git status --short -- README.md docs doc config python scripts app include src interface test .github .gitignore'
```

Expected: only intentional source/doc/config changes appear before the final commit.

- [ ] **Step 4: Commit any verification fixes**

If Task 10 required changes, commit them:

```bash
git add README.md docs doc config python scripts app include src interface test .github .gitignore
git commit -m "fix: complete productization verification"
```

If Task 10 required no changes, do not create an empty commit.

---

## Self-Review

Spec coverage:

- Public three-mode product contract: Tasks 2, 5, and 8.
- Python interface: Tasks 1, 2, and 5.
- Visualization tooling: Task 7.
- Reasonable config files, including single sphere and single capsule: Tasks 3 and 4.
- Architecture guidance: File Structure section and docs in Task 8.
- Validation/release confidence: Tasks 6, 9, and 10.
- License changes: intentionally excluded by user request.

Placeholder scan:

- The plan contains no placeholder markers.
- Code steps include exact file paths, commands, and expected results.
- The only extraction instructions are bounded to existing functions and specify required function names, arguments, and write targets.

Type consistency:

- Public API consistently uses `GenerateResult`, `generate()`, and `generate_all()`.
- Public modes are normalized to `convex`, `sphere`, and `capsule`.
- The private extension name is consistently `_urdf_approx_geom`.
- Presets consistently use `single`, `default`, and `high_detail` for capsule, and `single`, `default` for sphere.

## Execution Handoff

Plan complete. Recommended execution order is strict: Task 1, Task 3, Task 2, Task 4, Task 5, Task 6, Task 7, Task 8, Task 9, then Task 10. Task 10 must run last.
