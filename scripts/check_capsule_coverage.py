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


def main():
    caps = json.load(open(CAPS_JSON))
    cols = parse_collisions(FR3_URDF)
    print(f"{'link':16} {'covered':8} {'radius':>8} {'maxd':>8} {'slack%':>7} | "
          f"{'PCA axis (link)':26} {'bbox long axis':26}")
    all_ok = True
    for link, body in caps.items():
        if link not in cols:
            print(f"{link:16} (no collision parsed)")
            continue
        T, R, fn = cols[link]
        if fn is None:
            continue
        stl = fn.replace(MESH_PREFIX, os.path.join(REPO, "resources/fr3"))
        V = trimesh.load(stl).vertices
        cp = body["capsules"][0]
        p0L = np.array(cp["p0"]); p1L = np.array(cp["p1"]); r = float(cp["radius"])
        # link -> mesh-local: subtract origin, undo rotation
        Rt = R.T
        p0m = Rt @ (p0L - T); p1m = Rt @ (p1L - T)
        maxd = max(point_to_seg(V[i], p0m, p1m) for i in range(len(V)))
        slack = (r - maxd) / maxd * 100.0 if maxd > 1e-9 else 0.0
        covered = maxd <= r + 1e-6
        all_ok &= covered
        # ACTUAL capsule axis chosen by the fitter (from JSON, link frame) vs
        # the mesh's longest bbox axis.
        cap_axis = (p1L - p0L)
        cap_axis = cap_axis / np.linalg.norm(cap_axis) if np.linalg.norm(cap_axis) > 1e-12 else np.zeros(3)
        seg_len = float(np.linalg.norm(p1L - p0L))
        ext = V.max(axis=0) - V.min(axis=0)
        long_axis = np.zeros(3); long_axis[int(np.argmax(ext))] = 1.0
        align = abs(float(cap_axis @ long_axis))
        print(f"{link:16} {str(covered):8} {r:8.4f} {maxd:8.4f} {slack:7.2f} | "
              f"axis=[{cap_axis[0]:+.2f} {cap_axis[1]:+.2f} {cap_axis[2]:+.2f}] "
              f"len={seg_len:.3f} bbox_long=[{int(long_axis[0])} {int(long_axis[1])} {int(long_axis[2])}] align={align:.2f}")
    print(f"\nALL COVERED: {all_ok}")


if __name__ == "__main__":
    main()
