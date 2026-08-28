# Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
# All Rights Reserved.

"""Mesh preparation helpers for the Python-first URDF pipeline.

The C++ core only consumes OBJ/STL.  This module converts other formats
(currently DAE and anything trimesh can load) into temporary OBJ files and
performs light watertight validation before handing meshes to the extension.
"""

from __future__ import annotations

import pathlib
import tempfile
import xml.etree.ElementTree as ET
from typing import Iterable


def validate_watertight(path: str | pathlib.Path) -> bool:
    """Return True when trimesh considers the mesh watertight."""
    import trimesh

    mesh = trimesh.load(str(path), force="mesh")
    if mesh is None:
        return False
    if not hasattr(mesh, "is_watertight"):
        return False
    return bool(mesh.is_watertight)


def prepare_visual_meshes(
    input_urdf: str | pathlib.Path,
    replace_pairs: Iterable[tuple[str, str]] | None = None,
) -> tuple[list[tuple[str, str]], pathlib.Path | None, list[str]]:
    """Convert non-OBJ/STL visual meshes to OBJ and return replace pairs.

    Returns ``(merged_pairs, temp_dir, warnings)``.  The caller must keep
    ``temp_dir`` alive while the C++ extension runs.
    """
    pairs = [(str(a), str(b)) for a, b in (replace_pairs or [])]
    warnings: list[str] = []
    try:
        tree = ET.parse(str(input_urdf))
    except Exception as exc:
        warnings.append(f"could not parse URDF for mesh preparation: {exc}")
        return pairs, None, warnings

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
        return pairs, None, warnings

    import trimesh  # lazy: only required when non-OBJ/STL visuals exist

    _repo_root = pathlib.Path(__file__).resolve().parents[2]
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="urdf_approx_visual_"))
    for fn in unique:
        local_fn = (
            str(_repo_root / fn.removeprefix("/workspace/")) if fn.startswith("/workspace/") else fn
        )
        try:
            mesh = trimesh.load(local_fn, force="mesh")
            if mesh is None:
                warnings.append(f"failed to load {fn}")
                continue
            # Best-effort repair before exporting.
            mesh.merge_vertices()
            mesh.remove_degenerate_faces()
            mesh.fix_normals()
            if hasattr(mesh, "fill_holes") and not mesh.is_watertight:
                mesh.fill_holes()
            out = tmp / (pathlib.Path(fn).stem + ".obj")
            mesh.export(out)
            pairs.append((fn, str(out)))
        except Exception as exc:
            warnings.append(f"failed to prepare {fn}: {exc}")
    return pairs, tmp, warnings
