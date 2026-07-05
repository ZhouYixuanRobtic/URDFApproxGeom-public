"""Structured Python API over the C++ URDF approximation generators."""

from __future__ import annotations

from dataclasses import dataclass
import json
import pathlib
from typing import Iterable, Mapping, Sequence

from ._extension import load_extension
from .presets import resolve_preset

Mode = str


def _extension():
    return load_extension()


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


def _prepare_visual_meshes(
    input_urdf: str | pathlib.Path,
    replace_pairs: Iterable[tuple[str, str]] | None,
) -> tuple[list[tuple[str, str]], pathlib.Path | None]:
    """Convert any non-OBJ/STL visual meshes (FR3 ships .dae) to .obj so the
    C++ loader (OBJ/STL only) can fit the true visual geometry. Returns merged
    replace_pairs plus the temp dir holding the converted .obj (caller keeps it
    alive for the C++ run). No-op when every visual mesh is already OBJ/STL."""
    import tempfile
    import xml.etree.ElementTree as ET

    pairs = [(str(a), str(b)) for a, b in (replace_pairs or [])]
    try:
        tree = ET.parse(str(input_urdf))
    except Exception:
        return pairs, None

    unique: list[str] = []
    seen: set[str] = set()
    for vis in tree.iter("visual"):
        mesh = vis.find(".//mesh")
        if mesh is None:
            continue
        fn = mesh.get("filename", "")
        if not fn or pathlib.Path(fn).suffix.lower() in {".obj", ".stl"}:
            continue
        if fn in seen:
            continue
        seen.add(fn)
        unique.append(fn)
    if not unique:
        return pairs, None

    import trimesh  # lazy: only required when .dae (or similar) visuals exist
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="urdf_approx_visual_"))
    for fn in unique:
        mesh = trimesh.load(fn, force="mesh")
        out = tmp / (pathlib.Path(fn).stem + ".obj")
        mesh.export(out)
        pairs.append((fn, str(out)))
    return pairs, tmp


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
    mesh_source: str = "visual",
) -> GenerateResult:
    normal = _normal_mode(mode)
    input_path = pathlib.Path(input_urdf)
    output_path = pathlib.Path(output_urdf)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    pairs = [(str(a), str(b)) for a, b in (replace_pairs or [])]
    config_path = pathlib.Path(config) if config else resolve_preset(normal, preset)

    if mesh_source not in {"visual", "collision"}:
        raise ValueError(f"mesh_source must be 'visual' or 'collision', got {mesh_source!r}")
    if mesh_source == "visual":
        pairs, _ = _prepare_visual_meshes(input_path, pairs)

    if normal == "capsule":
        message = _extension().capsuleized(str(input_path), str(output_path), str(config_path), pairs, mesh_source)
        json_path = _sidecar_json(output_path)
        primitive_count = _count_json_primitives(json_path, "capsules")
    elif normal == "sphere":
        message = _extension().spherized(str(input_path), str(output_path), str(config_path), pairs, bool(simplify), mesh_source)
        json_path = _sidecar_json(output_path)
        primitive_count = _count_json_primitives(json_path, "spheres")
    else:
        message = _extension().convex(str(input_path), str(output_path), pairs)
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


def generate_sphere_pair(
    input_urdf: str | pathlib.Path,
    default_output: str | pathlib.Path,
    single_output: str | pathlib.Path,
    *,
    preset: str = "default",
    config: str | pathlib.Path | None = None,
    replace_pairs: Iterable[tuple[str, str]] | None = None,
    simplify: bool = True,
    mesh_source: str = "visual",
) -> tuple[GenerateResult, GenerateResult]:
    """Run the sphere tree once and emit both the multi-sphere URDF (default)
    and a single-sphere URDF (tree.biggest_sphere). The single fit is computed
    inside the default run, so this is one mesh load + one tree build for both
    outputs instead of two generator runs."""
    input_path = pathlib.Path(input_urdf)
    default_path = pathlib.Path(default_output)
    single_path = pathlib.Path(single_output)
    for p in (default_path, single_path):
        p.parent.mkdir(parents=True, exist_ok=True)
    pairs = [(str(a), str(b)) for a, b in (replace_pairs or [])]
    if mesh_source == "visual":
        pairs, _ = _prepare_visual_meshes(input_path, pairs)
    config_path = pathlib.Path(config) if config else resolve_preset("sphere", preset)
    message = _extension().spherized_pair(
        str(input_path), str(default_path), str(single_path), str(config_path),
        pairs, bool(simplify), mesh_source,
    )
    default_json = _sidecar_json(default_path)
    single_json = _sidecar_json(single_path)
    return (
        GenerateResult("sphere", input_path, default_path, default_json, config_path, message,
                       _count_json_primitives(default_json, "spheres")),
        GenerateResult("sphere", input_path, single_path, single_json, config_path, message,
                       _count_json_primitives(single_json, "spheres")),
    )


def generate_capsule_multi(
    input_urdf: str | pathlib.Path,
    outputs: Iterable[tuple[str | pathlib.Path, str]],
    *,
    replace_pairs: Iterable[tuple[str, str]] | None = None,
    mesh_source: str = "visual",
) -> list[GenerateResult]:
    """Fit multiple capsule presets on one mesh load + one Manifold pass per
    link. `outputs` is an iterable of (output_urdf, preset_name) pairs."""
    input_path = pathlib.Path(input_urdf)
    pairs = [(str(a), str(b)) for a, b in (replace_pairs or [])]
    if mesh_source == "visual":
        pairs, _ = _prepare_visual_meshes(input_path, pairs)
    resolved: list[tuple[str, pathlib.Path]] = []
    for out, preset in outputs:
        out_path = pathlib.Path(out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        cfg = resolve_preset("capsule", preset)
        resolved.append((str(out_path), cfg))
    message = _extension().capsuleized_multi(
        str(input_path),
        [(out, str(cfg)) for out, cfg in resolved],
        pairs,
        mesh_source,
    )
    results: list[GenerateResult] = []
    for out, cfg in resolved:
        out_path = pathlib.Path(out)
        json_path = _sidecar_json(out_path)
        results.append(
            GenerateResult("capsule", input_path, out_path, json_path, cfg, message,
                           _count_json_primitives(json_path, "capsules"))
        )
    return results


def generate_all(
    input_urdf: str | pathlib.Path,
    output_dir: str | pathlib.Path,
    *,
    modes: Sequence[str] = ("convex", "sphere", "capsule"),
    preset: str = "default",
    presets: Mapping[str, str] | None = None,
    replace_pairs: Iterable[tuple[str, str]] | None = None,
    simplify: bool = True,
    mesh_source: str = "visual",
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
                mesh_source=mesh_source,
            )
        )
    return results
