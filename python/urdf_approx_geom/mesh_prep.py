# Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
# All Rights Reserved.

"""Mesh preparation helpers for the Python-first URDF pipeline.

The C++ core only consumes OBJ/STL.  This module converts other formats
(currently DAE and anything trimesh can load) into temporary OBJ files and
performs best-effort repair before handing meshes to the extension.

FR3 visual DAEs are open/non-manifold render meshes (many separate patches),
so they cannot be repaired into a closed 2-manifold by merge/fill.  They are
still the true link geometry, so the Python pipeline passes them through with
``allow_open_mesh=True`` and only warns; the C++ entry points stay strict.
"""

from __future__ import annotations

import hashlib
import pathlib
import tempfile
import xml.etree.ElementTree as ET
from typing import Iterable

import numpy as np


def validate_watertight(path: str | pathlib.Path) -> bool:
    """Return True when trimesh considers the mesh watertight."""
    import trimesh

    try:
        mesh = trimesh.load(str(path), force="mesh")
    except Exception:
        return False
    if mesh is None:
        return False
    if not hasattr(mesh, "is_watertight"):
        return False
    return bool(mesh.is_watertight)


def _edge_valence_stats(mesh) -> tuple[int, int]:
    """Return ``(boundary_edges, nonmanifold_edges)`` for a trimesh mesh."""
    faces = np.asarray(mesh.faces, dtype=np.int64)
    if faces.ndim != 2 or faces.shape[1] != 3 or len(faces) == 0:
        return 0, 0
    edges = np.sort(np.vstack([faces[:, [0, 1]], faces[:, [1, 2]], faces[:, [2, 0]]]), axis=1)
    _, counts = np.unique(edges, axis=0, return_counts=True)
    return int((counts == 1).sum()), int((counts > 2).sum())


def _remove_degenerate_faces(mesh) -> None:
    """Drop degenerate faces; trimesh >= 5 removed the convenience method."""
    if hasattr(mesh, "remove_degenerate_faces"):
        mesh.remove_degenerate_faces()
        return
    if len(mesh.faces) == 0:
        return
    areas = np.asarray(mesh.area_faces, dtype=float)
    scale = float(np.max(np.abs(mesh.vertices))) if len(mesh.vertices) else 1.0
    # Mirrors validateMeshForMode: cross-product norm <= 1e-14 * scale means
    # face area <= 5e-15 * scale (unit-scale meshes use scale 1.0).
    keep = np.isfinite(areas) & (areas > 5e-15 * max(1.0, scale))
    if keep.all():
        return
    mesh.update_faces(keep)
    mesh.remove_unreferenced_vertices()


def _mesh_local_path(fn: str, repo_root: pathlib.Path, base: pathlib.Path) -> str:
    # Relative URDF mesh paths resolve against the URDF's directory, not the
    # process CWD; /workspace/-prefixed paths are the docker layout.
    if fn.startswith("/workspace/"):
        return str(repo_root / fn.removeprefix("/workspace/"))
    if pathlib.Path(fn).is_absolute():
        return fn
    return str(base / fn)


def prepare_visual_meshes(
    input_urdf: str | pathlib.Path,
    replace_pairs: Iterable[tuple[str, str]] | None = None,
    *,
    require_watertight: bool = False,
) -> tuple[list[tuple[str, str]], pathlib.Path | None, list[str], pathlib.Path, bool]:
    """Convert non-OBJ/STL visual meshes to OBJ and return replace pairs.

    Returns ``(merged_pairs, temp_dir, warnings, prepared_urdf, allow_open)``.
    The caller must keep ``temp_dir`` alive while the C++ extension runs and
    pass ``prepared_urdf`` as the URDF input to the extension.  ``prepared_urdf``
    is always the original path.  ``allow_open`` is true when a visual mesh
    remained non-watertight after repair; for modes that need a closed surface
    the caller should pass it to the C++ extension as ``allow_open_mesh`` so the
    true visual geometry is still used.
    """
    input_path = pathlib.Path(input_urdf)
    pairs = [(str(a), str(b)) for a, b in (replace_pairs or [])]
    warnings: list[str] = []
    try:
        tree = ET.parse(str(input_path))
    except Exception as exc:
        warnings.append(f"could not parse URDF for mesh preparation: {exc}")
        return pairs, None, warnings, input_path, False

    records: list[tuple[str, str]] = []
    seen: set[str] = set()
    for link in tree.iter("link"):
        link_name = link.get("name") or "<unnamed>"
        for vis in link.iter("visual"):
            mesh = vis.find(".//mesh")
            if mesh is None:
                continue
            fn = mesh.get("filename", "")
            if not fn or pathlib.Path(fn).suffix.lower() in {".obj", ".stl"}:
                continue
            if fn in seen:
                continue
            seen.add(fn)
            records.append((link_name, fn))
    if not records:
        return pairs, None, warnings, input_path, False

    import trimesh  # lazy: only required when non-OBJ/STL visuals exist

    repo_root = pathlib.Path(__file__).resolve().parents[2]
    base = input_path.resolve().parent
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="urdf_approx_visual_"))
    allow_open = False

    for link_name, fn in records:
        # A caller-supplied replacement already owns this path; the generated
        # OBJ below is ignored by the C++ replaceWith chain in that case.
        user_replaced = any(pair_fn == fn for pair_fn, _ in pairs)
        local_fn = _mesh_local_path(fn, repo_root, base)
        try:
            mesh = trimesh.load(local_fn, force="mesh")
            if mesh is None:
                warnings.append(f"failed to load {fn}")
                continue
            mesh.merge_vertices()
            _remove_degenerate_faces(mesh)
            mesh.fix_normals()
            if hasattr(mesh, "fill_holes") and not mesh.is_watertight:
                mesh.fill_holes()
            if require_watertight and not mesh.is_watertight and not user_replaced:
                boundary, nonmanifold = _edge_valence_stats(mesh)
                allow_open = True
                warnings.append(
                    f"link '{link_name}': visual mesh {fn} is not watertight after "
                    f"trimesh repair ({boundary} boundary edges, {nonmanifold} "
                    f"non-manifold edges); using the repaired open visual mesh for "
                    f"this fit (--mesh-source collision selects the legacy convex-hull fit)"
                )
            # Distinct source paths with the same stem (link1/mesh.dae vs
            # link2/mesh.dae) must not collide on the same output file.
            out = tmp / (f"{pathlib.Path(fn).stem}_{hashlib.sha1(fn.encode()).hexdigest()[:8]}.obj")
            mesh.export(out)
            pairs.append((fn, str(out)))
        except Exception as exc:
            warnings.append(f"failed to prepare {fn}: {exc}")

    return pairs, tmp, warnings, input_path, allow_open
