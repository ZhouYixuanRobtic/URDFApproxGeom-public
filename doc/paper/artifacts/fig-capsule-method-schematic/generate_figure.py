#!/usr/bin/env python3
"""Generate the 4-panel capsule method schematic figure.

Panels:
  1. Input mesh with PCA principal axis overlaid.
  2. Cross-section planes perpendicular to the PCA axis.
  3. 2D view of one cross-section contour with fitted covering circles.
  4. Resulting capsule chain in 3D overlaid on the original mesh.

Output: capsule_method_schematic.pdf, capsule_method_schematic.png
"""

from __future__ import annotations

import json
import math
import os
import pathlib
import sys

# ── matplotlib compatibility ─────────────────────────────────────────────────
# Prefer system matplotlib (3.5.1) over pip one (3.10, missing deps).
import sys as _sys

_USER_LOCAL = "/home/admin1/.local/lib/python3.10/site-packages"
if _USER_LOCAL in _sys.path:
    _sys.path.remove(_USER_LOCAL)
for _p in list(_sys.path):
    if _p == "/usr/lib/python3/dist-packages":
        _sys.path.remove(_p)
        _sys.path.insert(0, _p)
_sys.path.append(_USER_LOCAL)

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Circle, Polygon as MPLPolygon
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import numpy as np
import trimesh

# ── paths ──────────────────────────────────────────────────────────────────
REPO = pathlib.Path(__file__).resolve().parents[4]
assert REPO.name == "SpherizedURDFGenerator", f"unexpected repo root: {REPO}"
MESH_FILE = REPO / "resources/fr3/meshes/fr3/collision/collision/link3.obj"
CAPS_JSON = REPO / "resources/fr3/urdf/fr3_capsuleized.json"
OUT_DIR = pathlib.Path(__file__).resolve().parent

# Representative link name in the capsule JSON
LINK_NAME = "fr3_link3"

# ── helpers ────────────────────────────────────────────────────────────────


def pca_axis(vertices: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """PCA principal axis via covariance eigendecomposition.

    Returns (origin, axis, eigenvalues) where axis is the eigenvector
    corresponding to the largest eigenvalue.
    """
    origin = vertices.mean(axis=0)
    Cn = vertices - origin
    cov = (Cn.T @ Cn) / vertices.shape[0]
    eigvals, eigvecs = np.linalg.eigh(cov)
    axis = eigvecs[:, 2]  # largest eigenvalue
    return origin, axis, eigvals


def mesh_triangle_plane_intersection(
    V: np.ndarray,
    F: np.ndarray,
    plane_origin: np.ndarray,
    plane_normal: np.ndarray,
) -> list[np.ndarray]:
    """Intersect a triangle mesh with a plane, returning line segments.

    For each triangle that straddles the plane, compute the intersection
    segment. Returns list of (2, 3) arrays (segment endpoints).
    """
    segments = []
    eps = 1e-9
    for face in F:
        v = V[face]  # (3, 3)
        d = np.dot(v - plane_origin, plane_normal)  # signed distances
        lo = d.min()
        hi = d.max()
        if lo > eps or hi < -eps:
            continue  # triangle does not straddle plane
        pts = []
        for e in range(3):
            p = v[e]
            q = v[(e + 1) % 3]
            dp = d[e]
            dq = d[(e + 1) % 3]
            if abs(dq - dp) < 1e-18:
                continue
            if (dp <= 0 and dq >= 0) or (dp >= 0 and dq <= 0):
                t = (0 - dp) / (dq - dp)
                pts.append(p + t * (q - p))
        if len(pts) >= 2:
            segments.append(np.array(pts[:2]))
    return segments


def chain_segments(
    segments: list[np.ndarray],
) -> list[np.ndarray]:
    """Chain 3D line segments into ordered contours (list of (N, 3))."""
    if not segments:
        return []
    used = [False] * len(segments)
    contours = []
    eps = 1e-6
    for s in range(len(segments)):
        if used[s]:
            continue
        used[s] = True
        contour = [segments[s][0], segments[s][1]]
        cur = segments[s][1]
        start = segments[s][0]
        extended = True
        while extended:
            extended = False
            for j in range(len(segments)):
                if used[j]:
                    continue
                A, B = segments[j]
                if np.linalg.norm(A - cur) <= eps:
                    nxt = B
                elif np.linalg.norm(B - cur) <= eps:
                    nxt = A
                else:
                    continue
                used[j] = True
                if np.linalg.norm(nxt - start) <= eps:
                    break  # loop closed
                contour.append(nxt)
                cur = nxt
                extended = True
                break
        contours.append(np.array(contour))
    return contours


def project_to_2d(
    points: np.ndarray,
    plane_origin: np.ndarray,
    plane_u: np.ndarray,
    plane_v: np.ndarray,
) -> np.ndarray:
    """Project 3D points onto a 2D plane defined by orthonormal basis u, v."""
    d = points - plane_origin
    return np.column_stack([np.dot(d, plane_u), np.dot(d, plane_v)])


def minimum_enclosing_circle_2d(
    pts: np.ndarray,
) -> tuple[np.ndarray, float]:
    """Compute minimum enclosing circle for a set of 2D points.

    Uses Welzl's algorithm (move-to-front variant). Returns (center, radius).
    """
    P = [np.asarray(p, dtype=float) for p in pts]
    if not P:
        return np.zeros(2), 0.0
    if len(P) == 1:
        return P[0], 0.0

    # Deterministic shuffle for reproducibility
    rng = np.random.RandomState(0xC0FFEE)
    rng.shuffle(P)

    def _from2(a, b):
        return 0.5 * (a + b), 0.5 * np.linalg.norm(a - b)

    def _from3(a, b, c):
        A = np.array([[a[0], a[1], 1], [b[0], b[1], 1], [c[0], c[1], 1]], dtype=float)
        det = np.linalg.det(A)
        if abs(det) < 1e-18:
            # Collinear: pick the pair with largest distance
            d_ab = np.linalg.norm(a - b)
            d_ac = np.linalg.norm(a - c)
            d_bc = np.linalg.norm(b - c)
            if d_ab >= d_ac and d_ab >= d_bc:
                return _from2(a, b)
            elif d_ac >= d_bc:
                return _from2(a, c)
            else:
                return _from2(b, c)
        D = 2 * det
        ux = (
            np.linalg.norm(a) ** 2 * (b[1] - c[1])
            + np.linalg.norm(b) ** 2 * (c[1] - a[1])
            + np.linalg.norm(c) ** 2 * (a[1] - b[1])
        ) / D
        uy = (
            np.linalg.norm(a) ** 2 * (c[0] - b[0])
            + np.linalg.norm(b) ** 2 * (a[0] - c[0])
            + np.linalg.norm(c) ** 2 * (b[0] - a[0])
        ) / D
        center = np.array([ux, uy])
        return center, np.linalg.norm(a - center)

    C = (np.zeros(2), -1.0)
    for i in range(len(P)):
        if C[1] < 0 or np.linalg.norm(P[i] - C[0]) > C[1] + 1e-12:
            C = (P[i], 0.0)
            for j in range(i):
                if np.linalg.norm(P[j] - C[0]) > C[1] + 1e-12:
                    C = _from2(P[i], P[j])
                    for k in range(j):
                        if np.linalg.norm(P[k] - C[0]) > C[1] + 1e-12:
                            C = _from3(P[i], P[j], P[k])
    if C[1] < 0:
        C = (np.zeros(2), 0.0)
    return C


def build_capsule_mesh(
    p0: np.ndarray,
    p1: np.ndarray,
    radius: float,
    segments: int = 24,
) -> trimesh.Trimesh:
    """Build a trimesh capsule spanning p0->p1 with given radius."""
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
    cap_p.apply_translation(np.array([0, 0, L / 2.0]))
    cap_n = trimesh.creation.uv_sphere(radius=radius)
    cap_n.apply_translation(np.array([0, 0, -L / 2.0]))
    mesh = trimesh.util.concatenate([cyl, cap_p, cap_n])
    mat = trimesh.geometry.align_vectors(np.array([0.0, 0, 1.0]), axis / L)
    mesh.apply_transform(mat)
    mesh.apply_translation((p0 + p1) / 2.0)
    return mesh


def get_link_transform(urdf_path: str | pathlib.Path, link_name: str):
    """Get visual mesh origin for a link from URDF.

    Returns (filename, translation, rotation_matrix).
    """
    import xml.etree.ElementTree as ET

    tree = ET.parse(str(urdf_path))
    for link in tree.iter("link"):
        name = link.get("name")
        if name != link_name:
            continue
        visual = link.find("visual")
        if visual is None:
            continue
        mesh_elem = visual.find("geometry/mesh")
        if mesh_elem is None:
            continue
        fn = mesh_elem.get("filename")
        if not fn:
            continue
        if fn.startswith("/workspace/"):
            fn = str(REPO / fn.removeprefix("/workspace/"))
        origin = visual.find("origin")
        xyz = np.array([0.0, 0.0, 0.0])
        rpy = np.array([0.0, 0.0, 0.0])
        if origin is not None:
            if origin.get("xyz"):
                xyz = np.array([float(v) for v in origin.get("xyz").split()])
            if origin.get("rpy"):
                rpy = np.array([float(v) for v in origin.get("rpy").split()])
        cr, sr = math.cos(rpy[0]), math.sin(rpy[0])
        cp, sp = math.cos(rpy[1]), math.sin(rpy[1])
        cy, sy = math.cos(rpy[2]), math.sin(rpy[2])
        Rx = np.array([[1, 0, 0], [0, cr, -sr], [0, sr, cr]])
        Ry = np.array([[cp, 0, sp], [0, 1, 0], [-sp, 0, cp]])
        Rz = np.array([[cy, -sy, 0], [sy, cy, 0], [0, 0, 1]])
        R = Rz @ Ry @ Rx
        return fn, xyz, R
    return None, None, None


def load_mesh(fn: str | pathlib.Path) -> trimesh.Trimesh:
    """Load a mesh file, flattening scenes."""
    loaded = trimesh.load(str(fn), force="mesh")
    if isinstance(loaded, trimesh.Scene):
        loaded = trimesh.util.concatenate(tuple(loaded.geometry.values()))
    return loaded


# ── main ───────────────────────────────────────────────────────────────────


def main():
    np.random.seed(42)

    # ── Load mesh ───────────────────────────────────────────────────────────
    print(f"Loading mesh: {MESH_FILE}")
    mesh = load_mesh(MESH_FILE)
    V = mesh.vertices
    F = mesh.faces
    print(f"  vertices={V.shape}, faces={F.shape}")

    # Get URDF transform
    urdf_path = REPO / "resources/fr3/urdf/fr3.urdf"
    vis_fn, vis_T, vis_R = get_link_transform(urdf_path, LINK_NAME)
    print(f"  visual mesh: {vis_fn}")
    print(f"  origin: T={vis_T}")

    # ── PCA axis (replicating C++ CapsuleCrossSection.cpp logic) ────────────
    origin, axis, eigvals = pca_axis(V)
    axis = axis * np.sign(axis[2]) if abs(axis[2]) > 1e-6 else axis
    print(f"PCA axis: {axis}")
    print(f"eigenvalues: {eigvals}")

    # Orthonormal basis for projecting plane coordinates
    ref = np.array([1, 0, 0]) if abs(axis[0]) < 0.9 else np.array([0, 1, 0])
    e1 = np.cross(axis, ref)
    e1 /= np.linalg.norm(e1)
    e2 = np.cross(axis, e1)
    e2 /= np.linalg.norm(e2)

    # Project vertex extents along axis
    t_vals = np.dot(V - origin, axis)
    tmin, tmax = t_vals.min(), t_vals.max()
    print(f"  axis extent: [{tmin:.4f}, {tmax:.4f}]")

    # ── Cross-section contours (replicating C++ extractSections) ────────────
    N_SECTIONS = 8
    plane_ts = np.linspace(tmin, tmax, N_SECTIONS + 2)[1:-1]  # skip ends

    section_contours_3d = []  # list of list of (N,3) arrays
    section_contours_2d = []
    for tk in plane_ts:
        plane_origin = origin + tk * axis
        segments = mesh_triangle_plane_intersection(V, F, plane_origin, axis)
        contours_3d = chain_segments(segments)
        contours_2d = []
        for c in contours_3d:
            c2d = project_to_2d(c, plane_origin, e1, e2)
            contours_2d.append(c2d)
        section_contours_3d.append(contours_3d)
        section_contours_2d.append(contours_2d)

    # ── Fitted circles on one cross-section (panel 3) ───────────────────────
    # Use the middle cross-section for the 2D panel
    mid_idx = len(plane_ts) // 2
    target_contours_2d = section_contours_2d[mid_idx]
    # Sample points from the contour
    all_pts_2d = []
    for c in target_contours_2d:
        all_pts_2d.extend(c.tolist())
    all_pts_2d = np.array(all_pts_2d)

    # Fit minimum enclosing circle (MEC)
    mec_center, mec_radius = minimum_enclosing_circle_2d(all_pts_2d)

    # ── Load capsule JSON ───────────────────────────────────────────────────
    print(f"Loading capsule JSON: {CAPS_JSON}")
    caps_data = json.loads(CAPS_JSON.read_text())
    link_caps = caps_data.get(LINK_NAME, {}).get("capsules", [])
    print(f"  {len(link_caps)} capsules for {LINK_NAME}")

    # ── Build figure ────────────────────────────────────────────────────────
    plt.rcParams.update(
        {
            "font.family": "serif",
            "font.serif": ["DejaVu Serif", "Times New Roman", "Times"],
            "font.size": 10,
            "axes.labelsize": 11,
            "axes.titlesize": 12,
            "xtick.labelsize": 9,
            "ytick.labelsize": 9,
            "legend.fontsize": 9,
        }
    )

    fig = plt.figure(figsize=(16, 11))
    gs = fig.add_gridspec(2, 2, hspace=0.25, wspace=0.15)

    PANEL_LABELS = ["(a)", "(b)", "(c)", "(d)"]

    # ── Panel 1: Mesh + PCA axis ────────────────────────────────────────────
    print("Panel 1: Mesh + PCA axis")
    ax1 = fig.add_subplot(gs[0, 0], projection="3d")

    # Plot mesh as translucent surface with visible edges
    mesh_collection = Poly3DCollection(
        V[F], alpha=0.65, facecolor="#78909C", edgecolor="#455A64", linewidth=0.3
    )
    ax1.add_collection3d(mesh_collection)

    # PCA axis arrow
    axis_len = (tmax - tmin) * 0.85
    arrow_start = origin - 0.45 * axis_len * axis
    arrow_end = origin + 0.55 * axis_len * axis
    ax1.quiver(
        arrow_start[0],
        arrow_start[1],
        arrow_start[2],
        (arrow_end - arrow_start)[0],
        (arrow_end - arrow_start)[1],
        (arrow_end - arrow_start)[2],
        color="#D32F2F",
        linewidth=3.0,
        arrow_length_ratio=0.18,
        label="PCA principal axis",
    )

    # Set limits
    all_pts = V
    center = all_pts.mean(axis=0)
    extent = all_pts.ptp(axis=0).max() / 2.0 + 0.05
    ax1.set_xlim(center[0] - extent, center[0] + extent)
    ax1.set_ylim(center[1] - extent, center[1] + extent)
    ax1.set_zlim(center[2] - extent, center[2] + extent)
    ax1.set_box_aspect([1, 1, 1])
    ax1.view_init(elev=25, azim=-55)

    ax1.set_xlabel("X", labelpad=2)
    ax1.set_ylabel("Y", labelpad=2)
    ax1.set_zlabel("Z", labelpad=2)
    ax1.grid(True, alpha=0.2)
    ax1.legend(loc="upper right", framealpha=0.7, fontsize=9)
    ax1.set_title(f"Input mesh + PCA axis", fontsize=12)
    fig.text(
        ax1.get_position().x0 - 0.01,
        ax1.get_position().y1 + 0.01,
        PANEL_LABELS[0],
        fontsize=13,
        fontweight="bold",
        va="bottom",
        ha="left",
    )

    # ── Panel 2: Mesh + cross-section planes ────────────────────────────────
    print("Panel 2: Cross-section planes")
    ax2 = fig.add_subplot(gs[0, 1], projection="3d")

    # Plot mesh as wireframe (lighter)
    for face in F:
        for i in range(3):
            pts = V[face[[i, (i + 1) % 3]]]
            ax2.plot(
                pts[:, 0],
                pts[:, 1],
                pts[:, 2],
                color="#78909C",
                alpha=0.3,
                linewidth=0.4,
            )

    # Draw cross-section planes as translucent disks
    plane_colors = plt.cm.plasma(np.linspace(0.15, 0.85, len(plane_ts)))
    for tk, color in zip(plane_ts, plane_colors):
        plane_origin = origin + tk * axis
        # Generate a disk in the plane
        theta = np.linspace(0, 2 * np.pi, 32)
        r = extent * 0.6
        disk_x = r * np.cos(theta)
        disk_y = r * np.sin(theta)
        disk_3d = (
            plane_origin[:, np.newaxis]
            + e1[:, np.newaxis] * disk_x[np.newaxis, :]
            + e2[:, np.newaxis] * disk_y[np.newaxis, :]
        )
        # Draw as a filled polygon (list of (N,3) arrays)
        verts = np.stack([disk_3d[0], disk_3d[1], disk_3d[2]], axis=-1)
        plane_poly = Poly3DCollection(
            [verts], alpha=0.15, facecolor=color, edgecolor=color, linewidth=0.5
        )
        ax2.add_collection3d(plane_poly)

    # Also draw the contour on each plane
    for tk, contours_3d, color in zip(plane_ts, section_contours_3d, plane_colors):
        for c in contours_3d:
            if len(c) >= 3:
                ax2.plot(
                    c[:, 0],
                    c[:, 1],
                    c[:, 2],
                    color=color,
                    linewidth=1.2,
                )

    ax2.set_xlim(center[0] - extent, center[0] + extent)
    ax2.set_ylim(center[1] - extent, center[1] + extent)
    ax2.set_zlim(center[2] - extent, center[2] + extent)
    ax2.set_box_aspect([1, 1, 1])
    ax2.view_init(elev=25, azim=-55)

    ax2.set_xlabel("X", labelpad=2)
    ax2.set_ylabel("Y", labelpad=2)
    ax2.set_zlabel("Z", labelpad=2)
    ax2.grid(True, alpha=0.2)
    ax2.set_title("Cross-section planes", fontsize=12)
    fig.text(
        ax2.get_position().x0 - 0.01,
        ax2.get_position().y1 + 0.01,
        PANEL_LABELS[1],
        fontsize=13,
        fontweight="bold",
        va="bottom",
        ha="left",
    )

    # ── Panel 3: 2D cross-section + fitted circle ───────────────────────────
    print("Panel 3: Cross-section with fitted circle")
    ax3 = fig.add_subplot(gs[1, 0])

    # Draw contour(s)
    for c2d in target_contours_2d:
        if len(c2d) >= 3:
            ax3.fill(
                c2d[:, 0],
                c2d[:, 1],
                alpha=0.3,
                color="#B0BEC5",
                edgecolor="#546E7A",
                linewidth=1.0,
            )

    # Draw sampled points
    ax3.scatter(
        all_pts_2d[:, 0],
        all_pts_2d[:, 1],
        s=1.5,
        color="#37474F",
        alpha=0.5,
        label="Contour samples",
    )

    # Draw fitted MEC
    circle = Circle(
        mec_center,
        mec_radius,
        fill=False,
        edgecolor="#D32F2F",
        linewidth=2.0,
        linestyle="--",
        label=f"MEC (r={mec_radius:.3f})",
    )
    ax3.add_patch(circle)
    ax3.scatter(
        mec_center[0],
        mec_center[1],
        marker="x",
        color="#D32F2F",
        s=60,
        linewidths=2,
    )

    # Equal aspect
    ax3.set_aspect("equal")
    pad = mec_radius * 1.15 if mec_radius > 0 else 0.1
    ax3.set_xlim(mec_center[0] - pad, mec_center[0] + pad)
    ax3.set_ylim(mec_center[1] - pad, mec_center[1] + pad)

    ax3.set_xlabel("Plane coordinate u")
    ax3.set_ylabel("Plane coordinate v")
    ax3.grid(True, alpha=0.3)
    ax3.legend(loc="upper right", framealpha=0.8)
    ax3.set_title(
        f"Cross-section {mid_idx + 1} with fitted covering circle",
        fontsize=12,
    )
    ax3.text(
        -0.12,
        1.02,
        PANEL_LABELS[2],
        transform=ax3.transAxes,
        fontsize=13,
        fontweight="bold",
        va="bottom",
        ha="left",
    )

    # ── Panel 4: Mesh + capsule chain ───────────────────────────────────────
    print("Panel 4: Capsule chain overlay")
    ax4 = fig.add_subplot(gs[1, 1], projection="3d")

    # Plot mesh wireframe
    for face in F:
        for i in range(3):
            pts = V[face[[i, (i + 1) % 3]]]
            ax4.plot(
                pts[:, 0],
                pts[:, 1],
                pts[:, 2],
                color="#78909C",
                alpha=0.25,
                linewidth=0.4,
            )

    # Capsule parameters are in the link frame; the mesh we loaded is also
    # in the link frame (collision meshes are already in link frame, while
    # visual meshes need the URDF origin transform). The collision mesh
    # link3.obj is directly in the link frame -- no transform needed.
    capsules_drawn = 0
    for cp in link_caps:
        p0 = np.array(cp["p0"], dtype=float)
        p1 = np.array(cp["p1"], dtype=float)
        radius = float(cp["radius"])
        cap_mesh = build_capsule_mesh(p0, p1, radius, segments=20)

        # Draw capsule as translucent surface
        cap_verts = cap_mesh.vertices
        cap_faces = cap_mesh.faces
        cap_poly = Poly3DCollection(
            cap_verts[cap_faces],
            alpha=0.35,
            facecolor="#E53935",
            edgecolor="#C62828",
            linewidth=0.2,
        )
        ax4.add_collection3d(cap_poly)
        capsules_drawn += 1

    print(f"  drew {capsules_drawn} capsules")

    # Add labels for capsules
    for cp in link_caps:
        p0 = np.array(cp["p0"], dtype=float)
        p1 = np.array(cp["p1"], dtype=float)
        radius = float(cp["radius"])
        mid = (p0 + p1) / 2.0
        ax4.text(
            mid[0],
            mid[1],
            mid[2],
            f"  r={radius:.2f}",
            color="#C62828",
            fontsize=8,
            fontweight="bold",
        )

    ax4.set_xlim(center[0] - extent, center[0] + extent)
    ax4.set_ylim(center[1] - extent, center[1] + extent)
    ax4.set_zlim(center[2] - extent, center[2] + extent)
    ax4.set_box_aspect([1, 1, 1])
    ax4.view_init(elev=25, azim=-55)

    ax4.set_xlabel("X", labelpad=2)
    ax4.set_ylabel("Y", labelpad=2)
    ax4.set_zlabel("Z", labelpad=2)
    ax4.grid(True, alpha=0.2)
    ax4.set_title("Capsule chain overlay", fontsize=12)
    fig.text(
        ax4.get_position().x0 - 0.01,
        ax4.get_position().y1 + 0.01,
        PANEL_LABELS[3],
        fontsize=13,
        fontweight="bold",
        va="bottom",
        ha="left",
    )

    # ── Save ────────────────────────────────────────────────────────────────
    pdf_path = OUT_DIR / "capsule_method_schematic.pdf"
    png_path = OUT_DIR / "capsule_method_schematic.png"
    print(f"\nSaving {pdf_path}")
    fig.savefig(str(pdf_path), dpi=300, bbox_inches="tight", pad_inches=0.05)
    print(f"Saving {png_path}")
    fig.savefig(str(png_path), dpi=300, bbox_inches="tight", pad_inches=0.05)
    plt.close(fig)
    print("Done.")


if __name__ == "__main__":
    main()
