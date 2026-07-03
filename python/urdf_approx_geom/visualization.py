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
