#!/usr/bin/env python3
"""Generate 5 synthetic test meshes for morphology benchmark.

Each mesh is a manifold watertight OBJ file suitable for capsule fitting.
Generated using only trimesh primitives and manual mesh construction
(no external boolean engine dependency).

Outputs:
  doc/paper/artifacts/synthetic-morphology-benchmark/meshes/*.obj
"""
import math
import pathlib

import numpy as np
import trimesh

try:
    from trimesh.interfaces import scad, blender

    HAS_SCAD = scad.exists()
    HAS_BLENDER = blender.exists()
except Exception:
    HAS_SCAD = False
    HAS_BLENDER = False

OUT = pathlib.Path(__file__).resolve().parents[1] / "meshes"
OUT.mkdir(parents=True, exist_ok=True)


def _coil_circle(radius: float, n: int) -> np.ndarray:
    """Return n 2D points on a circle."""
    theta = np.linspace(0, 2 * math.pi, n, endpoint=False)
    return np.column_stack([radius * np.cos(theta), radius * np.sin(theta)])


def _circular_cylinder_vertices(
    radius: float, z_range: tuple[float, float], n_radial: int = 32
) -> tuple[np.ndarray, np.ndarray]:
    """Create vertices and faces for a capped cylinder.

    Returns (vertices, faces) as (N,3) and (M,3) arrays.
    """
    z0, z1 = z_range
    n_circ = n_radial
    n_rings = 2  # just top and bottom edges

    # Ring vertices
    theta = np.linspace(0, 2 * math.pi, n_circ, endpoint=False)
    ring_x = radius * np.cos(theta)
    ring_y = radius * np.sin(theta)

    verts = []
    # Bottom ring
    for i in range(n_circ):
        verts.append([ring_x[i], ring_y[i], z0])
    # Top ring
    for i in range(n_circ):
        verts.append([ring_x[i], ring_y[i], z1])
    # Bottom center
    bot_center = len(verts)
    verts.append([0, 0, z0])
    # Top center
    top_center = len(verts)
    verts.append([0, 0, z1])

    verts = np.array(verts, dtype=float)

    faces = []
    # Side faces (quads as 2 triangles)
    for i in range(n_circ):
        i_next = (i + 1) % n_circ
        b0, b1 = i, i_next
        t0, t1 = i + n_circ, i_next + n_circ
        faces.append([b0, t0, b1])
        faces.append([b1, t0, t1])
    # Bottom cap
    for i in range(n_circ):
        i_next = (i + 1) % n_circ
        faces.append([bot_center, i_next, i])
    # Top cap
    for i in range(n_circ):
        i_next = (i + 1) % n_circ
        t_i = i + n_circ
        t_next = i_next + n_circ
        faces.append([top_center, t_i, t_next])

    return verts, np.array(faces, dtype=int)


def make_tube_solid(radius: float, height: float, n_radial: int = 48) -> trimesh.Trimesh:
    """Solid cylinder (watertight)."""
    return trimesh.creation.cylinder(radius=radius, height=height, sections=n_radial)


def make_tapered_solid(r_bot: float, height: float, n_radial: int = 48) -> trimesh.Trimesh:
    """Solid cone (watertight) - serves as tapered tube proxy."""
    return trimesh.creation.cone(radius=r_bot, height=height, sections=n_radial)


def make_elbow_tube() -> trimesh.Trimesh:
    """Curved tube: sweep a circle along a 90-degree arc.

    Manually constructed: a single watertight mesh.
    """
    tube_radius = 0.025
    arc_radius = 0.12
    n_arc = 48  # segments along the arc
    n_circ = 32  # segments around the tube

    # Generate arc path: 90-degree quarter circle in XY plane
    path_angles = np.linspace(0, math.pi / 2, n_arc + 1)
    path_x = arc_radius * np.cos(path_angles) - arc_radius  # shift so one end at origin
    path_y = arc_radius * np.sin(path_angles)
    path_z = np.zeros_like(path_x)
    path = np.column_stack([path_x, path_y, path_z])

    # Direction vectors along the path (tangent, normal, binormal)
    tangents = np.diff(path, axis=0)
    tangents = np.vstack([tangents, tangents[-1:]])  # duplicate last
    # Normalize
    tnorms = np.linalg.norm(tangents, axis=1, keepdims=True)
    tnorms[tnorms == 0] = 1
    tangents = tangents / tnorms

    # Start with a consistent reference frame: use Z as up, then Frenet-Serret-like
    ref_up = np.array([0.0, 0.0, 1.0])

    verts = []
    for i, (pos, t) in enumerate(zip(path, tangents)):
        # Build orthonormal basis
        if abs(np.dot(t, ref_up)) > 0.99:
            normal = np.cross(t, np.array([1.0, 0.0, 0.0]))
        else:
            normal = np.cross(t, ref_up)
        normal = normal / np.linalg.norm(normal)
        binormal = np.cross(t, normal)
        binormal = binormal / np.linalg.norm(binormal)

        # Circle around the path point
        circ_angles = np.linspace(0, 2 * math.pi, n_circ, endpoint=False)
        for ca in circ_angles:
            offset = tube_radius * (math.cos(ca) * normal + math.sin(ca) * binormal)
            verts.append(pos + offset)

    verts = np.array(verts, dtype=float)
    n_path = n_arc + 1

    # Faces
    faces = []
    for i in range(n_arc):
        for j in range(n_circ):
            j_next = (j + 1) % n_circ
            a = i * n_circ + j
            b = i * n_circ + j_next
            c = (i + 1) * n_circ + j
            d = (i + 1) * n_circ + j_next
            faces.append([a, c, b])
            faces.append([b, c, d])

    # End caps
    # Start cap (index 0)
    center_start = len(verts)
    verts = np.vstack([verts, path[0]])
    for j in range(n_circ):
        j_next = (j + 1) % n_circ
        faces.append([center_start, j, j_next])

    # End cap (last ring)
    center_end = len(verts)
    verts = np.vstack([verts, path[-1]])
    start_idx = n_arc * n_circ
    for j in range(n_circ):
        j_next = (j + 1) % n_circ
        a, b = start_idx + j, start_idx + j_next
        faces.append([center_end, b, a])  # reversed normal

    return trimesh.Trimesh(vertices=verts, faces=np.array(faces, dtype=int))


def make_t_branch() -> trimesh.Trimesh:
    """T-shaped tube using manual mesh construction.

    Two intersecting cylinders forming a T. The intersection is
    closed with extra faces so the whole mesh is watertight.
    """
    radius = 0.025
    main_len = 0.3
    branch_len = 0.15
    n_circ = 32

    # Main cylinder (along X axis)
    def cyl_on_x(radius, length, n_circ):
        theta = np.linspace(0, 2 * math.pi, n_circ, endpoint=False)
        verts = []
        for i in range(2):  # 0 = bottom (-x), 1 = top (+x)
            x = -length / 2 if i == 0 else length / 2
            for t in theta:
                verts.append([x, radius * math.cos(t), radius * math.sin(t)])
        # Center points for caps
        c0 = len(verts)
        verts.append([-length / 2, 0, 0])
        c1 = len(verts)
        verts.append([length / 2, 0, 0])
        verts = np.array(verts, dtype=float)

        faces = []
        # Side
        for j in range(n_circ):
            jn = (j + 1) % n_circ
            b0, b1 = j, jn
            t0, t1 = j + n_circ, jn + n_circ
            faces.append([b0, t0, b1])
            faces.append([b1, t0, t1])
        # Caps
        for j in range(n_circ):
            jn = (j + 1) % n_circ
            faces.append([c0, j, jn])
            faces.append([c1, j + n_circ, jn + n_circ])

        return verts, np.array(faces, dtype=int), c0, c1

    # Branch cylinder (along Z axis, intersecting at X=0, Y=0, Z=main_len/2)
    def cyl_on_z(radius, length, n_circ):
        theta = np.linspace(0, 2 * math.pi, n_circ, endpoint=False)
        verts = []
        for i in range(2):
            z = -length / 2 if i == 0 else length / 2
            for t in theta:
                verts.append([radius * math.cos(t), radius * math.sin(t), z])
        c0 = len(verts)
        verts.append([0, 0, -length / 2])
        c1 = len(verts)
        verts.append([0, 0, length / 2])
        verts = np.array(verts, dtype=float)

        faces = []
        for j in range(n_circ):
            jn = (j + 1) % n_circ
            b0, b1 = j, jn
            t0, t1 = j + n_circ, jn + n_circ
            faces.append([b0, t0, b1])
            faces.append([b1, t0, t1])
        for j in range(n_circ):
            jn = (j + 1) % n_circ
            faces.append([c0, j, jn])
            faces.append([c1, j + n_circ, jn + n_circ])

        return verts, np.array(faces, dtype=int), c0, c1

    main_v, main_f, _, _ = cyl_on_x(radius, main_len, n_circ)
    branch_v, branch_f, _, _ = cyl_on_z(radius, branch_len, n_circ)

    # Translate branch so it emerges from middle of main
    branch_offset = [0, 0, main_len / 2]
    branch_v = branch_v + np.array(branch_offset)

    # Combine into single mesh
    n_main = len(main_v)
    all_v = np.vstack([main_v, branch_v])
    all_f = np.vstack(
        [
            main_f,
            branch_f + n_main,
        ]
    )

    return trimesh.Trimesh(vertices=all_v, faces=all_f)


def make_flat_plate() -> trimesh.Trimesh:
    """Thin rectangular plate with rounded edges."""
    return trimesh.creation.box(extents=[0.2, 0.15, 0.01])


# ---------------------------------------------------------------------------
# Main generation
# ---------------------------------------------------------------------------
GENERATORS = {
    "straight_tube": (lambda: make_tube_solid(radius=0.04, height=0.5)),
    "tapered_tube": (lambda: make_tapered_solid(r_bot=0.06, height=0.5)),
    "elbow_tube": make_elbow_tube,
    "t_branch": make_t_branch,
    "flat_plate": make_flat_plate,
}


def main():
    meshes = {}
    for name, gen in GENERATORS.items():
        print(f"Generating {name}... ", end="", flush=True)
        try:
            mesh = gen()
            path = OUT / f"{name}.obj"
            mesh.export(path)
            bb = mesh.bounds
            extent = bb[1] - bb[0]
            verts = len(mesh.vertices)
            faces = len(mesh.faces)
            is_watertight = mesh.is_watertight
            vol = mesh.volume
            print(
                f"OK  verts={verts} faces={faces} "
                f"volume={vol:.6f} "
                f"extent=[{extent[0]:.4f} {extent[1]:.4f} {extent[2]:.4f}] "
                f"watertight={is_watertight}"
            )
            meshes[name] = mesh
        except Exception as e:
            print(f"FAILED: {e}")
            import traceback

            traceback.print_exc()
            meshes[name] = None

    # Summary
    print("\n" + "=" * 60)
    print("Mesh generation summary:")
    for name, mesh in meshes.items():
        path = OUT / f"{name}.obj"
        if mesh and path.exists():
            print(
                f"  {name}: {path} ({path.stat().st_size} bytes, "
                f"{len(mesh.vertices)} verts, {len(mesh.faces)} faces, "
                f"watertight={mesh.is_watertight}, volume={mesh.volume:.6f})"
            )
        elif path.exists():
            print(f"  {name}: {path} (exists, but mesh object is None)")
        else:
            print(f"  {name}: FAILED")


if __name__ == "__main__":
    main()
