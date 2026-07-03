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
