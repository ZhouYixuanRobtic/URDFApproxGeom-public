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

`compare --mode capsule` prints baseline/candidate metrics and validates the
candidate against absolute ceilings. Add `--require-improvement` when the
candidate must also be no worse than the baseline on worst capV/aabb and
r/binMed.

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
