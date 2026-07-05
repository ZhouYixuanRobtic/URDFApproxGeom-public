# Release Readiness Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove the remaining release blockers for the URDF-to-convex/sphere/capsule product branch: branch-wide hygiene, public CI smoke coverage, real public sphere-tree assets, and unambiguous capsule comparison semantics.

**Architecture:** Keep the private Docker full-build path for C++ integration tests, but add a no-secret public CI path that validates packaging, CLI importability, and documentation-level commands. Make Python extension loading lazy so non-generation commands work before the pybind11 module is built. Replace private sphere-tree test assets with small FR3 OBJ files already checked into `resources/fr3/meshes`.

**Tech Stack:** CMake, GoogleTest, GitHub Actions, Python 3.10, setuptools PEP 660 editable installs, pytest, existing pybind11 extension `_urdf_approx_geom`.

---

## File Structure

- Modify `.gitignore`: remove the trailing blank line that currently fails `git diff --check`.
- Modify `.github/workflows/ci.yml`: make whitespace checking cover the branch diff, keep private Docker full tests, and add a public no-secret Python package smoke job.
- Modify `python/urdf_approx_geom/api.py`: lazy-load `_urdf_approx_geom` only inside generation calls.
- Modify `python/urdf_approx_geom/__init__.py`: expose legacy `capsuleized`, `convex`, and `spherized` as lazy wrappers instead of importing the extension during package import.
- Modify `python/urdf_approx_geom/cli.py`: add `compare --require-improvement` and keep `presets` usable without the compiled extension.
- Modify `python/urdf_approx_geom/validation.py`: support optional relative improvement checks for capsule comparison.
- Modify `python/tests/test_validation_cli.py`: test both absolute-only compare and strict relative compare.
- Create `python/tests/test_public_python_smoke.py`: verify CLI `presets` and `--help` work with only `python/` on `PYTHONPATH`.
- Modify `test/test_spheretree.cpp`: use small public FR3 OBJ assets and require the sphere-tree algorithm tests to run.
- Modify `README.md`, `docs/visualization.md`, or `docs/config-presets.md`: document compare semantics and the optional strict improvement flag.

All commands below must run inside the existing container:

```bash
docker exec spherized-development bash -lc 'cd /workspace && <command>'
```

---

### Task 1: Fix Branch Hygiene And CI Diff Scope

**Files:**
- Modify: `.gitignore:67-70`
- Modify: `.github/workflows/ci.yml:12-25`

- [ ] **Step 1: Verify the current hygiene failure**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git diff --check 6c7af55..HEAD'
```

Expected: FAIL with:

```text
.gitignore:71: new blank line at EOF.
```

- [ ] **Step 2: Remove the trailing blank line in `.gitignore`**

Change the end of `.gitignore` from:

```gitignore
# Python packaging artifacts
*.egg-info/
*.egg-link
dist/

```

to:

```gitignore
# Python packaging artifacts
*.egg-info/
*.egg-link
dist/
```

- [ ] **Step 3: Run branch-wide whitespace check**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git diff --check 6c7af55..HEAD'
```

Expected: PASS with no output.

- [ ] **Step 4: Make CI whitespace check use the PR/base diff**

In `.github/workflows/ci.yml`, change the `source-hygiene` checkout and whitespace step to:

```yaml
      - uses: actions/checkout@v4
        with:
          submodules: false
          fetch-depth: 0

      - name: Reject generated artifacts
        run: |
          set -e
          if git ls-files build build_dkr .codegraph python/__pycache__ python/tests/__pycache__ python/urdf_approx_geom/__pycache__ scripts/__pycache__ check_json.py check_json2.py | grep .; then
            echo "Generated artifacts are tracked; remove them from git."
            exit 1
          fi

      - name: Whitespace check
        run: |
          set -e
          BASE_SHA="${{ github.event.pull_request.base.sha }}"
          if [ -z "$BASE_SHA" ]; then
            BASE_SHA="${{ github.event.before }}"
          fi
          if git rev-parse --verify "$BASE_SHA^{commit}" >/dev/null 2>&1; then
            git diff --check "$BASE_SHA"..HEAD
          else
            git diff --check
          fi
```

- [ ] **Step 5: Parse the workflow YAML**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 - <<PY
import yaml
with open(".github/workflows/ci.yml", "r", encoding="utf-8") as f:
    data = yaml.safe_load(f)
print(data["name"])
print(sorted(data["jobs"]))
PY'
```

Expected output includes:

```text
CI
['check-secrets', 'docker-build-test', 'docker-build-test-skipped', 'source-hygiene']
```

- [ ] **Step 6: Commit**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git add .gitignore .github/workflows/ci.yml && git commit -m "ci: check release branch hygiene against base"'
```

Expected: commit succeeds.

---

### Task 2: Add Public No-Secret Python Package Smoke CI

**Files:**
- Modify: `python/urdf_approx_geom/api.py:10-13`
- Modify: `python/urdf_approx_geom/__init__.py:5-12`
- Create: `python/tests/test_public_python_smoke.py`
- Modify: `.github/workflows/ci.yml`

- [ ] **Step 1: Write failing public-smoke tests**

Create `python/tests/test_public_python_smoke.py`:

```python
from __future__ import annotations

import os
import subprocess
import sys


def _python_only_env() -> dict[str, str]:
    env = os.environ.copy()
    env["PYTHONPATH"] = "/workspace/python"
    return env


def test_cli_presets_works_without_built_extension_from_tmp() -> None:
    proc = subprocess.run(
        [sys.executable, "-m", "urdf_approx_geom.cli", "presets"],
        cwd="/tmp",
        env=_python_only_env(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert "convex: default" in proc.stdout
    assert "sphere: default, single" in proc.stdout
    assert "capsule:" in proc.stdout


def test_cli_help_works_without_built_extension_from_tmp() -> None:
    proc = subprocess.run(
        [sys.executable, "-m", "urdf_approx_geom.cli", "--help"],
        cwd="/tmp",
        env=_python_only_env(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert "Generate convex, sphere, or capsule" in proc.stdout
```

- [ ] **Step 2: Run the smoke tests and verify they fail**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python python3 -m pytest python/tests/test_public_python_smoke.py -q'
```

Expected: FAIL with `ModuleNotFoundError: No module named '_urdf_approx_geom'`.

- [ ] **Step 3: Lazy-load the extension in `api.py`**

In `python/urdf_approx_geom/api.py`, remove this top-level line:

```python
_ext = load_extension()
```

Add this helper after `Mode = str`:

```python
def _extension():
    return load_extension()
```

Change the extension calls inside `generate()` from:

```python
        message = _ext.capsuleized(str(input_path), str(output_path), str(config_path), pairs)
```

```python
        message = _ext.spherized(str(input_path), str(output_path), str(config_path), pairs, bool(simplify))
```

```python
        message = _ext.convex(str(input_path), str(output_path), pairs)
```

to:

```python
        message = _extension().capsuleized(str(input_path), str(output_path), str(config_path), pairs)
```

```python
        message = _extension().spherized(str(input_path), str(output_path), str(config_path), pairs, bool(simplify))
```

```python
        message = _extension().convex(str(input_path), str(output_path), pairs)
```

- [ ] **Step 4: Lazy-wrap legacy extension functions in `__init__.py`**

Replace `python/urdf_approx_geom/__init__.py` with:

```python
"""Public Python API for URDF approximate collision geometry generation."""

from __future__ import annotations

from ._extension import load_extension
from .api import GenerateResult, generate, generate_all


def _extension_function(name: str):
    def call(*args, **kwargs):
        return getattr(load_extension(), name)(*args, **kwargs)

    call.__name__ = name
    return call


capsuleized = _extension_function("capsuleized")
convex = _extension_function("convex")
spherized = _extension_function("spherized")

__all__ = [
    "GenerateResult",
    "capsuleized",
    "convex",
    "generate",
    "generate_all",
    "spherized",
]
```

- [ ] **Step 5: Verify public-smoke tests pass**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python python3 -m pytest python/tests/test_public_python_smoke.py -q'
```

Expected:

```text
2 passed
```

- [ ] **Step 6: Add the no-secret public CI job**

Append this job to `.github/workflows/ci.yml` under `jobs:` at the same indentation level as `source-hygiene`:

```yaml
  public-python-smoke:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: false

      - uses: actions/setup-python@v5
        with:
          python-version: "3.10"

      - name: Install Python package metadata
        run: |
          python -m pip install --upgrade pip setuptools pytest
          python -m pip install -e python --no-build-isolation

      - name: Run public CLI smoke tests
        run: |
          urdf-approx-geom --help
          urdf-approx-geom presets
          PYTHONPATH=$PWD/python python -m pytest python/tests/test_public_python_smoke.py -q
```

- [ ] **Step 7: Parse the workflow YAML**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 - <<PY
import yaml
with open(".github/workflows/ci.yml", "r", encoding="utf-8") as f:
    data = yaml.safe_load(f)
print(sorted(data["jobs"]))
PY'
```

Expected output includes:

```text
public-python-smoke
```

- [ ] **Step 8: Verify normal extension-backed tests still pass**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_api_contract.py python/tests/test_cli.py python/tests/test_public_python_smoke.py -q'
```

Expected: all selected tests pass.

- [ ] **Step 9: Commit**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git add .github/workflows/ci.yml python/urdf_approx_geom/api.py python/urdf_approx_geom/__init__.py python/tests/test_public_python_smoke.py && git commit -m "ci: add public python smoke coverage"'
```

Expected: commit succeeds.

---

### Task 3: Use Public FR3 OBJ Assets For Sphere-Tree Tests

**Files:**
- Modify: `test/test_spheretree.cpp:93-163`

- [ ] **Step 1: Confirm the current test skips algorithm coverage**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && ./build/test/test_spheretree --gtest_color=no'
```

Expected: 5 skipped tests:

```text
[  SKIPPED ] SphereTreeTest.MedialTest
[  SKIPPED ] SphereTreeTest.GridTest
[  SKIPPED ] SphereTreeTest.SpawnTest
[  SKIPPED ] SphereTreeTest.HubbardTest
[  SKIPPED ] SphereTreeTest.OctreeTest
```

- [ ] **Step 2: Replace private asset helper with a required public FR3 OBJ helper**

In `test/test_spheretree.cpp`, replace `requireFirstObjOrSkip()` with:

```cpp
    std::string publicFr3Obj(const std::string& name = "finger.obj") const {
        namespace fs = std::filesystem;
        const std::vector<std::string> candidates = {
            resourcePath + "/fr3/meshes/franka_hand/collision/collision/" + name,
            resourcePath + "/fr3/meshes/fr3/collision/collision/link7.obj",
            resourcePath + "/fr3/meshes/plate/collision/collision/flex_griper_connect.obj",
        };
        for (const auto& candidate : candidates) {
            if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
                return fs::absolute(candidate).string();
            }
        }
        return "";
    }
```

Keep `fetchAllFilesWith()` only if another local test uses it; otherwise remove it and the unused `replaceWith()` helper in the same file.

- [ ] **Step 3: Change every sphere-tree method test to use the public helper**

For each of `MedialTest`, `GridTest`, `SpawnTest`, `HubbardTest`, and `OctreeTest`, replace the private or panda `meshPath` block:

```cpp
    std::string meshPath = resourcePath + "/robots/panda/meshes/collision/simple";
    const std::string test_obj = requireFirstObjOrSkip(meshPath);
    if (test_obj.empty()) GTEST_SKIP() << "No .obj test assets found under " << meshPath;
```

with:

```cpp
    const std::string test_obj = publicFr3Obj();
    ASSERT_FALSE(test_obj.empty()) << "FR3 public OBJ test asset is missing";
```

For `MedialTest`, replace its current private `YOUSHOU.obj` block with the same two lines.

- [ ] **Step 4: Rebuild and verify sphere-tree tests no longer skip**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake --build build -j$(nproc) && ./build/test/test_spheretree --gtest_color=no'
```

Expected: all 6 tests pass and the output contains no `[  SKIPPED ]`.

- [ ] **Step 5: Commit**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git add test/test_spheretree.cpp && git commit -m "test: use public fr3 assets for sphere-tree coverage"'
```

Expected: commit succeeds.

---

### Task 4: Make Capsule Compare Semantics Explicit And Strict When Requested

**Files:**
- Modify: `python/urdf_approx_geom/validation.py:75-99`
- Modify: `python/urdf_approx_geom/cli.py:62-68,137-143`
- Modify: `python/tests/test_validation_cli.py`
- Modify: `README.md`
- Modify: `docs/visualization.md`

- [ ] **Step 1: Write failing strict-compare tests**

Append these tests to `python/tests/test_validation_cli.py`:

```python
def test_compare_capsule_cli_can_require_relative_improvement(tmp_path):
    out_a = tmp_path / "baseline.urdf"
    out_b = tmp_path / "candidate.urdf"
    baseline = generate("capsule", FR3_URDF, out_a, preset="default")
    candidate = generate("capsule", FR3_URDF, out_b, preset="single")
    proc = run_cli(
        "compare",
        "--mode",
        "capsule",
        "--baseline-json",
        str(baseline.json_path),
        "--candidate-json",
        str(candidate.json_path),
        "--urdf",
        FR3_URDF,
        "--max-capv-aabb",
        "2.50",
        "--max-r-binmed",
        "1.50",
        "--require-improvement",
    )
    assert proc.returncode == 0, proc.stdout
    assert "candidate_worst_capV_aabb" in proc.stdout


def test_compare_capsule_cli_fails_when_candidate_worsens_relative_metrics(tmp_path):
    out_a = tmp_path / "baseline.urdf"
    out_b = tmp_path / "candidate.urdf"
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
        "--urdf",
        FR3_URDF,
        "--max-capv-aabb",
        "2.50",
        "--max-r-binmed",
        "1.50",
        "--require-improvement",
    )
    assert proc.returncode == 1, proc.stdout
    assert "candidate capV/aabb worsened" in proc.stdout
    assert "candidate r/binMed worsened" in proc.stdout
```

- [ ] **Step 2: Run tests and verify failure**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_validation_cli.py -q'
```

Expected: FAIL because `--require-improvement` is not recognized.

- [ ] **Step 3: Add CLI flag**

In `python/urdf_approx_geom/cli.py`, after the `--max-r-binmed` argument in the compare parser, add:

```python
    cmp_parser.add_argument(
        "--require-improvement",
        action="store_true",
        help="fail if candidate worst capV/aabb or r/binMed is worse than baseline",
    )
```

In the `compare_capsule_files()` call, change:

```python
            args.max_r_binmed,
```

to:

```python
            args.max_r_binmed,
            require_improvement=args.require_improvement,
```

- [ ] **Step 4: Add strict relative comparison in validation**

Change `compare_capsule_files()` in `python/urdf_approx_geom/validation.py` to:

```python
def compare_capsule_files(
    baseline_json: str,
    candidate_json: str,
    urdf: str,
    max_capv_aabb: float,
    max_r_binmed: float,
    *,
    require_improvement: bool = False,
) -> int:
    baseline = _capsule_metrics(baseline_json, urdf)
    candidate = _capsule_metrics(candidate_json, urdf)
    baseline_capv = _worst(baseline, "capV_aabb")
    candidate_capv = _worst(candidate, "capV_aabb")
    baseline_r = _worst(baseline, "r_binMed")
    candidate_r = _worst(candidate, "r_binMed")
    summary = {
        "baseline_count": _count(baseline),
        "candidate_count": _count(candidate),
        "baseline_worst_capV_aabb": baseline_capv,
        "candidate_worst_capV_aabb": candidate_capv,
        "baseline_worst_r_binMed": baseline_r,
        "candidate_worst_r_binMed": candidate_r,
    }
    print(json.dumps(summary, indent=2, sort_keys=True))
    failures = validate_capsule_metrics(candidate, max_capv_aabb, max_r_binmed)
    if require_improvement:
        if candidate_capv > baseline_capv:
            failures.append(
                f"candidate capV/aabb worsened: {candidate_capv:.2f} > {baseline_capv:.2f}"
            )
        if candidate_r > baseline_r:
            failures.append(
                f"candidate r/binMed worsened: {candidate_r:.2f} > {baseline_r:.2f}"
            )
    if failures:
        print("candidate validation failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0
```

- [ ] **Step 5: Verify validation tests pass**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests/test_validation_cli.py -q'
```

Expected: all tests in `test_validation_cli.py` pass.

- [ ] **Step 6: Document compare semantics**

In `README.md`, add this paragraph after the command-line examples:

```markdown
`compare --mode capsule` prints baseline/candidate metrics and validates the
candidate against absolute ceilings. Add `--require-improvement` when the
candidate must also be no worse than the baseline on worst capV/aabb and
r/binMed.
```

In `docs/visualization.md`, add this section after `## Validation`:

Add this Markdown:

    ## Compare Capsule Sidecars
    
    ```bash
    PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli compare --mode capsule --baseline-json /tmp/fr3_single.json --candidate-json /tmp/fr3_default.json --urdf resources/fr3/urdf/fr3.urdf --max-capv-aabb 2.50 --max-r-binmed 1.50
    ```
    
    By default, `compare` summarizes both sidecars and validates only the candidate
    against absolute ceilings. Add `--require-improvement` when a tuning run must
    also improve or match the baseline on worst capV/aabb and r/binMed.

- [ ] **Step 7: Commit**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git add python/urdf_approx_geom/cli.py python/urdf_approx_geom/validation.py python/tests/test_validation_cli.py README.md docs/visualization.md && git commit -m "feat: make capsule comparison semantics explicit"'
```

Expected: commit succeeds.

---

### Task 5: Run Release Readiness Verification

**Files:**
- No source edits unless a verification command exposes a concrete failure.

- [ ] **Step 1: Run branch hygiene checks**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git diff --check 6c7af55..HEAD && git ls-files build build_dkr .codegraph python/__pycache__ python/tests/__pycache__ python/urdf_approx_geom/__pycache__ scripts/__pycache__ check_json.py check_json2.py'
```

Expected: no output.

- [ ] **Step 2: Run C++ tests**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && cmake -B build -DCMAKE_BUILD_TYPE=Release -DCOMPILE_URDFApproxGeom_PYBINDING=ON && cmake --build build -j$(nproc) && ./build/test/test_capsule --gtest_color=no && ./build/test/test_spheretree --gtest_color=no'
```

Expected:

```text
test_capsule: all tests pass
test_spheretree: all 6 tests pass, 0 skipped
```

- [ ] **Step 3: Run Python tests**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m pytest python/tests -q'
```

Expected: all Python tests pass.

- [ ] **Step 4: Verify source-only public smoke path**

Run:

```bash
docker exec spherized-development bash -lc 'cd /tmp && PYTHONPATH=/workspace/python python3 -m urdf_approx_geom.cli --help && PYTHONPATH=/workspace/python python3 -m urdf_approx_geom.cli presets'
```

Expected: both commands exit 0 and `presets` lists convex, sphere, and capsule presets.

- [ ] **Step 5: Verify editable install and console script**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && python3 -m pip install -e python --no-build-isolation && cd /tmp && urdf-approx-geom --help && urdf-approx-geom presets && python3 -m pip uninstall -y urdf-approx-geom'
```

Expected: install succeeds as `urdf-approx-geom-1.5.0`, console script works, uninstall succeeds.

- [ ] **Step 6: Verify product workflow**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && rm -rf /tmp/fr3_release_final && mkdir -p /tmp/fr3_release_final && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m urdf_approx_geom.cli generate --mode all -i resources/fr3/urdf/fr3.urdf --output-dir /tmp/fr3_release_final --sphere-preset single --capsule-preset default && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m urdf_approx_geom.cli validate --mode capsule --json /tmp/fr3_release_final/fr3_capsule.json --urdf resources/fr3/urdf/fr3.urdf --max-capv-aabb 2.50 --max-r-binmed 1.50 && cd /tmp && PYTHONPATH=/workspace/python:/workspace/build/python python3 -m urdf_approx_geom.cli visualize --mode capsule --urdf /workspace/resources/fr3/urdf/fr3.urdf --json /tmp/fr3_release_final/fr3_capsule.json --mjcf /tmp/fr3_release_final/fr3_capsules.xml && test -f /tmp/fr3_release_final/fr3_convex.urdf && test -f /tmp/fr3_release_final/fr3_spherized.urdf && test -f /tmp/fr3_release_final/fr3_capsule.urdf && test -f /tmp/fr3_release_final/fr3_capsules.xml'
```

Expected: generation, validation, visualization, and artifact checks all pass.

- [ ] **Step 7: Confirm worktree state**

Run:

```bash
docker exec spherized-development bash -lc 'cd /workspace && git status --short --branch'
```

Expected: only intentional untracked plan files appear, or a clean tree if plan files were committed separately.

---

## Self-Review

Spec coverage:
- Branch hygiene failure is covered by Task 1.
- CI diff-scope gap is covered by Task 1.
- Public CI without private registry credentials is covered by Task 2.
- Python import and CLI smoke without compiled extension is covered by Task 2.
- Real sphere-tree algorithm coverage using public FR3 OBJ files is covered by Task 3.
- Capsule compare ambiguity is covered by Task 4.
- End-to-end release verification is covered by Task 5.

Placeholder scan:
- No task uses placeholder text.
- Every code-changing step includes the target code or exact replacement.
- Every test step includes a concrete command and expected result.

Type and signature consistency:
- `compare_capsule_files()` gains keyword-only `require_improvement`; `cli.py` passes the same name.
- Tests call the public CLI flag `--require-improvement`.
- Lazy extension wrappers preserve exported names: `capsuleized`, `convex`, `spherized`, `generate`, and `generate_all`.
