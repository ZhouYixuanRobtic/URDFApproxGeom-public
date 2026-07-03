# Python API

The `urdf_approx_geom` Python package exposes all three approximation modes through a clean Python API backed by pybind11 C++ bindings.

## Quick Reference

```python
from urdf_approx_geom import generate, generate_all

# Single mode
result = generate("capsule", "robot.urdf", "out/robot_capsule.urdf", preset="default")
print(f"JSON: {result.json_path}, primitives: {result.primitive_count}")

# All modes at once
all_results = generate_all("robot.urdf", "out", modes=["convex", "sphere", "capsule"])
for r in all_results:
    print(f"{r.mode}: {r.output_urdf}")
```

## Parameters

- `mode`: one of `"convex"`, `"sphere"`, `"capsule"`
- `input_urdf`: path to the input mesh-based URDF
- `output_urdf`: path for the output URDF
- `preset`: configuration preset name (default: `"default"`)
- `replace_pairs`: optional list of `(key, value)` pairs for mesh path substitution

## Return Value

Each `generate()` call returns a result object with:
- `output_urdf`: path to the generated URDF
- `json_path`: path to the JSON sidecar (empty for convex mode)
- `primitive_count`: number of collision primitives in the output
