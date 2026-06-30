#!/usr/bin/env python3
"""Visualize covering capsules (from fr3_capsuleized.json) overlaid on the FR3 robot.

Host-side inspection tool. Loads fr3.urdf in pybullet (mesh paths rewritten for
the host), then for each link draws the fitted capsule (pybullet GEOM_CAPSULE)
in the link frame at the rest configuration, so you can eyeball that capsules
hug the links. Red capsules + the original mesh bodies.

    python3 scripts/viz_capsules.py            # GUI (needs DISPLAY)
    python3 scripts/viz_capsules.py --png OUT  # headless render to PNG
"""
import argparse
import json
import math
import os
import sys

import numpy as np
import trimesh

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FR3_URDF = os.path.join(REPO, "resources/fr3/urdf/fr3.urdf")
CAPS_JSON = os.path.join(REPO, "resources/fr3/urdf/fr3_capsuleized.json")


def build_capsule_mesh(p0, p1, radius, segments=28):
    """Capsule (cylinder + 2 hemispheres) spanning p0->p1 in world coords."""
    p0 = np.asarray(p0, dtype=float)
    p1 = np.asarray(p1, dtype=float)
    axis = p1 - p0
    L = float(np.linalg.norm(axis))
    if L < 1e-9:
        m = trimesh.creation.uv_sphere(radius=radius)
        m.apply_translation(p0)
        return m
    cyl = trimesh.creation.cylinder(radius=radius, height=L, sections=segments)
    cap_p = trimesh.creation.uv_sphere(radius=radius); cap_p.apply_translation([0, 0, L / 2.0])
    cap_n = trimesh.creation.uv_sphere(radius=radius); cap_n.apply_translation([0, 0, -L / 2.0])
    mesh = trimesh.util.concatenate([cyl, cap_p, cap_n])
    # mesh axis is +Z; rotate Z onto the segment direction, then translate to midpoint.
    mat = trimesh.geometry.align_vectors(np.array([0.0, 0, 1.0]), axis / L)
    mesh.apply_transform(mat)
    mesh.apply_translation((p0 + p1) / 2.0)
    return mesh


def quat_align_z(direction):
    """Quaternion [x,y,z,w] rotating the z-axis onto `direction` (need not be unit)."""
    d = np.asarray(direction, dtype=float)
    n = np.linalg.norm(d)
    if n < 1e-12:
        return [0.0, 0.0, 0.0, 1.0]
    d /= n
    z = np.array([0.0, 0.0, 1.0])
    c = float(np.dot(z, d))
    if c < -1.0 + 1e-9:
        return [0.0, 1.0, 0.0, 0.0]  # 180 deg about y
    axis = np.cross(z, d)
    w = 1.0 + c
    an = float(np.linalg.norm(axis))
    axis = axis / an if an > 1e-12 else np.array([0.0, 0.0, 0.0])
    return [float(axis[0]), float(axis[1]), float(axis[2]), float(w)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--png", default="", help="if set, render to this PNG (headless) instead of GUI")
    args = ap.parse_args()

    import pybullet as p
    import pybullet_data

    caps = json.load(open(CAPS_JSON))

    # Rewrite absolute /workspace mesh paths to the host repo path.
    urdf_txt = open(FR3_URDF).read()
    urdf_txt = urdf_txt.replace("/workspace/resources/fr3",
                                os.path.join(REPO, "resources/fr3"))
    tmp_urdf = "/tmp/fr3_host.urdf"
    open(tmp_urdf, "w").write(urdf_txt)

    if args.png:
        p.connect(p.DIRECT)
    else:
        try:
            p.connect(p.GUI)
        except Exception as e:
            print(f"GUI failed ({e}); falling back to DIRECT -> /tmp/fr3_capsules.png")
            p.connect(p.DIRECT)
            args.png = "/tmp/fr3_capsules.png"

    p.setAdditionalSearchPath(pybullet_data.getDataPath())
    p.configureDebugVisualizer(p.COV_ENABLE_GUI, 0)
    robot = p.loadURDF(tmp_urdf, useFixedBase=True, flags=p.URDF_USE_MATERIAL_COLORS_FROM_MTL)

    # link name -> joint index (base link has index -1)
    n = p.getNumJoints(robot)
    name2idx = {p.getBodyInfo(robot)[0].decode(): -1}
    for j in range(n):
        name2idx[p.getJointInfo(robot, j)[12].decode()] = j
    p.stepSimulation()

    # Draw each link's collision mesh (gray) so the link bodies are visible,
    # then the red capsules on top. getCollisionShapeData tuple:
    #   [2]shapeType [3]scale [4]meshFile(bytes) [5]localPos [6]localOrn
    for idx in range(-1, n):
        try:
            cs = p.getCollisionShapeData(robot, idx)
        except Exception:
            cs = []
        if idx < 0:
            lw_pos, lw_orn = p.getBasePositionAndOrientation(robot)
        else:
            ls = p.getLinkState(robot, idx, computeForwardKinematics=True)
            lw_pos, lw_orn = ls[4], ls[5]
        for c in cs:
            if len(c) < 7 or c[2] != p.GEOM_MESH:
                continue
            mesh_file = c[4]
            if isinstance(mesh_file, bytes):
                mesh_file = mesh_file.decode()
            scale = list(c[3])
            wpos, worn = p.multiplyTransforms(lw_pos, lw_orn, list(c[5]), list(c[6]))
            mvs = p.createVisualShape(p.GEOM_MESH, fileName=mesh_file,
                                      meshScale=scale, rgbaColor=[0.7, 0.7, 0.78, 1.0])
            p.createMultiBody(baseMass=0, baseVisualShapeIndex=mvs,
                              basePosition=list(wpos), baseOrientation=list(worn))

    drawn = 0
    for link, body in caps.items():
        if link not in name2idx:
            print(f"  skip {link}: not a loaded link")
            continue
        idx = name2idx[link]
        if idx < 0:
            lw_pos, lw_orn = p.getBasePositionAndOrientation(robot)
        else:
            ls = p.getLinkState(robot, idx, computeForwardKinematics=True)
            lw_pos, lw_orn = ls[4], ls[5]  # URDF link frame in world
        R = np.array(p.getMatrixFromQuaternion(lw_orn)).reshape(3, 3)
        origin = np.array(lw_pos)
        for k, cp in enumerate(body["capsules"]):
            p0 = np.array(cp["p0"], dtype=float)
            p1 = np.array(cp["p1"], dtype=float)
            r = float(cp["radius"])
            P0 = origin + R @ p0
            P1 = origin + R @ p1
            mesh = build_capsule_mesh(P0, P1, r)
            obj = f"/tmp/cap_{link}_{k}.obj"
            mesh.export(obj)
            vs = p.createVisualShape(p.GEOM_MESH, fileName=obj, meshScale=[1, 1, 1],
                                     rgbaColor=[1.0, 0.15, 0.15, 0.5])
            p.createMultiBody(baseMass=0, baseVisualShapeIndex=vs,
                              basePosition=[0, 0, 0], baseOrientation=[0, 0, 0, 1])
            mid = ((P0 + P1) / 2.0).tolist()
            p.addUserDebugText(f"{link} r={r:.3f} L={float(np.linalg.norm(P1-P0)):.3f}",
                               mid, textColorRGB=[1, 1, 0], textSize=0.9)
            drawn += 1

    print(f"drew {drawn} capsules over {robot=} ({n} joints)")

    if args.png:
        from PIL import Image  # noqa
        view = (1.6, 1.4, 1.2, 0, 0, 0.6)
        p.resetDebugVisualizerCamera(*view) if False else p.resetDebugVisualizerCamera(1.8, 35, -25, [0, 0, 0.5])
        _, _, px, _, _ = p.getCameraImage(1280, 800, renderer=p.ER_TINY_RENDERER)
        import numpy as _np
        arr = _np.array(px, dtype=_np.uint8).reshape(800, 1280, 4)
        from PIL import Image as _I
        _I.fromarray(arr[:, :, :3]).save(args.png)
        print(f"rendered -> {args.png}")
        p.disconnect()
    else:
        print("GUI open. Close the window or Ctrl-C to exit.")
        try:
            while True:
                p.stepSimulation()
        except KeyboardInterrupt:
            pass
        p.disconnect()


if __name__ == "__main__":
    main()
