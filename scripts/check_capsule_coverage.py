#!/usr/bin/env python3
"""Diagnostic: does each stored capsule actually cover its mesh, and is the
PCA axis sensible? Reads fr3.urdf (collision origins + mesh files) +
fr3_capsuleized.json, reverses the collision origin to test coverage in the
mesh-local frame, and prints the PCA axis vs the mesh's longest bbox axis.
"""
import json
import math
import os
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


def parse_collisions(urdf_path):
    tree = ET.parse(urdf_path)
    out = {}
    for link in tree.iter("link"):
        name = link.get("name")
        col = link.find("collision")
        if col is None:
            continue
        origin = col.find("origin")
        xyz = [0.0, 0.0, 0.0]
        rpy = [0.0, 0.0, 0.0]
        if origin is not None:
            if origin.get("xyz"):
                xyz = [float(v) for v in origin.get("xyz").split()]
            if origin.get("rpy"):
                rpy = [float(v) for v in origin.get("rpy").split()]
        mesh = col.find("geometry/mesh")
        fn = mesh.get("filename") if mesh is not None else None
        out[name] = (np.array(xyz), rpy_to_R(*rpy), fn)
    return out


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


def tightness_metrics(V, capsules, assigned):
    cap_volume = sum(capsule_volume(p0, p1, radius) for p0, p1, radius in capsules)
    ext = V.max(axis=0) - V.min(axis=0)
    aabb_volume = float(np.prod(np.maximum(ext, 1e-12)))
    inflation = cap_volume / aabb_volume if aabb_volume > 1e-12 else 0.0

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
    return inflation, worst_radius_ratio


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


def evaluate_capsules(caps_json, urdf_path):
    caps = json.load(open(caps_json))
    cols = parse_collisions(urdf_path)
    rows = []
    all_ok = True
    for link, body in caps.items():
        if link not in cols or "capsules" not in body:
            continue
        T, R, fn = cols[link]
        if fn is None:
            continue
        stl = fn.replace(MESH_PREFIX, os.path.join(REPO, "resources/fr3"))
        V = trimesh.load(stl).vertices
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
        inflation, radius_ratio = tightness_metrics(V, capsules, assigned)

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
    ap.add_argument("--json", action="store_true", help="emit machine-readable metrics")
    args = ap.parse_args()

    result = evaluate_capsules(args.caps_json, args.urdf)
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
        return

    print(f"{'link':16} {'caps':>4} {'covered':8} {'worst':>9} {'radius':>8} "
          f"{'maxd':>8} {'capV/aabb':>9} {'r/binMed':>8} | "
          f"{'PCA axis (link)':26} {'bbox long axis':26}")
    for row in result["links"]:
        axis = row["axis"]
        bbox = row["bbox_long_axis"]
        print(f"{row['link']:16} {row['capsules']:4} {str(row['covered']):8} "
              f"{row['worst']:9.6f} {row['radius']:8.4f} {row['maxd']:8.4f} "
              f"{row['capV_aabb']:9.4f} {row['r_binMed']:8.2f} | "
              f"axis=[{axis[0]:+.2f} {axis[1]:+.2f} {axis[2]:+.2f}] "
              f"len={row['axis_length']:.3f} bbox_long=[{bbox[0]} {bbox[1]} {bbox[2]}] "
              f"align={row['axis_bbox_align']:.2f}")
    print(f"\nALL COVERED: {result['all_covered']}")


if __name__ == "__main__":
    main()
