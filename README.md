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

The Python CLI locates repository scripts from the package path, so `validate` and `visualize` work even when invoked from outside the repository root as long as `PYTHONPATH` includes both `python` and `build/python`.

Compare all approximation variants side by side in [robot_viewer](https://github.com/fan-ziqi/robot_viewer) — generates convex + sphere (single, default) + capsule (single, default, high_detail), rewrites every mesh path to relative `meshes/<name>`, and copies the meshes into one self-contained directory:

```bash
PYTHONPATH=$PWD/python:$PWD/build/python python3 -m urdf_approx_geom.cli compare-all -i robot.urdf --bundle-dir out/compare_bundle
```

Install robot_viewer yourself (it is not bundled with this repo), start its dev server, and drag `out/compare_bundle` onto its file tree. Each variant loads as its own URDF; toggle Visual / Collision to compare the `.dae` ground truth against the approximation. To compare two capsule sidecars' metrics in the terminal instead, use `compare --mode capsule` (add `--require-improvement` to fail when the candidate worsens worst capV/aabb or r/binMed).

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
- `high_detail`: more axial sections and a larger capsule budget; raises the
  detail ceiling but does not guarantee tighter output than `default`. On FR3
  both emit 17 capsules and `default` has a marginally lower worst capV/aabb
  (1.77 vs 1.86) while `high_detail` lowers worst r/binMed.
- `tight`: alias for `high_detail`.

Sphere presets:

- `single`: one conservative sphere per link.
- `default`: sphere-tree fit.

Convex presets:

- `default`: convex hull collision mesh generation.

## Output Contracts

- `convex`: writes an output URDF whose collision meshes point to generated convex meshes.
- `sphere`: writes an output URDF and a JSON sidecar. The canonical per-link field is `spheres`, where each entry has `center` and `radius`. Some legacy sphere-tree outputs may also include `BiggestSphere` and `SubSpheres`; new consumers should read `spheres`.
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

## Sample Output (FR3)

Geometry accuracy vs the `*.dae` visual meshes in `resources/fr3/meshes/fr3/visual/`, treated as ground truth (`vol/dae` = approximation volume ÷ `.dae` volume; `.dae` = 1.0, lower = closer to the true shape). All rows measured on fr3 arm links link0–link7 (hand/finger links have no `*.dae`):

| Form | Primitives | Worst vol/dae |
|------|-----------:|-------------:|
| `*.dae` visual (reference) | mesh | **1.00** |
| `convex` hull              | mesh | 1.76 |
| capsule `single`           | 8    | 3.07 |
| capsule `default`          | 10   | 4.23 |
| capsule `high_detail`      | 10   | 3.30 |
| sphere `default`           | 62   | 5.12 |
| sphere `single`            | 8    | 12.21 |

`convex` is closest to the `.dae` ground truth (1.76) since a hull hugs the convex envelope of the mesh. Capsules and spheres are looser volume-wise because they are simple primitives grown to cover the surface — the tradeoff for low primitive count and simulator-native collision. `capsule single` is one bounding capsule per link (8 total, tightest capsule preset on this metric); `sphere single` is one bounding sphere per link and over-sweeps elongated links the most, so its `default` (medial sphere tree, 62 spheres) is the tighter sphere preset.

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
