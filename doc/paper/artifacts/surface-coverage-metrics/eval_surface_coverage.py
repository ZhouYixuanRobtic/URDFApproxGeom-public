#!/usr/bin/env python3
"""Surface-coverage evaluation for FR3 link approximations.

Samples points uniformly on the original mesh surface and computes signed
distance to each mode's collision primitives.  Produces per-link CSV rows
and a human-readable summary.

Modes evaluated:
  - convex/default
  - sphere/single, sphere/default
  - capsule/single, capsule/default, capsule/high_detail

Usage:
  python eval_surface_coverage.py
  # writes doc/paper/artifacts/surface-coverage-metrics/data/*.{csv,md}
"""

from __future__ import annotations

import csv
import json
import math
import os
import pathlib
import sys
import xml.etree.ElementTree as ET

import numpy as np
import trimesh
from scipy.spatial import ConvexHull as SciPyConvexHull

# ---------------------------------------------------------------------------
# paths
# ---------------------------------------------------------------------------
REPO = pathlib.Path(__file__).resolve().parents[4]  # repo root (worktree)
BUNDLE = REPO / "assets/fr3_compare_bundle"
FR3_MESHES = REPO / "resources/fr3/meshes/fr3"
OUT = REPO / "doc/paper/artifacts/surface-coverage-metrics"

SEED = 20260705
SURFACE_SAMPLES_PER_LINK = 12_000  # target; actual may be lower on tiny meshes

# ---------------------------------------------------------------------------
# geometry helpers
# ---------------------------------------------------------------------------


def rpy_to_R(roll: float, pitch: float, yaw: float) -> np.ndarray:
    cr, sr = math.cos(roll), math.sin(roll)
    cp, sp = math.cos(pitch), math.sin(pitch)
    cy, sy = math.cos(yaw), math.sin(yaw)
    Rx = np.array([[1, 0, 0], [0, cr, -sr], [0, sr, cr]])
    Ry = np.array([[cp, 0, sp], [0, 1, 0], [-sp, 0, cp]])
    Rz = np.array([[cy, -sy, 0], [sy, cy, 0], [0, 0, 1]])
    return Rz @ Ry @ Rx


def parse_origin(elem: ET.Element) -> tuple[np.ndarray, np.ndarray]:
    """Return (xyz, rotation_matrix) from an XML origin element (or defaults)."""
    xyz = np.zeros(3, dtype=float)
    rpy = np.zeros(3, dtype=float)
    if elem is not None:
        if elem.get("xyz"):
            xyz = np.array([float(v) for v in elem.get("xyz", "").split()], dtype=float)
        if elem.get("rpy"):
            rpy = np.array([float(v) for v in elem.get("rpy", "").split()], dtype=float)
    return xyz, rpy_to_R(*rpy)


# ---------------------------------------------------------------------------
# capsule primitive helpers
# ---------------------------------------------------------------------------


def _cylinder_to_capsule(origin_xyz, origin_R, radius, length):
    """Convert a URDF <cylinder> at origin to capsule endpoints in link frame."""
    half = length / 2.0
    p0 = origin_xyz + origin_R @ np.array([0.0, 0.0, -half])
    p1 = origin_xyz + origin_R @ np.array([0.0, 0.0, half])
    return p0, p1, radius


def signed_distance_to_capsule(point, p0, p1, radius):
    """Signed distance from point to capsule (line segment + radius)."""
    axis = p1 - p0
    denom = float(axis @ axis)
    if denom < 1e-12:
        closest = p0
    else:
        t = np.clip(float((point - p0) @ axis / denom), 0.0, 1.0)
        closest = p0 + t * axis
    return float(np.linalg.norm(point - closest) - radius)


def signed_distance_to_sphere(point, center, radius):
    """Signed distance from point to sphere."""
    return float(np.linalg.norm(point - center) - radius)


# ---------------------------------------------------------------------------
# URDF parsers
# ---------------------------------------------------------------------------


def parse_generated_urdf(urdf_path: str):
    """Parse a generated comparison URDF.

    Returns dict[mode_name, dict[link_name, list[(primitive_type, params...)]]]

    primitive_type is one of:
      ("capsule", p0, p1, radius)   -- reconstructed from <cylinder>
      ("sphere", center, radius)     -- from <sphere>
      ("convex_mesh", mesh_path)     -- from <mesh> for convex mode
    """
    tree = ET.parse(urdf_path)
    root = tree.getroot()
    links: dict[str, list] = {}

    for link in root.iter("link"):
        name = link.get("name")
        if name is None:
            continue
        primitives: list = []
        for collision in link.iter("collision"):
            origin_elem = collision.find("origin")
            xyz, R = parse_origin(origin_elem)

            geom = collision.find("geometry")
            if geom is None:
                continue

            cyl = geom.find("cylinder")
            if cyl is not None:
                radius = float(cyl.get("radius", "0"))
                length = float(cyl.get("length", "0"))
                p0, p1, cap_r = _cylinder_to_capsule(xyz, R, radius, length)
                primitives.append(("capsule", p0, p1, cap_r))

            sphere = geom.find("sphere")
            if sphere is not None:
                radius = float(sphere.get("radius", "0"))
                primitives.append(("sphere", xyz, radius))

            mesh = geom.find("mesh")
            if mesh is not None:
                fn = mesh.get("filename", "")
                # Resolve relative to bundle dir
                mesh_path = str(pathlib.Path(urdf_path).parent / fn)
                primitives.append(("convex_mesh", mesh_path))

        if primitives:
            links[name] = primitives

    return links


# ---------------------------------------------------------------------------
# convex hull from visual mesh (fallback when OBJ not in bundle)
# ---------------------------------------------------------------------------

# Cache: link_visual_path -> (scipy ConvexHull equations, None|exc)
# Using scipy's half-space representation for correct signed distance
# (trimesh ProximityQuery.signed_distance can flip sign on non-watertight meshes).
_convex_cache: dict[str, SciPyConvexHull] = {}


def _convex_hull_for_link(link_visual_mesh_path: str) -> SciPyConvexHull | None:
    """Compute convex hull of a visual mesh (cached per path).

    Returns the scipy.spatial.ConvexHull whose `.equations` give the
    correct signed distance: max_i (n_i·p + d_i), negative = inside.
    """
    if link_visual_mesh_path in _convex_cache:
        return _convex_cache[link_visual_mesh_path]
    try:
        m = trimesh.load(link_visual_mesh_path, force="mesh")
        if isinstance(m, trimesh.Scene):
            m = trimesh.util.concatenate(tuple(m.geometry.values()))
        hull = SciPyConvexHull(m.vertices)
        _convex_cache[link_visual_mesh_path] = hull
        return hull
    except Exception as exc:
        print(f"  WARN: convex hull failed for {link_visual_mesh_path}: {exc}", file=sys.stderr)
        _convex_cache[link_visual_mesh_path] = exc
        return None


def _signed_distance_to_convex_hull(points: np.ndarray, hull: SciPyConvexHull) -> np.ndarray:
    """Signed distance of points to convex hull using half-space representation.

    hull.equations is (N_faces, 4): [A, B, C, D] for A*x + B*y + C*z + D = 0,
    with interior on the negative side.

    Returns (N,) array: negative = inside, zero = on surface, positive = outside.
    """
    if len(points) == 0:
        return np.array([], dtype=float)
    A, B, C, D = (
        hull.equations[:, 0],
        hull.equations[:, 1],
        hull.equations[:, 2],
        hull.equations[:, 3],
    )
    # (N_points, N_faces) signed distances per face
    signed = (
        A[np.newaxis, :] * points[:, 0:1]
        + B[np.newaxis, :] * points[:, 1:2]
        + C[np.newaxis, :] * points[:, 2:3]
        + D[np.newaxis, :]
    )
    return signed.max(axis=1)


# ---------------------------------------------------------------------------
# main evaluation
# ---------------------------------------------------------------------------


def evaluate_mode_preset(mode: str, preset: str, urdf_path: pathlib.Path) -> list[dict]:
    """Evaluate surface coverage for one mode/preset combination.

    Returns list of per-link result dicts.
    """
    print(f"\n{'='*72}")
    print(f"Mode: {mode}, preset: {preset}")
    print(f"URDF: {urdf_path}")
    print(f"{'='*72}")

    if not urdf_path.exists():
        print(f"  SKIP: URDF not found")
        return []

    parsed = parse_generated_urdf(str(urdf_path))
    if not parsed:
        print(f"  SKIP: no collision primitives found")
        return []

    rng = np.random.default_rng(SEED)
    results: list[dict] = []

    for link_name in sorted(parsed.keys()):
        primitives = parsed[link_name]

        # -- Load visual mesh for surface sampling --
        # The visual mesh file name matches mesh_N.dae (from compare bundle)
        # OR resources/fr3/meshes/fr3/visual/linkN.dae
        # Extract link number from the generated URDF link name
        visual_path = _resolve_visual_mesh(link_name)
        if visual_path is None or not visual_path.exists():
            print(f"  SKIP {link_name}: visual mesh not found ({visual_path})")
            continue

        try:
            visual_mesh = trimesh.load(str(visual_path), force="mesh")
            if isinstance(visual_mesh, trimesh.Scene):
                visual_mesh = trimesh.util.concatenate(tuple(visual_mesh.geometry.values()))
        except Exception as exc:
            print(f"  SKIP {link_name}: cannot load visual mesh: {exc}")
            continue

        if visual_mesh.is_empty or len(visual_mesh.faces) == 0:
            print(f"  SKIP {link_name}: empty visual mesh")
            continue

        # -- Sample surface --
        area = visual_mesh.area
        n_faces = len(visual_mesh.faces)
        # Don't sample more than faces * 20 (avoid excessive per-face repeats)
        n_samples = min(SURFACE_SAMPLES_PER_LINK, max(1000, n_faces * 5))
        # Also make sure we have enough samples for the area
        n_samples = max(n_samples, 500)

        # ponytail: trimesh.sample.sample_surface uses area-weighted selection,
        # which is deterministic when seeded through the rng we pass.
        surface_points, face_indices = trimesh.sample.sample_surface(
            visual_mesh, n_samples, seed=rng
        )

        # -- Compute signed distances --
        distances = _signed_distances_to_primitives(surface_points, primitives, mode, visual_path)

        worst_dist = float(np.max(distances))
        p95_dist = float(np.percentile(distances, 95))
        uncovered = float(np.mean(distances > 1e-9))  # fraction > 0 (tolerance for fp)

        # Count primitives (rough: capsule = 1, sphere = 1, convex_mesh = 1)
        primitive_count = _count_primitives(primitives)

        # Load original collision mesh for vertex-level comparison too
        vertex_worst = _vertex_worst_distance(link_name, primitives, mode)

        results.append(
            {
                "mode": mode,
                "preset": preset,
                "link": link_name,
                "primitive_count": primitive_count,
                "vertex_worst_signed_distance_m": vertex_worst,
                "surface_sample_count": n_samples,
                "surface_worst_signed_distance_m": worst_dist,
                "surface_p95_signed_distance_m": p95_dist,
                "surface_uncovered_fraction": uncovered,
                "random_seed": SEED,
            }
        )

        cov_status = "COVERED" if worst_dist <= 1e-9 else f"UNCOVERED ({worst_dist:.6f} m)"
        print(
            f"  {link_name:20}  prims={primitive_count:2}  samples={n_samples:5}  "
            f"worst={worst_dist:.6f}  p95={p95_dist:.6f}  "
            f"uncovered={uncovered:.4f}  [{cov_status}]"
        )

    return results


def _resolve_visual_mesh(link_name: str) -> pathlib.Path | None:
    """Map a link name to the visual mesh path.

    The compare bundle has link0.dae ... link7.dae, hand.dae, flex_griper_connect.stl
    """
    if link_name.startswith("fr3_link"):
        try:
            idx = int(link_name.replace("fr3_link", "").split("_")[0])
        except ValueError:
            return None
        # Try bundle meshes first, then resources
        p = BUNDLE / "meshes" / f"link{idx}.dae"
        if p.exists():
            return p
        p = FR3_MESHES / "visual" / f"link{idx}.dae"
        if p.exists():
            return p
        p = FR3_MESHES / "collision" / f"link{idx}.stl"
        return p if p.exists() else None
    elif link_name == "hand_hand":
        p = BUNDLE / "meshes" / "hand.dae"
        if p.exists():
            return p
        return None
    elif link_name in ("hand_leftfinger", "hand_rightfinger"):
        p = BUNDLE / "meshes" / "flex_griper_connect.stl"
        return p if p.exists() else None
    return None


def _signed_distances_to_primitives(
    points: np.ndarray,
    primitives: list,
    mode: str,
    visual_path: pathlib.Path,
) -> np.ndarray:
    """Compute minimum signed distance for each point across all primitives."""
    n = points.shape[0]
    dists = np.full(n, float("inf"), dtype=float)

    for prim in primitives:
        ptype = prim[0]

        if ptype == "capsule":
            _, p0, p1, radius = prim
            axis = p1 - p0
            denom = float(axis @ axis)
            if denom < 1e-12:
                closest = np.tile(p0, (n, 1))
            else:
                t = np.clip((points - p0) @ axis / denom, 0.0, 1.0)
                closest = p0 + t[:, None] * axis
            d = np.linalg.norm(points - closest, axis=1) - radius
            np.minimum(dists, d, out=dists)

        elif ptype == "sphere":
            _, center, radius = prim
            d = np.linalg.norm(points - center, axis=1) - radius
            np.minimum(dists, d, out=dists)

        elif ptype == "convex_mesh":
            hull = _convex_hull_for_link(str(visual_path))
            if hull is None:
                continue
            d = _signed_distance_to_convex_hull(points, hull)
            np.minimum(dists, d, out=dists)

    return dists


def _count_primitives(primitives: list) -> int:
    count = 0
    for p in primitives:
        t = p[0]
        # capsule mode: 1 capsule = 1 primitive
        if t == "capsule":
            count += 1
        elif t == "sphere":
            count += 1
        elif t == "convex_mesh":
            count += 1
    return count


def _vertex_worst_distance(link_name: str, primitives: list, mode: str) -> float:
    """Compute the worst signed distance at mesh vertices (for comparison)."""
    # Load the collision STL mesh for vertex positions
    mesh_path = FR3_MESHES / "collision" / f"{link_name.replace('fr3_', '')}.stl"
    if not mesh_path.exists():
        return float("nan")

    try:
        m = trimesh.load(str(mesh_path), force="mesh")
        if isinstance(m, trimesh.Scene):
            m = trimesh.util.concatenate(tuple(m.geometry.values()))
        verts = m.vertices
    except Exception:
        return float("nan")

    if len(verts) == 0:
        return float("nan")

    dists = np.full(len(verts), float("inf"), dtype=float)
    for prim in primitives:
        ptype = prim[0]
        if ptype == "capsule":
            _, p0, p1, radius = prim
            axis = p1 - p0
            denom = float(axis @ axis)
            if denom < 1e-12:
                closest = np.tile(p0, (len(verts), 1))
            else:
                t = np.clip((verts - p0) @ axis / denom, 0.0, 1.0)
                closest = p0 + t[:, None] * axis
            d = np.linalg.norm(verts - closest, axis=1) - radius
            np.minimum(dists, d, out=dists)
        elif ptype == "sphere":
            _, center, radius = prim
            d = np.linalg.norm(verts - center, axis=1) - radius
            np.minimum(dists, d, out=dists)
        elif ptype == "convex_mesh":
            pass  # handled separately

    if mode == "convex":
        visual_path = _resolve_visual_mesh(link_name)
        if visual_path and visual_path.exists():
            hull = _convex_hull_for_link(str(visual_path))
            if hull is not None:
                d = _signed_distance_to_convex_hull(verts, hull)
                np.minimum(dists, d, out=dists)

    valid = dists[np.isfinite(dists)]
    return float(valid.max()) if len(valid) > 0 else float("nan")


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------


def main():
    PRESETS = [
        ("convex", "default"),
        ("sphere", "single"),
        ("sphere", "default"),
        ("capsule", "single"),
        ("capsule", "default"),
        ("capsule", "high_detail"),
    ]

    all_rows: list[dict] = []

    for mode, preset in PRESETS:
        # Check comparison bundle URDF first
        urdf_path = BUNDLE / f"fr3_{mode}_{preset}.urdf"
        # Also check alternate naming
        if not urdf_path.exists() and mode == "convex":
            urdf_path = BUNDLE / "fr3_convex.urdf"
        if not urdf_path.exists():
            print(f"  SKIP: URDF not found at {urdf_path}")
            continue
        rows = evaluate_mode_preset(mode, preset, urdf_path)
        all_rows.extend(rows)

    # -- Write CSV --
    csv_path = OUT / "data" / "fr3_surface_coverage.csv"
    fieldnames = [
        "mode",
        "preset",
        "link",
        "primitive_count",
        "vertex_worst_signed_distance_m",
        "surface_sample_count",
        "surface_worst_signed_distance_m",
        "surface_p95_signed_distance_m",
        "surface_uncovered_fraction",
        "random_seed",
    ]
    with open(csv_path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        w.writerows(all_rows)
    print(f"\nWrote {csv_path}  ({len(all_rows)} rows)")

    # -- Write summary --
    summary_md = _build_summary(all_rows)
    summary_path = OUT / "data" / "fr3_surface_coverage_summary.md"
    summary_path.write_text(summary_md)
    print(f"Wrote {summary_path}")

    print("\nDone.")


def _build_summary(rows: list[dict]) -> str:
    """Generate a human-readable summary markdown."""

    # Group by (mode, preset)
    groups: dict[tuple[str, str], list[dict]] = {}
    for r in rows:
        key = (r["mode"], r["preset"])
        groups.setdefault(key, []).append(r)

    lines = []
    lines.append("# FR3 Surface Coverage Summary")
    lines.append("")
    lines.append(f"Random seed: {SEED}")
    lines.append(f"Target samples per link: {SURFACE_SAMPLES_PER_LINK}")
    lines.append("")

    # Overview table
    lines.append("## Aggregate by Mode/Preset")
    lines.append("")
    lines.append(
        "| Mode | Preset | Links | Primitives | Surface Samples | Worst SD (m) | P95 SD (m) | Uncovered Frac |"
    )
    lines.append(
        "|------|--------|------:|-----------:|----------------:|-------------:|-----------:|---------------:|"
    )

    for key in sorted(groups.keys()):
        mode, preset = key
        grp = groups[key]
        n_links = len(grp)
        total_prims = sum(r["primitive_count"] for r in grp)
        total_samples = sum(r["surface_sample_count"] for r in grp)
        worst_worst = max(r["surface_worst_signed_distance_m"] for r in grp)
        worst_p95 = max(r["surface_p95_signed_distance_m"] for r in grp)
        overall_uncovered = (
            sum(r["surface_uncovered_fraction"] * r["surface_sample_count"] for r in grp)
            / total_samples
            if total_samples > 0
            else 0
        )
        lines.append(
            f"| {mode} | {preset} | {n_links} | {total_prims} | {total_samples} | "
            f"{worst_worst:.6f} | {worst_p95:.6f} | {overall_uncovered:.4f} |"
        )

    lines.append("")

    # Per-mode detail
    for key in sorted(groups.keys()):
        mode, preset = key
        grp = groups[key]
        lines.append(f"## {mode}/{preset}")
        lines.append("")
        lines.append(
            "| Link | Prims | Samples | Worst SD (m) | P95 SD (m) | Uncovered Frac | Vertex Worst (m) |"
        )
        lines.append(
            "|------|------:|-------:|-------------:|-----------:|---------------:|-----------------:|"
        )
        for r in sorted(grp, key=lambda x: x["link"]):
            vw = r["vertex_worst_signed_distance_m"]
            vw_str = f"{vw:.6f}" if not math.isnan(vw) else "N/A"
            cov = "**UNCOVERED**" if r["surface_worst_signed_distance_m"] > 1e-9 else "covered"
            lines.append(
                f"| {r['link']} | {r['primitive_count']} | {r['surface_sample_count']} | "
                f"{r['surface_worst_signed_distance_m']:.6f} | "
                f"{r['surface_p95_signed_distance_m']:.6f} | "
                f"{r['surface_uncovered_fraction']:.4f} | {vw_str} |"
            )
        lines.append("")

    # Cross-mode comparison
    lines.append("## Cross-Mode Comparison")
    lines.append("")
    lines.append("### Worst-case uncovered distance by mode")
    lines.append("")
    for key in sorted(groups.keys()):
        mode, preset = key
        grp = groups[key]
        worst = max(r["surface_worst_signed_distance_m"] for r in grp)
        total_uncovered = sum(
            r["surface_uncovered_fraction"] * r["surface_sample_count"] for r in grp
        )
        total_s = sum(r["surface_sample_count"] for r in grp)
        frac = total_uncovered / total_s if total_s > 0 else 0
        lines.append(
            f"- **{mode}/{preset}**: worst SD = {worst:.6f} m, uncovered fraction = {frac:.4f}"
        )

    lines.append("")
    lines.append("### Surface vs Vertex coverage")
    lines.append("")
    # Compare the two metrics across all rows
    surface_worse = 0
    vertex_worse = 0
    for r in rows:
        sw = r["surface_worst_signed_distance_m"]
        vw = r["vertex_worst_signed_distance_m"]
        if math.isnan(vw):
            continue
        if sw > vw + 1e-9:
            surface_worse += 1
        elif vw > sw + 1e-9:
            vertex_worse += 1
    lines.append(
        f"- Surface sampling finds **worse** uncovered distance in **{surface_worse}** cases"
    )
    lines.append(f"- Vertex-only finds **worse** uncovered distance in **{vertex_worse}** cases")
    lines.append("")

    return "\n".join(lines)


if __name__ == "__main__":
    main()
