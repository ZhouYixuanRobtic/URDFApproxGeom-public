"""Structured Python API over the C++ URDF approximation generators."""

from __future__ import annotations

from dataclasses import dataclass
import json
import pathlib
from typing import Iterable, Mapping, Sequence

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


def _preset_for_mode(mode: str, common_preset: str, presets: Mapping[str, str] | None) -> str:
    if presets:
        for key, value in presets.items():
            if _normal_mode(key) == mode:
                return value
    if mode == "convex" and common_preset != "default":
        return "default"
    return common_preset


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
        json_path=json_path,
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
    presets: Mapping[str, str] | None = None,
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
                preset=_preset_for_mode(normal, preset, presets),
                replace_pairs=replace_pairs,
                simplify=simplify,
            )
        )
    return results
