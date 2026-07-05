#!/usr/bin/env python3
"""Diagnostic: does each stored capsule actually cover its mesh, and is the
PCA axis sensible? Reads fr3.urdf (collision origins + mesh files) +
fr3_capsuleized.json, reverses the collision origin to test coverage in the
mesh-local frame, and prints the PCA axis vs the mesh's longest bbox axis.
"""
import json
import math
import os
import pathlib
import xml.etree.ElementTree as ET

import numpy as np
import trimesh

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FR3_URDF = os.path.join(REPO, "resources/fr3/urdf/fr3.urdf")
CAPS_JSON = os.path.join(REPO, "resources/fr3/urdf/fr3_capsuleized.json")
MESH_PREFIX = "/workspace/resources/fr3"


def rpy_to_R(roll, pitch, yaw):
    cr, sr = math.cos(roll), math.sin(roll)
    cp, sp = math.cos(pitch), math.sin(pitch)
    cy, sy = math.cos(yaw), math.sin(yaw)
    Rx = np.array([[1, 0, 0], [0, cr, -sr], [0, sr, cr]])
    Ry = np.array([[cp, 0, sp], [0, 1, 0], [-sp, 0, cp]])
    Rz = np.array([[cy, -sy, 0], [sy, cy, 0], [0, 0, 1]])
    return Rz @ Ry @ Rx


def parse_mesh_sources(urdf_path, mesh_source="visual"):
    repo = pathlib.Path(__file__).resolve().parents[1]
    tree = ET.parse(urdf_path)
    out = {}

    def read_origin(elem):
        origin = elem.find("origin")
        xyz = [0.0, 0.0, 0.0]
        rpy = [0.0, 0.0, 0.0]
        if origin is not None:
            if origin.get("xyz"):
                xyz = [float(v) for v in origin.get("xyz").split()]
            if origin.get("rpy"):
                rpy = [float(v) for v in origin.get("rpy").split()]
        return np.array(xyz), rpy_to_R(*rpy)

    def read_mesh(link, tag):
        elem = link.find(tag)
        if elem is None:
            return None
        mesh = elem.find("geometry/mesh")
        if mesh is None:
            return None
        fn = mesh.get("filename")
        if not fn:
            return None
        if fn.startswith("/workspace/"):
            fn = str(repo / fn.removeprefix("/workspace/"))
        T, R = read_origin(elem)
        return T, R, fn

    for link in tree.iter("link"):
        name = link.get("name")
        primary = read_mesh(link, "visual" if mesh_source == "visual" else "collision")
        fallback = read_mesh(link, "collision" if mesh_source == "visual" else "visual")
        chosen = primary if primary is not None else fallback
        if chosen is not None:
            out[name] = chosen
    return out


def parse_collisions(urdf_path):
    return parse_mesh_sources(urdf_path, "collision")


def point_to_seg(p, a, b):
    d = b - a
    denom = float(d @ d)
    t = 0.0 if denom < 1e-12 else float(np.clip((p - a) @ d / denom, 0.0, 1.0))
    return np.linalg.norm(p - (a + t * d))


def point_to_seg_with_t(p, a, b):
    d = b - a
    denom = float(d @ d)
    t = 0.0 if denom < 1e-12 else float(np.clip((p - a) @ d / denom, 0.0, 1.0))
    q = a + t * d
    return float(np.linalg.norm(p - q)), t


def capsule_volume(p0, p1, radius):
    length = float(np.linalg.norm(p1 - p0))
    return math.pi * radius * radius * length + (4.0 / 3.0) * math.pi * radius ** 3


def capsule_bounds(capsules):
    if not capsules:
        return None
    lows = []
    highs = []
    for p0, p1, radius in capsules:
        r = np.array([radius, radius, radius], dtype=float)
        lows.append(np.minimum(p0, p1) - r)
        highs.append(np.maximum(p0, p1) + r)
    return np.min(np.vstack(lows), axis=0), np.max(np.vstack(highs), axis=0)


def estimate_capsule_union_volume(capsules, samples_per_axis=64, chunk_size=200000):
    if not capsules:
        return 0.0
    if len(capsules) == 1:
        p0, p1, radius = capsules[0]
        return capsule_volume(p0, p1, radius)
    bounds = capsule_bounds(capsules)
    if bounds is None:
        return 0.0
    lo, hi = bounds
    ext = hi - lo
    box_volume = float(np.prod(ext))
    if box_volume <= 0.0:
        return 0.0

    n = max(1, int(samples_per_axis))
    coords = [lo[d] + (np.arange(n, dtype=float) + 0.5) * ext[d] / n for d in range(3)]
    total = n ** 3
    inside = 0
    for start in range(0, total, chunk_size):
        stop = min(total, start + chunk_size)
        ids = np.arange(start, stop, dtype=np.int64)
        ix = ids // (n * n)
        iy = (ids // n) % n
        iz = ids % n
        points = np.column_stack((coords[0][ix], coords[1][iy], coords[2][iz]))
        covered = np.zeros(points.shape[0], dtype=bool)
        for p0, p1, radius in capsules:
            axis = p1 - p0
            denom = float(axis @ axis)
            if denom < 1e-12:
                nearest = np.repeat(p0.reshape(1, 3), points.shape[0], axis=0)
            else:
                t = np.clip(((points - p0) @ axis) / denom, 0.0, 1.0)
                nearest = p0 + t[:, None] * axis
            covered |= np.linalg.norm(points - nearest, axis=1) <= radius
        inside += int(np.count_nonzero(covered))
    return box_volume * (inside / float(total))


def tightness_metrics(V, capsules, assigned, volume_samples=64):
    primitive_sum = sum(capsule_volume(p0, p1, radius) for p0, p1, radius in capsules)
    ext = V.max(axis=0) - V.min(axis=0)
    aabb_volume = float(np.prod(np.maximum(ext, 1e-12)))
    inflation = primitive_sum / aabb_volume if aabb_volume > 1e-12 else 0.0
    union_volume = estimate_capsule_union_volume(capsules, samples_per_axis=volume_samples)

    worst_radius_ratio = 0.0
    for idx, (p0, p1, radius) in enumerate(capsules):
        mask = assigned == idx
        if not np.any(mask):
            continue
        bin_max = []
        for vertex in V[mask]:
            distance, t = point_to_seg_with_t(vertex, p0, p1)
            slot = min(9, max(0, int(t * 10.0)))
            while len(bin_max) <= slot:
                bin_max.append(0.0)
            bin_max[slot] = max(bin_max[slot], distance)
        nonzero = [v for v in bin_max if v > 1e-12]
        if nonzero:
            median_section_radius = float(np.median(nonzero))
            worst_radius_ratio = max(worst_radius_ratio, radius / median_section_radius)
    return inflation, worst_radius_ratio, union_volume, primitive_sum


def axis_overhang_metrics(V, capsules, assigned):
    worst_ratio = 0.0
    worst_abs = 0.0
    for idx, (p0, p1, radius) in enumerate(capsules):
        mask = assigned == idx
        if not np.any(mask):
            continue
        axis = p1 - p0
        length = float(np.linalg.norm(axis))
        if length < 1e-12 or radius <= 1e-12:
            continue
        unit = axis / length
        vertex_projection = V[mask] @ unit
        endpoint_projection = np.array([p0 @ unit, p1 @ unit], dtype=float)
        low_gap = float(endpoint_projection.min() - vertex_projection.min())
        high_gap = float(vertex_projection.max() - endpoint_projection.max())
        low_overhang = max(0.0, radius - low_gap)
        high_overhang = max(0.0, radius - high_gap)
        capsule_worst = max(low_overhang, high_overhang)
        worst_abs = max(worst_abs, capsule_worst)
        worst_ratio = max(worst_ratio, capsule_worst / radius)
    return worst_abs, worst_ratio


def capsule_to_mesh_frame(cp, T, R):
    p0L = np.array(cp["p0"], dtype=float)
    p1L = np.array(cp["p1"], dtype=float)
    radius = float(cp["radius"])
    Rt = R.T
    return Rt @ (p0L - T), Rt @ (p1L - T), radius


def best_capsule_distance(vertex, capsules):
    best_signed = float("inf")
    best_raw = float("inf")
    best_index = -1
    for idx, (p0m, p1m, radius) in enumerate(capsules):
        raw = point_to_seg(vertex, p0m, p1m)
        signed = raw - radius
        if signed < best_signed:
            best_signed = signed
            best_raw = raw
            best_index = idx
    return best_signed, best_raw, best_index


def evaluate_capsules(caps_json, urdf_path, mesh_source="visual", volume_samples=64):
    caps = json.load(open(caps_json))
    cols = parse_mesh_sources(urdf_path, mesh_source)
    rows = []
    all_ok = True
    for link, body in caps.items():
        if link not in cols or "capsules" not in body:
            continue
        T, R, fn = cols[link]
        if fn is None:
            continue
        stl = fn.replace(MESH_PREFIX, os.path.join(REPO, "resources/fr3"))
        loaded = trimesh.load(stl, force="mesh")
        if isinstance(loaded, trimesh.Scene):
            loaded = trimesh.util.concatenate(tuple(loaded.geometry.values()))
        V = loaded.vertices
        capsules = [capsule_to_mesh_frame(cp, T, R) for cp in body["capsules"]]
        signed = []
        raw = []
        assigned = []
        for vertex in V:
            sdist, rdist, idx = best_capsule_distance(vertex, capsules)
            signed.append(sdist)
            raw.append(rdist)
            assigned.append(idx)
        signed = np.array(signed, dtype=float)
        raw = np.array(raw, dtype=float)
        assigned = np.array(assigned, dtype=int)
        worst = float(signed.max())
        covered = worst <= 1e-6
        all_ok &= covered
        inflation, radius_ratio, union_volume, primitive_sum = tightness_metrics(
            V, capsules, assigned, volume_samples=volume_samples)
        axis_overhang, axis_overhang_ratio = axis_overhang_metrics(V, capsules, assigned)

        p0L = np.array(body["capsules"][0]["p0"], dtype=float)
        p1L = np.array(body["capsules"][0]["p1"], dtype=float)
        cap_axis = p1L - p0L
        seg_len = float(np.linalg.norm(cap_axis))
        cap_axis = cap_axis / seg_len if seg_len > 1e-12 else np.zeros(3)
        ext = V.max(axis=0) - V.min(axis=0)
        long_axis = np.zeros(3)
        long_axis[int(np.argmax(ext))] = 1.0
        align = abs(float(cap_axis @ long_axis))

        rows.append({
            "link": link,
            "capsules": len(body["capsules"]),
            "covered": bool(covered),
            "worst": worst,
            "radius": float(max(c[2] for c in capsules)),
            "maxd": float(raw.max()),
            "capV_aabb": float(inflation),
            "r_binMed": float(radius_ratio),
            "capsule_union_volume": float(union_volume),
            "capsule_primitive_volume_sum": float(primitive_sum),
            "volume_samples": volume_samples,
            "mesh_source": mesh_source,
            "axis_overhang": float(axis_overhang),
            "axis_overhang_r": float(axis_overhang_ratio),
            "axis": [float(cap_axis[0]), float(cap_axis[1]), float(cap_axis[2])],
            "axis_length": seg_len,
            "bbox_long_axis": [int(long_axis[0]), int(long_axis[1]), int(long_axis[2])],
            "axis_bbox_align": align,
        })
    return {"all_covered": bool(all_ok), "links": rows}


def main():
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("--caps-json", default=CAPS_JSON)
    ap.add_argument("--urdf", default=FR3_URDF)
    ap.add_argument("--mesh-source", default="visual", choices=["visual", "collision"])
    ap.add_argument("--volume-samples", type=int, default=64)
    ap.add_argument("--json", action="store_true", help="emit machine-readable metrics")
    args = ap.parse_args()

    result = evaluate_capsules(args.caps_json, args.urdf,
                               mesh_source=args.mesh_source,
                               volume_samples=args.volume_samples)
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
        return

    print(f"{'link':16} {'caps':>4} {'covered':8} {'worst':>9} {'radius':>8} "
          f"{'maxd':>8} {'capV/aabb':>9} {'r/binMed':>8} {'over/r':>8} | "
          f"{'PCA axis (link)':26} {'bbox long axis':26}")
    for row in result["links"]:
        axis = row["axis"]
        bbox = row["bbox_long_axis"]
        print(f"{row['link']:16} {row['capsules']:4} {str(row['covered']):8} "
              f"{row['worst']:9.6f} {row['radius']:8.4f} {row['maxd']:8.4f} "
              f"{row['capV_aabb']:9.4f} {row['r_binMed']:8.2f} {row['axis_overhang_r']:8.2f} | "
              f"axis=[{axis[0]:+.2f} {axis[1]:+.2f} {axis[2]:+.2f}] "
              f"len={row['axis_length']:.3f} bbox_long=[{bbox[0]} {bbox[1]} {bbox[2]}] "
              f"align={row['axis_bbox_align']:.2f}")
    print(f"\nALL COVERED: {result['all_covered']}")


if __name__ == "__main__":
    main()
