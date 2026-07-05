# Productization Review Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix the review blockers in the productization branch so the repository cleanly supports the public contract: one mesh-based URDF input can generate convex, sphere, and capsule collision approximations through stable C++ and Python entry points.

**Architecture:** Keep the current productization structure: C++ generators remain the core, Python package/CLI remains the public layer, and legacy scripts remain compatibility wrappers. The fixes focus on repository hygiene, source-tree-independent CLI behavior, sphere JSON/API consistency, per-mode preset handling, reliable tests, and accurate documentation.

**Tech Stack:** C++17, CMake, pybind11, Python 3.10+, argparse, pytest, GoogleTest, yaml-cpp, Docker container `spherized-development`.

---

## Review Findings Covered

- Tracked build outputs, binary artifacts, Python caches, CodeGraph files, and temporary `check_json*.py` scripts must be removed from the commit.
- Python CLI `validate` and `visualize` must work outside the repository root.
- Default sphere output must match the documented `spheres` JSON schema, and Python `primitive_count` must be nonzero for default sphere generation.
- `generate_all(..., preset="single")` must be usable for the intended "single sphere + single capsule" workflow without breaking convex mode.
- `test_spheretree` must not fail or segfault on machines without the author's private mesh paths.
- CI must not hard-fail on open-source pull requests because private registry secrets are absent.
- Documentation must match the implemented API, schema, test state, and extension loading behavior.

All project commands must run inside the container:

```bash
docker exec spherized-development bash -lc 'cd /workspace && <command>'
```

## File Structure

- Modify `.gitignore`: ignore `.codegraph/`, build trees, caches, and temporary diagnostics.
- Remove from git index: `build/`, `build_dkr/`, `.codegraph/`, `python/__pycache__/`, `python/tests/__pycache__/`, `python/urdf_approx_geom/__pycache__/`, `scripts/__pycache__/`, `check_json.py`, `check_json2.py`.
- Modify `python/urdf_approx_geom/api.py`: count legacy and canonical sphere JSON, add per-mode preset resolution for `generate_all()`.
- Modify `python/urdf_approx_geom/cli.py`: use package modules instead of direct `scripts` imports; add per-mode preset flags.
- Create `python/urdf_approx_geom/validation.py`: repository-root-aware validation functions.
- Create `python/urdf_approx_geom/visualization.py`: repository-root-aware visualization functions.
- Modify `scripts/urdf_approx_validate.py`: compatibility wrapper that imports package validation.
- Modify `scripts/urdf_approx_visualize.py`: compatibility wrapper that imports package visualization.
- Modify `scripts/compare_capsule_presets.py`: keep compatibility behavior after validation module move.
- Modify `src/SphereTreeURDFGenerator.cpp`: emit canonical `spheres` array for default sphere-tree output while preserving legacy fields.
- Modify `test/test_spheretree.cpp`: skip unavailable external-resource tests instead of dereferencing empty vectors and crashing; keep single-sphere config test.
- Modify Python tests under `python/tests/`: add cwd-independent CLI tests, default sphere schema/count test, and per-mode preset tests.
- Modify `.github/workflows/ci.yml`: make full Docker build conditional on secrets and add an always-on lightweight source hygiene job.
- Modify docs: `README.md`, `docs/output-schema.md`, `docs/python-api.md`, `docs/developer-guide.md`, `docs/config-presets.md`, `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`.

---

### Task 1: Remove Generated Artifacts From The Commit

**Files:**
- Modify: `.gitignore`
- Remove from git index: `build/`
- Remove from git index: `build_dkr/`
- Remove from git index: `.codegraph/`
- Remove from git index: `python/__pycache__/`
- Remove from git index: `python/tests/__pycache__/`
- Remove from git index: `python/urdf_approx_geom/__pycache__/`
- Remove from git index: `scripts/__pycache__/`
- Delete: `check_json.py`
- Delete: `check_json2.py`

- [ ] **Step 1: Confirm the current bad tracked artifact set**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git ls-files build build_dkr .codegraph python/__pycache__ python/tests/__pycache__ python/urdf_approx_geom/__pycache__ scripts/__pycache__ check_json.py check_json2.py | sed -n "1,80p"'
```

Expected: prints tracked generated files, including paths under `build/` and the two `check_json*.py` files.

- [ ] **Step 2: Strengthen `.gitignore`**

Append these entries to `.gitignore` if they are not already present:

```gitignore

# Local CodeGraph index
.codegraph/

# CMake build trees
build/
build_dkr/

# Python caches
__pycache__/
*.py[cod]
.pytest_cache/
python/__pycache__/
python/tests/__pycache__/
python/urdf_approx_geom/__pycache__/
scripts/__pycache__/

# Local review/debug scripts
check_json.py
check_json2.py
```

- [ ] **Step 3: Remove generated files from git tracking**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git rm -r --cached build build_dkr .codegraph python/__pycache__ python/tests/__pycache__ python/urdf_approx_geom/__pycache__ scripts/__pycache__ && git rm check_json.py check_json2.py'
```

Expected: `git rm` prints removed paths. The local generated files may remain ignored after `--cached`; `check_json.py` and `check_json2.py` are removed from the working tree.

- [ ] **Step 4: Verify generated artifacts are no longer tracked**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git ls-files build build_dkr .codegraph python/__pycache__ python/tests/__pycache__ python/urdf_approx_geom/__pycache__ scripts/__pycache__ check_json.py check_json2.py'
```

Expected: no output.

- [ ] **Step 5: Verify whitespace check no longer scans build artifacts**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git diff --check e4323e5..HEAD'
```

Expected: no output and exit code 0. If it still reports whitespace under `build/`, the build tree is still tracked and Step 3 was incomplete.

- [ ] **Step 6: Commit**

```bash
git add .gitignore
git commit -m "chore: remove generated artifacts from productization branch"
```

---

### Task 2: Make Validation And Visualization CLI Cwd-Independent

**Files:**
- Create: `python/urdf_approx_geom/validation.py`
- Create: `python/urdf_approx_geom/visualization.py`
- Modify: `python/urdf_approx_geom/cli.py`
- Modify: `scripts/urdf_approx_validate.py`
- Modify: `scripts/urdf_approx_visualize.py`
- Test: `python/tests/test_cli_cwd_independent.py`

- [ ] **Step 1: Add a failing cwd-independent CLI test**

Create `python/tests/test_cli_cwd_independent.py`:

```python
from __future__ import annotations

import os
import subprocess
import sys

from urdf_approx_geom import generate


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def _env():
    env = os.environ.copy()
    env["PYTHONPATH"] = "/workspace/python:/workspace/build/python"
    return env


def test_validate_command_works_outside_repo_root(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    result = generate("capsule", FR3_URDF, out, preset="default")

    proc = subprocess.run(
        [
            sys.executable,
            "-m",
            "urdf_approx_geom.cli",
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
        ],
        cwd="/tmp",
        env=_env(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert '"all_covered": true' in proc.stdout


def test_visualize_mjcf_command_works_outside_repo_root(tmp_path):
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
        cwd="/tmp",
        env=_env(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert mjcf.exists()
    assert "<mujoco" in mjcf.read_text()
```

- [ ] **Step 2: Run the new test and verify failure**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_cli_cwd_independent.py -q'
```

Expected before implementation: FAIL with `ModuleNotFoundError: No module named 'scripts'`.

- [ ] **Step 3: Add package validation module**

Create `python/urdf_approx_geom/validation.py`:

```python
"""Validation helpers used by the public CLI and legacy scripts."""

from __future__ import annotations

import json
import pathlib
import subprocess
import sys


def repo_root() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[2]


def _capsule_metrics(caps_json: str, urdf: str = "resources/fr3/urdf/fr3.urdf") -> dict:
    root = repo_root()
    script = root / "scripts" / "check_capsule_coverage.py"
    urdf_path = pathlib.Path(urdf)
    if not urdf_path.is_absolute():
        urdf_path = root / urdf_path
    proc = subprocess.run(
        [
            sys.executable,
            str(script),
            "--caps-json",
            str(caps_json),
            "--urdf",
            str(urdf_path),
            "--json",
        ],
        cwd=str(root),
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
    return max((float(row[key]) for row in metrics.get("links", [])), default=0.0)


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

- [ ] **Step 4: Add package visualization module**

Create `python/urdf_approx_geom/visualization.py`:

```python
"""Visualization helpers used by the public CLI and legacy scripts."""

from __future__ import annotations

import importlib.util
import pathlib
import sys


def repo_root() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[2]


def _load_script_module(module_name: str, relative_path: str):
    root = repo_root()
    path = root / relative_path
    spec = importlib.util.spec_from_file_location(module_name, path)
    if spec is None or spec.loader is None:
        raise ImportError(f"cannot load {module_name} from {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module


def visualize_capsules(urdf: str, caps_json: str, *, png: str = "", mjcf: str = "") -> None:
    if mjcf:
        module = _load_script_module("_urdf_approx_make_mjcf", "scripts/make_mjcf.py")
        module.write_capsule_mjcf(urdf, caps_json, mjcf)
        print(f"wrote {mjcf}")
        return

    module = _load_script_module("_urdf_approx_viz_capsules", "scripts/viz_capsules.py")
    module.render_capsule_overlay(urdf, caps_json, png=png)
    if png:
        print(f"rendered {png}")
    else:
        print("opened capsule overlay viewer")
```

- [ ] **Step 5: Update CLI imports**

In `python/urdf_approx_geom/cli.py`, replace:

```python
        from scripts.urdf_approx_validate import validate_capsule_file
```

with:

```python
        from .validation import validate_capsule_file
```

Replace:

```python
        from scripts.urdf_approx_validate import compare_capsule_files
```

with:

```python
        from .validation import compare_capsule_files
```

Replace:

```python
        from scripts.urdf_approx_visualize import visualize_capsules
```

with:

```python
        from .visualization import visualize_capsules
```

- [ ] **Step 6: Turn script modules into compatibility wrappers**

Replace `scripts/urdf_approx_validate.py` with:

```python
#!/usr/bin/env python3
"""Compatibility wrapper for package validation helpers."""

from urdf_approx_geom.validation import (
    compare_capsule_files,
    validate_capsule_file,
    validate_capsule_metrics,
)

__all__ = ["compare_capsule_files", "validate_capsule_file", "validate_capsule_metrics"]
```

Replace `scripts/urdf_approx_visualize.py` with:

```python
#!/usr/bin/env python3
"""Compatibility wrapper for package visualization helpers."""

from urdf_approx_geom.visualization import visualize_capsules

__all__ = ["visualize_capsules"]
```

- [ ] **Step 7: Run tests**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_cli_cwd_independent.py python/tests/test_validation_cli.py python/tests/test_visualize_cli.py -q'
```

Expected: PASS.

- [ ] **Step 8: Commit**

```bash
git add python/urdf_approx_geom/cli.py python/urdf_approx_geom/validation.py python/urdf_approx_geom/visualization.py scripts/urdf_approx_validate.py scripts/urdf_approx_visualize.py python/tests/test_cli_cwd_independent.py
git commit -m "fix: make public cli independent of working directory"
```

---

### Task 3: Make Default Sphere JSON Match The Public Schema

**Files:**
- Modify: `src/SphereTreeURDFGenerator.cpp`
- Modify: `python/urdf_approx_geom/api.py`
- Test: `python/tests/test_sphere_default_schema.py`

- [ ] **Step 1: Add failing default sphere schema/count test**

Create `python/tests/test_sphere_default_schema.py`:

```python
from __future__ import annotations

import json

from urdf_approx_geom import generate


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_default_sphere_schema_has_canonical_spheres_array(tmp_path):
    out = tmp_path / "fr3_spherized.urdf"
    result = generate("sphere", FR3_URDF, out, preset="default")

    assert result.json_path is not None
    assert result.json_path.exists()
    assert result.primitive_count > 0

    data = json.loads(result.json_path.read_text())
    mesh_links = [body for body in data.values() if isinstance(body, dict) and body.get("spheres")]
    assert mesh_links
    for body in mesh_links:
        assert isinstance(body["spheres"], list)
        assert body["spheres"]
        first = body["spheres"][0]
        assert sorted(first) == ["center", "radius"]
        assert len(first["center"]) == 3
        assert first["radius"] > 0.0
```

- [ ] **Step 2: Run the test and verify failure**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_sphere_default_schema.py -q'
```

Expected before implementation: FAIL because default sphere output has `SubSpheres` and `BiggestSphere`, and `primitive_count` is 0.

- [ ] **Step 3: Emit canonical `spheres` entries in the default sphere-tree path**

In `src/SphereTreeURDFGenerator.cpp`, inside `buildSphereModel()` near the existing code:

```cpp
                                    link_json["SubSpheres"] = nlohmann::json();
                                    auto& spheres_json = link_json["SubSpheres"];
```

replace it with:

```cpp
                                    link_json["SubSpheres"] = nlohmann::json();
                                    link_json["spheres"] = nlohmann::json::array();
                                    auto& legacy_spheres_json = link_json["SubSpheres"];
                                    auto& canonical_spheres_json = link_json["spheres"];
```

Then inside the `if (sphere->radius > 0.005)` block, replace:

```cpp
                                            spheres_json[("r" + std::to_string(i++))] = std::vector<double>{sphere_collision->origin.position.x,
                                                                                                      sphere_collision->origin.position.y,
                                                                                                      sphere_collision->origin.position.z,
                                                                                                      sphere->radius};
                                            link_pair.second->collision_array.emplace_back(sphere_collision);
```

with:

```cpp
                                            std::vector<double> legacy_entry{
                                                sphere_collision->origin.position.x,
                                                sphere_collision->origin.position.y,
                                                sphere_collision->origin.position.z,
                                                sphere->radius};
                                            legacy_spheres_json[("r" + std::to_string(i++))] = legacy_entry;

                                            nlohmann::json canonical_entry;
                                            canonical_entry["center"] = {
                                                sphere_collision->origin.position.x,
                                                sphere_collision->origin.position.y,
                                                sphere_collision->origin.position.z};
                                            canonical_entry["radius"] = sphere->radius;
                                            canonical_spheres_json.push_back(canonical_entry);

                                            link_pair.second->collision_array.emplace_back(sphere_collision);
```

- [ ] **Step 4: Make Python primitive counting backward-compatible**

In `python/urdf_approx_geom/api.py`, replace `_count_json_primitives()` with:

```python
def _count_json_primitives(json_path: pathlib.Path | None, key: str) -> int:
    if json_path is None or not json_path.exists():
        return 0
    data = json.loads(json_path.read_text())
    count = 0
    for body in data.values():
        if body is None or not isinstance(body, dict):
            continue
        values = body.get(key)
        if isinstance(values, list):
            count += len(values)
            continue
        if key == "spheres":
            legacy = body.get("SubSpheres")
            if isinstance(legacy, dict):
                count += len(legacy)
    return count
```

- [ ] **Step 5: Run the sphere schema test**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_sphere_default_schema.py -q'
```

Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add src/SphereTreeURDFGenerator.cpp python/urdf_approx_geom/api.py python/tests/test_sphere_default_schema.py
git commit -m "fix: expose canonical sphere json schema"
```

---

### Task 4: Support Per-Mode Presets In `generate_all`

**Files:**
- Modify: `python/urdf_approx_geom/api.py`
- Modify: `python/urdf_approx_geom/cli.py`
- Test: `python/tests/test_generate_all_presets.py`

- [ ] **Step 1: Add failing tests for single preset all-mode workflow**

Create `python/tests/test_generate_all_presets.py`:

```python
from __future__ import annotations

import json
import subprocess
import sys

from urdf_approx_geom import generate_all


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_generate_all_global_single_uses_default_for_convex(tmp_path):
    results = generate_all(FR3_URDF, tmp_path, preset="single")
    assert [r.mode for r in results] == ["convex", "sphere", "capsule"]
    assert (tmp_path / "fr3_convex.urdf").exists()

    sphere = next(r for r in results if r.mode == "sphere")
    capsule = next(r for r in results if r.mode == "capsule")

    sphere_data = json.loads(sphere.json_path.read_text())
    capsule_data = json.loads(capsule.json_path.read_text())
    assert all(len(body.get("spheres", [])) == 1 for body in sphere_data.values())
    assert all(len(body.get("capsules", [])) == 1 for body in capsule_data.values())


def test_generate_all_accepts_per_mode_presets(tmp_path):
    results = generate_all(
        FR3_URDF,
        tmp_path,
        presets={"sphere": "single", "capsule": "default", "convex": "default"},
    )
    assert [r.mode for r in results] == ["convex", "sphere", "capsule"]
    capsule = next(r for r in results if r.mode == "capsule")
    capsule_data = json.loads(capsule.json_path.read_text())
    assert sum(len(body.get("capsules", [])) for body in capsule_data.values()) > 1


def test_cli_all_accepts_per_mode_preset_flags(tmp_path):
    proc = subprocess.run(
        [
            sys.executable,
            "-m",
            "urdf_approx_geom.cli",
            "generate",
            "--mode",
            "all",
            "-i",
            FR3_URDF,
            "--output-dir",
            str(tmp_path),
            "--sphere-preset",
            "single",
            "--capsule-preset",
            "single",
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert "convex:" in proc.stdout
    assert "sphere:" in proc.stdout
    assert "capsule:" in proc.stdout
```

- [ ] **Step 2: Run the tests and verify failure**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_generate_all_presets.py -q'
```

Expected before implementation: FAIL because convex receives preset `single`, and CLI has no per-mode preset flags.

- [ ] **Step 3: Add per-mode preset helper**

In `python/urdf_approx_geom/api.py`, add this import:

```python
from typing import Iterable, Mapping, Sequence
```

Replace the current import:

```python
from typing import Iterable, Sequence
```

Add this helper after `_normal_mode()`:

```python
def _preset_for_mode(mode: str, common_preset: str, presets: Mapping[str, str] | None) -> str:
    if presets:
        for key, value in presets.items():
            if _normal_mode(key) == mode:
                return value
    if mode == "convex" and common_preset != "default":
        return "default"
    return common_preset
```

- [ ] **Step 4: Extend `generate_all()` signature and implementation**

In `python/urdf_approx_geom/api.py`, replace the `generate_all()` signature with:

```python
def generate_all(
    input_urdf: str | pathlib.Path,
    output_dir: str | pathlib.Path,
    *,
    modes: Sequence[str] = ("convex", "sphere", "capsule"),
    preset: str = "default",
    presets: Mapping[str, str] | None = None,
    replace_pairs: Iterable[tuple[str, str]] | None = None,
    simplify: bool = True,
) -> list[GenerateResult]:
```

Inside the loop, replace:

```python
                preset=preset,
```

with:

```python
                preset=_preset_for_mode(normal, preset, presets),
```

- [ ] **Step 5: Add CLI flags for per-mode presets**

In `python/urdf_approx_geom/cli.py`, after:

```python
    gen.add_argument("--preset", default="default", help="named preset for sphere/capsule")
```

add:

```python
    gen.add_argument("--convex-preset", default=None, help="convex preset for --mode all")
    gen.add_argument("--sphere-preset", default=None, help="sphere preset for --mode all")
    gen.add_argument("--capsule-preset", default=None, help="capsule preset for --mode all")
```

In the `args.mode == "all"` branch, before calling `generate_all()`, add:

```python
            per_mode_presets = {
                mode: value
                for mode, value in {
                    "convex": args.convex_preset,
                    "sphere": args.sphere_preset,
                    "capsule": args.capsule_preset,
                }.items()
                if value is not None
            }
```

Then pass:

```python
                presets=per_mode_presets,
```

to `generate_all()`.

- [ ] **Step 6: Run per-mode preset tests**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_generate_all_presets.py -q'
```

Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add python/urdf_approx_geom/api.py python/urdf_approx_geom/cli.py python/tests/test_generate_all_presets.py
git commit -m "fix: support per-mode presets in all-mode generation"
```

---

### Task 5: Make SphereTree Tests Safe In Clean Environments

**Files:**
- Modify: `test/test_spheretree.cpp`
- Modify: `.github/workflows/ci.yml`

- [ ] **Step 1: Confirm current failure**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_spheretree --gtest_color=no'
```

Expected before implementation: FAIL on `SphereTreeTest.MedialTest` or segfault after dereferencing an empty file list.

- [ ] **Step 2: Add a skip helper to `test/test_spheretree.cpp`**

In `test/test_spheretree.cpp`, inside `class SphereTreeTest`, after `replaceWith()`, add:

```cpp
    static std::string requireFirstObjOrSkip(const std::string& meshPath) {
        std::vector<std::string> allOBJFiles;
        fetchAllFilesWith(meshPath, ".obj", allOBJFiles);
        if (allOBJFiles.empty()) {
            GTEST_SKIP() << "No .obj test assets found under " << meshPath;
        }
        return allOBJFiles.front();
    }
```

- [ ] **Step 3: Remove unsafe `front()` calls**

In each of `MedialTest`, `GridTest`, `SpawnTest`, `HubbardTest`, and `OctreeTest`, replace this pattern:

```cpp
    std::vector<std::string> allOBJFiles;
    fetchAllFilesWith(meshPath, ".obj", allOBJFiles);
    m_method = SphereTreeMethod::SphereTreeMethodGrid::create(configPath + "/sphereTree/sphereTreeConfig.yml");
    const std::string& test_obj = allOBJFiles.front();
```

with:

```cpp
    const std::string test_obj = requireFirstObjOrSkip(meshPath);
    m_method = SphereTreeMethod::SphereTreeMethodGrid::create(configPath + "/sphereTree/sphereTreeConfig.yml");
```

Use the matching method constructor for each test:

```cpp
SphereTreeMethod::SphereTreeMethodMedial::create(...)
SphereTreeMethod::SphereTreeMethodGrid::create(...)
SphereTreeMethod::SphereTreeMethodSpawn::create(...)
SphereTreeMethod::SphereTreeMethodHubbard::create(...)
SphereTreeMethod::SphereTreeMethodOctree::create(...)
```

In `MedialTest`, replace:

```cpp
    auto ret = m_method->constructTree(meshPath, tree);
```

with:

```cpp
    auto ret = m_method->constructTree(test_obj, tree);
```

- [ ] **Step 4: Run `test_spheretree`**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_spheretree --gtest_color=no'
```

Expected: PASS, with skipped tests allowed when repository-local OBJ assets are absent. No segfault is acceptable.

- [ ] **Step 5: Update CI to run the safe sphere-tree test**

In `.github/workflows/ci.yml`, in the Docker `Build and test` script, after:

```yaml
            ./build/test/test_capsule --gtest_color=no
```

add:

```yaml
            ./build/test/test_spheretree --gtest_color=no
```

- [ ] **Step 6: Commit**

```bash
git add test/test_spheretree.cpp .github/workflows/ci.yml
git commit -m "fix: make sphere-tree tests safe without private assets"
```

---

### Task 6: Make CI Non-Red For Open Pull Requests

**Files:**
- Modify: `.github/workflows/ci.yml`

- [ ] **Step 1: Replace CI workflow with guarded full build plus source hygiene**

Replace `.github/workflows/ci.yml` with:

```yaml
name: CI

on:
  pull_request:
  push:
    branches: [main, master]

jobs:
  source-hygiene:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: false

      - name: Reject generated artifacts
        run: |
          set -e
          if git ls-files build build_dkr .codegraph python/__pycache__ python/tests/__pycache__ python/urdf_approx_geom/__pycache__ scripts/__pycache__ check_json.py check_json2.py | grep .; then
            echo "Generated artifacts are tracked; remove them from git."
            exit 1
          fi

      - name: Whitespace check
        run: git diff --check HEAD~1..HEAD || git diff --check

  docker-build-test:
    runs-on: ubuntu-22.04
    if: ${{ secrets.DOCKER_REGISTRY_USERNAME != '' && secrets.DOCKER_REGISTRY_PASSWORD != '' }}
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Log in to Docker registry
        uses: docker/login-action@v3
        with:
          registry: irmv-docker-hub-registry.cn-shanghai.cr.aliyuncs.com
          username: ${{ secrets.DOCKER_REGISTRY_USERNAME }}
          password: ${{ secrets.DOCKER_REGISTRY_PASSWORD }}

      - name: Build Docker image
        run: docker build -t spherized-dev -f docker/Dockerfile .

      - name: Build and test
        run: |
          docker run --rm -v $PWD:/workspace spherized-dev bash -lc '
            set -e
            cmake -B build -DCMAKE_BUILD_TYPE=Release -DCOMPILE_URDFApproxGeom_PYBINDING=ON
            cmake --build build -j$(nproc)
            ./build/test/test_capsule --gtest_color=no
            ./build/test/test_spheretree --gtest_color=no
            PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests -q
          '

  docker-build-test-skipped:
    runs-on: ubuntu-22.04
    if: ${{ secrets.DOCKER_REGISTRY_USERNAME == '' || secrets.DOCKER_REGISTRY_PASSWORD == '' }}
    steps:
      - name: Explain skipped full build
        run: |
          echo "Skipping full Docker build because private registry credentials are not available."
          echo "The source-hygiene job still runs on every pull request."
```

- [ ] **Step 2: Validate workflow YAML syntax locally**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 - <<PY
import yaml
from pathlib import Path
data = yaml.safe_load(Path(".github/workflows/ci.yml").read_text())
assert data["name"] == "CI"
assert "source-hygiene" in data["jobs"]
assert "docker-build-test" in data["jobs"]
assert "docker-build-test-skipped" in data["jobs"]
print("ci yaml ok")
PY'
```

Expected: prints `ci yaml ok`.

- [ ] **Step 3: Commit**

```bash
git add .github/workflows/ci.yml
git commit -m "ci: guard private docker build for public pull requests"
```

---

### Task 7: Correct Documentation To Match The Implemented Product

**Files:**
- Modify: `README.md`
- Modify: `docs/output-schema.md`
- Modify: `docs/python-api.md`
- Modify: `docs/developer-guide.md`
- Modify: `docs/config-presets.md`
- Modify: `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`
- Test: documentation grep checks

- [ ] **Step 1: Update sphere schema docs**

In `README.md` and `docs/output-schema.md`, make the sphere output section state:

```markdown
`sphere` writes an output URDF and a JSON sidecar. The canonical per-link field is `spheres`, where each entry has `center` and `radius`. Some legacy sphere-tree outputs may also include `BiggestSphere` and `SubSpheres`; new consumers should read `spheres`.
```

- [ ] **Step 2: Update Python API docs**

In `docs/python-api.md`, add `presets` and per-mode preset information:

```markdown
`generate_all()` accepts either a common `preset` or a per-mode `presets` mapping:

```python
from urdf_approx_geom import generate_all

results = generate_all(
    "robot.urdf",
    "out",
    presets={"sphere": "single", "capsule": "default", "convex": "default"},
)
```

When a common preset such as `"single"` is used with `generate_all()`, convex mode keeps its `default` preset because convex currently has no single-primitive preset.
```

- [ ] **Step 3: Fix developer guide test commands**

In `docs/developer-guide.md`, replace the Python test block:

```bash
cp build/python/urdf_approx_geom/*.so python/urdf_approx_geom/
PYTHONPATH=/workspace/python python3 -m pytest python/tests -q
```

with:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m pytest python/tests -q
```

Replace:

```bash
cd build && ctest --output-on-failure
```

with:

```bash
./build/test/test_capsule --gtest_color=no
./build/test/test_spheretree --gtest_color=no
```

- [ ] **Step 4: Fix handoff stale test status**

In `doc/handoffs/2026-07-02-capsule-geometry-handoff.md`, replace:

```markdown
Python round-trip test (`pytest python/tests`) currently crashes with Bus error on `capsuleized()` call — pre-existing, not introduced by this branch.
```

with:

```markdown
The productized Python package now runs the Python test suite through `PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests -q`. If future pybind crashes appear, treat them as release blockers rather than documented expected behavior.
```

- [ ] **Step 5: Add cwd-independent CLI note**

In `README.md`, under the command-line section, add:

```markdown
The Python CLI locates repository scripts from the package path, so `validate` and `visualize` work even when invoked from outside the repository root as long as `PYTHONPATH` includes both `python` and `build/python`.
```

- [ ] **Step 6: Run documentation checks**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && grep -R "BiggestSphere.*SubSpheres" -n README.md docs/output-schema.md && grep -R "PYTHONPATH=\\$PWD/python:\\$PWD/build/python" -n README.md docs docs/developer-guide.md && ! grep -R "Bus error" -n README.md docs doc/handoffs/2026-07-02-capsule-geometry-handoff.md'
```

Expected: the first two `grep` commands find matches; the final negated `grep` finds no `Bus error` in public docs or the updated handoff.

- [ ] **Step 7: Commit**

```bash
git add README.md docs doc/handoffs/2026-07-02-capsule-geometry-handoff.md
git commit -m "docs: align product docs with verified interfaces"
```

---

### Task 8: Final Verification And Review Gate

**Files:**
- Modify only files needed to fix failures found in this task.

- [ ] **Step 1: Run build and core tests**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake -B build -DCMAKE_BUILD_TYPE=Release -DCOMPILE_URDFApproxGeom_PYBINDING=ON && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_color=no && ./build/test/test_spheretree --gtest_color=no'
```

Expected: build succeeds, `test_capsule` passes, and `test_spheretree` passes or reports skipped resource-dependent tests without crashing.

- [ ] **Step 2: Run Python tests**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests -q'
```

Expected: all Python tests pass. If runtime exceeds 6 minutes, record the slowest tests with `pytest --durations=10` and split heavy integration tests in a follow-up plan.

- [ ] **Step 3: Run the full public product workflow**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && rm -rf /tmp/fr3_product_final && mkdir -p /tmp/fr3_product_final && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m urdf_approx_geom.cli generate --mode all -i resources/fr3/urdf/fr3.urdf --output-dir /tmp/fr3_product_final --sphere-preset single --capsule-preset default && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m urdf_approx_geom.cli validate --mode capsule --json /tmp/fr3_product_final/fr3_capsule.json --urdf resources/fr3/urdf/fr3.urdf --max-capv-aabb 2.50 --max-r-binmed 1.50 && cd /tmp && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m urdf_approx_geom.cli visualize --mode capsule --urdf /workspace/resources/fr3/urdf/fr3.urdf --json /tmp/fr3_product_final/fr3_capsule.json --mjcf /tmp/fr3_product_final/fr3_capsules.xml && test -f /tmp/fr3_product_final/fr3_convex.urdf && test -f /tmp/fr3_product_final/fr3_spherized.urdf && test -f /tmp/fr3_product_final/fr3_capsule.urdf && test -f /tmp/fr3_product_final/fr3_capsules.xml'
```

Expected: exit code 0. Output for sphere must report `primitives` greater than 0.

- [ ] **Step 4: Verify generated artifacts are not tracked**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git ls-files build build_dkr .codegraph python/__pycache__ python/tests/__pycache__ python/urdf_approx_geom/__pycache__ scripts/__pycache__ check_json.py check_json2.py && git diff --check e4323e5..HEAD'
```

Expected: `git ls-files` prints no paths, and `git diff --check` prints no whitespace errors.

- [ ] **Step 5: Inspect the branch diff at a high level**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git diff --stat e4323e5..HEAD && git diff --name-status e4323e5..HEAD | sed -n "1,180p"'
```

Expected: diff contains source, config, docs, tests, and workflow files only. It must not contain `build/`, `.codegraph/`, `__pycache__/`, `check_json.py`, or `check_json2.py`.

- [ ] **Step 6: Commit verification fixes if needed**

If this task required code or doc changes, commit them:

```bash
git add .github .gitignore README.md docs doc config python scripts src test
git commit -m "fix: complete productization review cleanup"
```

If there are no changes after verification, do not create an empty commit.

---

## Self-Review

Spec coverage:

- Repository hygiene: Task 1 and Task 8.
- Cwd-independent CLI: Task 2 and Task 8.
- Sphere schema and primitive count: Task 3 and Task 7.
- Per-mode preset workflow: Task 4 and Task 7.
- Safe sphere-tree tests and CI coverage: Task 5 and Task 6.
- Public/open pull request CI behavior: Task 6.
- Documentation consistency: Task 7.

Placeholder scan:

- The plan contains no placeholder markers.
- Each code-changing task names exact files and includes concrete snippets.
- Verification commands include expected results and run inside the required Docker container.

Type consistency:

- Public modes remain `convex`, `sphere`, and `capsule`.
- Public API still uses `GenerateResult`, `generate()`, and `generate_all()`.
- New package modules are `urdf_approx_geom.validation` and `urdf_approx_geom.visualization`.
- Canonical sphere JSON field is consistently `spheres`.

## Execution Handoff

Execute tasks in order. Task 1 must run first because tracked generated artifacts currently break diff hygiene and whitespace checks. Task 8 must run last.
