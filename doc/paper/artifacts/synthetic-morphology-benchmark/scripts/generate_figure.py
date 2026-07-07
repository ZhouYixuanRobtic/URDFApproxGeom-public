#!/usr/bin/env python3
"""Generate qualitative figure comparing capsule overlays for best and worst cases.

Figure shows:
- Row 1: straight_tube (best case: elongated, tubular)
- Row 2: flat_plate (worst case: planar, non-elongated)

Each row shows the original mesh with capsule overlay from both presets.
"""
import json
import pathlib
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ARTIFACT_DIR = pathlib.Path(__file__).resolve().parents[1]
MESH_DIR = ARTIFACT_DIR / "meshes"
DATA_DIR = ARTIFACT_DIR / "data"
FIG_DIR = ARTIFACT_DIR / "figures"
FIG_DIR.mkdir(parents=True, exist_ok=True)


def load_capsules(shape: str, preset: str):
    json_path = DATA_DIR / f"{shape}_{preset}.json"
    if not json_path.exists():
        return None
    data = json.loads(json_path.read_text())
    body = data.get(shape)
    if body is None:
        return None
    return [(np.array(c["p0"]), np.array(c["p1"]), c["radius"]) for c in body.get("capsules", [])]


def plot_mesh_with_capsules(ax, V, F, capsules, title):
    """Plot mesh wireframe + capsule overlay on a 2D projection."""
    # Project to XY (best PCA plane visualization)
    ax.plot_trisurf(
        V[:, 0],
        V[:, 1],
        V[:, 2],
        triangles=F,
        alpha=0.3,
        color="lightblue",
        edgecolor="none",
        shade=False,
    )
    # Draw capsules
    for p0, p1, radius in capsules:
        # Draw capsule axis
        ax.plot([p0[0], p1[0]], [p0[1], p1[1]], [p0[2], p1[2]], "r-", linewidth=2, alpha=0.8)
        # Draw sphere caps
        u = np.linspace(0, 2 * np.pi, 20)
        v = np.linspace(0, np.pi, 20)
        for center in [p0, p1]:
            xs = center[0] + radius * np.outer(np.cos(u), np.sin(v))
            ys = center[1] + radius * np.outer(np.sin(u), np.sin(v))
            zs = center[2] + radius * np.outer(np.ones_like(u), np.cos(v))
            ax.plot_wireframe(xs, ys, zs, color="red", alpha=0.3, linewidth=0.5)
        # Draw mid cylinder silhouette
        theta = np.linspace(0, 2 * np.pi, 20)
        axis_dir = p1 - p0
        axis_len = np.linalg.norm(axis_dir)
        if axis_len > 1e-12:
            unit = axis_dir / axis_len
            # Find perpendicular vectors
            if abs(unit[2]) < 0.9:
                perp1 = np.cross(unit, [0, 0, 1])
            else:
                perp1 = np.cross(unit, [1, 0, 0])
            perp1 = perp1 / np.linalg.norm(perp1)
            perp2 = np.cross(unit, perp1)
            for t in np.linspace(0, 1, 10):
                pt = p0 + t * axis_dir
                circ_x = pt[0] + radius * (np.cos(theta) * perp1[0] + np.sin(theta) * perp2[0])
                circ_y = pt[1] + radius * (np.cos(theta) * perp1[1] + np.sin(theta) * perp2[1])
                circ_z = pt[2] + radius * (np.cos(theta) * perp1[2] + np.sin(theta) * perp2[2])
                ax.plot(circ_x, circ_y, circ_z, color="red", alpha=0.15, linewidth=0.5)

    ax.set_title(title)
    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    # Auto-scale
    all_pts = [V]
    for p0, p1, r in capsules:
        all_pts.append(np.array([p0, p1]) + [[r, r, r]])
    all_pts.append(np.array([p0, p1]) - [[r, r, r]])
    combined = np.vstack(all_pts)
    center = combined.mean(axis=0)
    extent = np.ptp(combined, axis=0)
    max_ext = max(extent)
    for d in range(3):
        ax.set_xlim3d(center[0] - max_ext, center[0] + max_ext)
        ax.set_ylim3d(center[1] - max_ext, center[1] + max_ext)
        ax.set_zlim3d(center[2] - max_ext, center[2] + max_ext)


def main():
    import trimesh

    # Best case: straight_tube, Worst case: flat_plate
    cases = [
        ("straight_tube", "Best: straight_tube"),
        ("elbow_tube", "Moderate: elbow_tube"),
        ("t_branch", "Poor: t_branch"),
        ("flat_plate", "Worst: flat_plate"),
    ]

    fig = plt.figure(figsize=(14, 10))
    fig.suptitle(
        "Capsule Fitting on Synthetic Morphologies (default preset)", fontsize=14, fontweight="bold"
    )

    for idx, (shape, label) in enumerate(cases):
        mesh_path = MESH_DIR / f"{shape}.obj"
        mesh = trimesh.load_mesh(str(mesh_path))
        V = mesh.vertices
        F = mesh.faces

        capsules = load_capsules(shape, "default")
        if capsules is None:
            print(f"WARNING: no capsules for {shape}, skipping")
            continue

        ax = fig.add_subplot(2, 2, idx + 1, projection="3d")
        plot_mesh_with_capsules(ax, V, F, capsules, label)

    plt.tight_layout()
    out_path = FIG_DIR / "capsule_overlay_comparison.png"
    plt.savefig(str(out_path), dpi=200, bbox_inches="tight")
    print(f"Figure saved: {out_path}")

    # Also generate PDF
    out_pdf = FIG_DIR / "capsule_overlay_comparison.pdf"
    plt.savefig(str(out_pdf), format="pdf", bbox_inches="tight")
    print(f"Figure saved: {out_pdf}")

    # Second figure: preset comparison for t_branch (interesting case)
    fig2, axes = plt.subplots(1, 2, figsize=(12, 5), subplot_kw={"projection": "3d"})
    fig2.suptitle("t_branch: default vs high_detail", fontsize=14, fontweight="bold")

    mesh_path = MESH_DIR / "t_branch.obj"
    mesh = trimesh.load_mesh(str(mesh_path))
    V = mesh.vertices
    F = mesh.faces

    caps_default = load_capsules("t_branch", "default")
    caps_detail = load_capsules("t_branch", "high_detail")

    plot_mesh_with_capsules(
        axes[0], V, F, caps_default, f"default ({len(caps_default)} caps, capV/aabb=4.14)"
    )
    plot_mesh_with_capsules(
        axes[1], V, F, caps_detail, f"high_detail ({len(caps_detail)} caps, capV/aabb=4.11)"
    )

    plt.tight_layout()
    out_path2 = FIG_DIR / "t_branch_preset_comparison.png"
    fig2.savefig(str(out_path2), dpi=200, bbox_inches="tight")
    print(f"Figure saved: {out_path2}")
    out_pdf2 = FIG_DIR / "t_branch_preset_comparison.pdf"
    fig2.savefig(str(out_pdf2), format="pdf", bbox_inches="tight")
    print(f"Figure saved: {out_pdf2}")


if __name__ == "__main__":
    main()
