#!/usr/bin/env python3
"""Render a 2x2 qualitative overlay grid for FR3 link3.

Panels:
  [Original mesh]   [Convex hull overlay]
  [Sphere overlay]  [Capsule overlay]

Each panel shows the original visual mesh as gray wireframe with the
approximation geometry rendered as a colored translucent surface.

Usage:
    cd /home/admin1/projects/SpherizedURDFGenerator
    python3 doc/paper/artifacts/fig-qualitative-overlay/render_overlay_grid.py
"""

from __future__ import annotations

import json
import math
import os
import pathlib
import sys

# Fix matplotlib compatibility: system mpl_toolkits (3.5.1) conflicts with
# user-installed matplotlib 3.10+. Re-exec with -S if system mpl_toolkits
# would shadow the correct version.
if not sys.flags.no_site and os.path.exists("/usr/lib/python3/dist-packages/mpl_toolkits"):
    # Re-execute with -S flag to skip site module (which adds the system
    # dist-packages that contain the conflicting mpl_toolkits from
    # matplotlib 3.5.1). Include both user site-packages and local
    # dist-packages (where trimesh/numpy live) but NOT system dist-packages.
    _pp = "/home/admin1/.local/lib/python3.10/site-packages:/usr/local/lib/python3.10/dist-packages"
    os.execvpe(sys.executable, [sys.executable, "-S"] + sys.argv, {**os.environ, "PYTHONPATH": _pp})

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection, Line3DCollection
import numpy as np
import trimesh

REPO = pathlib.Path(__file__).resolve().parents[4]
FR3_URDF = REPO / "resources/fr3/urdf/fr3.urdf"
LINK_NAME = "fr3_link3"
WORK_DIR = REPO / "doc/paper/artifacts/exp-fr3-mode-comparison/work"
OUT_DIR = pathlib.Path(__file__).resolve().parent

# Which presets to use for each mode
CONVEX_PRESET = "default"
SPHERE_PRESET = "default"
CAPSULE_PRESET = "default"

VIEW_ELEV = 25
VIEW_AZIM = -60


def build_capsule_mesh(p0, p1, radius, segments=24):
    """Build a trimesh capsule between two endpoints."""
    p0 = np.asarray(p0, dtype=float)
    p1 = np.asarray(p1, dtype=float)
    axis = p1 - p0
    L = float(np.linalg.norm(axis))
    if L < 1e-9:
        m = trimesh.creation.uv_sphere(radius=radius)
        m.apply_translation(p0)
        return m
    cyl = trimesh.creation.cylinder(radius=radius, height=L, sections=segments)
    cap_p = trimesh.creation.uv_sphere(radius=radius)
    cap_p.apply_translation([0, 0, L / 2.0])
    cap_n = trimesh.creation.uv_sphere(radius=radius)
    cap_n.apply_translation([0, 0, -L / 2.0])
    m = trimesh.util.concatenate([cyl, cap_p, cap_n])
    mat = trimesh.geometry.align_vectors(np.array([0.0, 0, 1.0]), axis / L)
    m.apply_transform(mat)
    m.apply_translation((p0 + p1) / 2.0)
    return m


def load_visual_mesh():
    """Load the visual mesh for link3 (identity origin in URDF)."""
    fn = REPO / "resources/fr3/meshes/fr3/visual/link3.dae"
    mesh = trimesh.load(str(fn), force="mesh")
    if isinstance(mesh, trimesh.Scene):
        mesh = trimesh.util.concatenate(tuple(mesh.geometry.values()))
    return mesh


def make_convex_mesh(visual_mesh):
    """Compute convex hull of the visual mesh."""
    return visual_mesh.convex_hull


def load_sphere_meshes():
    """Load spheres from JSON and return list of trimesh sphere meshes."""
    json_path = WORK_DIR / f"fr3_sphere_{SPHERE_PRESET}.json"
    data = json.loads(json_path.read_text())
    body = data[LINK_NAME]
    spheres = body.get("spheres", [])
    meshes = []
    for s in spheres:
        c = np.array(s["center"], dtype=float)
        r = float(s["radius"])
        sphere_mesh = trimesh.creation.uv_sphere(radius=r)
        sphere_mesh.apply_translation(c)
        meshes.append(sphere_mesh)
    return meshes


def load_capsule_meshes():
    """Load capsules from JSON and return list of trimesh capsule meshes."""
    json_path = WORK_DIR / f"fr3_capsule_{CAPSULE_PRESET}.json"
    data = json.loads(json_path.read_text())
    body = data[LINK_NAME]
    capsules = body.get("capsules", [])
    meshes = []
    for cp in capsules:
        p0 = np.array(cp["p0"], dtype=float)
        p1 = np.array(cp["p1"], dtype=float)
        radius = float(cp["radius"])
        meshes.append(build_capsule_mesh(p0, p1, radius))
    return meshes


def _get_unique_edges(verts, faces):
    """Extract unique mesh edges as pairs of vertex indices."""
    edges = set()
    for face in faces:
        for i in range(3):
            a, b = int(face[i]), int(face[(i + 1) % 3])
            if a < b:
                edges.add((a, b))
            else:
                edges.add((b, a))
    return list(edges)


def panel_plot(
    ax, visual_mesh, approx_meshes, approx_color, title, all_verts, center, extent, edge_pairs
):
    """Render a single panel: gray wireframe + colored overlay."""
    verts = visual_mesh.vertices

    # Visual mesh wireframe — draw all edges as a single Line3DCollection
    segs = [np.array([verts[a], verts[b]]) for a, b in edge_pairs]
    lc = Line3DCollection(segs, colors="gray", alpha=0.25, linewidths=0.25)
    ax.add_collection3d(lc)

    # Colored overlay
    for mesh in approx_meshes:
        vm = mesh.vertices
        fm = mesh.faces
        poly = Poly3DCollection(
            vm[fm], alpha=0.40, facecolor=approx_color, edgecolor=approx_color, linewidth=0.15
        )
        ax.add_collection3d(poly)

    ax.set_xlim(center[0] - extent, center[0] + extent)
    ax.set_ylim(center[1] - extent, center[1] + extent)
    ax.set_zlim(center[2] - extent, center[2] + extent)
    ax.set_box_aspect([1, 1, 1])
    ax.view_init(elev=VIEW_ELEV, azim=VIEW_AZIM)
    ax.set_title(title, fontsize=13, pad=6)
    ax.set_xlabel("X", fontsize=9)
    ax.set_ylabel("Y", fontsize=9)
    ax.set_zlabel("Z", fontsize=9)
    ax.tick_params(labelsize=7)
    ax.grid(True, alpha=0.2)


def main():
    print("Loading visual mesh...")
    visual_mesh = load_visual_mesh()
    print(f"  {len(visual_mesh.vertices)} vertices, {len(visual_mesh.faces)} faces")

    print("Computing convex hull...")
    convex_mesh = make_convex_mesh(visual_mesh)
    print(f"  {len(convex_mesh.vertices)} vertices, {len(convex_mesh.faces)} faces")

    print("Loading spheres...")
    sphere_meshes = load_sphere_meshes()
    print(f"  {len(sphere_meshes)} spheres")

    print("Loading capsules...")
    capsule_meshes = load_capsule_meshes()
    print(f"  {len(capsule_meshes)} capsules")

    # Precompute unique edges for efficient wireframe rendering
    edge_pairs = _get_unique_edges(visual_mesh.vertices, visual_mesh.faces)
    print(f"  {len(edge_pairs)} unique edges")

    # Compute unified frame
    all_verts = visual_mesh.vertices
    center = all_verts.mean(axis=0)
    extent = all_verts.ptp(axis=0).max() / 2.0 + 0.03

    # Colors
    MESH_COLOR = "#555555"  # gray for original mesh
    CONVEX_COLOR = "#457B9D"  # blue
    SPHERE_COLOR = "#E63946"  # red
    CAPSULE_COLOR = "#2A9D8F"  # teal

    # Labels
    panel_titles = [
        "(a) Original visual mesh",
        "(b) Convex hull overlay",
        "(c) Sphere tree overlay",
        "(d) Capsule primitives overlay",
    ]

    approx_lists = [
        [],  # original mesh only (no overlay)
        [convex_mesh],  # convex hull
        sphere_meshes,  # spheres
        capsule_meshes,  # capsules
    ]

    approx_colors = [
        None,
        CONVEX_COLOR,
        SPHERE_COLOR,
        CAPSULE_COLOR,
    ]

    fig, axes = plt.subplots(2, 2, figsize=(12, 10), subplot_kw={"projection": "3d"})

    for idx, (ax, title, approx_list, color) in enumerate(
        zip(axes.flat, panel_titles, approx_lists, approx_colors)
    ):
        row, col = divmod(idx, 2)
        print(f"  Panel ({row+1},{col+1}): {title}")
        panel_plot(
            ax, visual_mesh, approx_list, color, title, all_verts, center, extent, edge_pairs
        )

    plt.subplots_adjust(left=0.02, right=0.98, bottom=0.02, top=0.95, wspace=0.08, hspace=0.15)

    output_png = OUT_DIR / "fig_qualitative_overlay.png"
    fig.savefig(str(output_png), dpi=200, bbox_inches="tight")
    print(f"\nFigure saved: {output_png}")

    # Also save PDF for paper
    output_pdf = OUT_DIR / "fig_qualitative_overlay.pdf"
    fig.savefig(str(output_pdf), dpi=200, bbox_inches="tight")
    print(f"Figure saved: {output_pdf}")

    plt.close(fig)


if __name__ == "__main__":
    main()
