"""Named configuration presets for public generation modes."""

from __future__ import annotations

import pathlib

from ._paths import config_root


_PRESETS = {
    "capsule": {
        "single": "capsule/single.yml",
        "default": "capsule/default.yml",
        "high_detail": "capsule/high_detail.yml",
        "tight": "capsule/high_detail.yml",
    },
    "sphere": {
        "single": "sphereTree/single.yml",
        "default": "sphereTree/default.yml",
    },
    "convex": {
        "default": "convex/default.yml",
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
    root = config_root()
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
