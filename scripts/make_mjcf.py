#!/usr/bin/env python3
"""Convert fr3_capsuleized.json (+ fr3.urdf) into an MJCF (MuJoCo XML) for
visualization in robot-viewer / MuJoCo viewer.

MJCF has a native capsule primitive, so each fitted capsule becomes
    <geom type="capsule" fromto="x0 y0 z0 x1 y1 z1" size="radius"/>
placed in world at the rest configuration. Link collision STLs are added as
gray mesh geoms for context. The robot is assembled via pybullet's forward
kinematics (joints at 0); the MJCF itself is a static scene (no joints).
"""
import json
import os
import xml.etree.ElementTree as ET

import numpy as np

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FR3_URDF = os.path.join(REPO, "resources/fr3/urdf/fr3.urdf")
CAPS_JSON = os.path.join(REPO, "resources/fr3/urdf/fr3_capsuleized.json")
OUT_MJCF = os.path.join(REPO, "resources/fr3/urdf/fr3_capsules.xml")


def quat_xyzw_to_wxyz(q):
    return [float(q[3]), float(q[0]), float(q[1]), float(q[2])]


def fmt_vec(v, ndig=6):
    return " ".join(f"{float(x):.{ndig}f}" for x in v)


def write_capsule_mjcf(urdf_path=FR3_URDF, caps_json=CAPS_JSON, out_mjcf=OUT_MJCF):
    """Write an MJCF scene file with capsule primitives read from *caps_json*."""
    import pybullet as p

    caps = json.load(open(caps_json))
    urdf_txt = open(urdf_path).read().replace(
        "/workspace/resources/fr3", os.path.join(REPO, "resources/fr3"))
    tmp = "/tmp/fr3_host.urdf"
    open(tmp, "w").write(urdf_txt)

    p.connect(p.DIRECT)
    robot = p.loadURDF(tmp, useFixedBase=True)
    n = p.getNumJoints(robot)
    name2idx = {p.getBodyInfo(robot)[0].decode(): -1}
    for j in range(n):
        name2idx[p.getJointInfo(robot, j)[12].decode()] = j
    p.stepSimulation()

    mujoco = ET.Element("mujoco", {"model": "fr3_capsules"})
    ET.SubElement(mujoco, "compiler", {"angle": "radian", "autolimits": "true"})
    visual = ET.SubElement(mujoco, "visual")
    ET.SubElement(visual, "global", {"offwidth": "1280", "offheight": "800"})
    asset = ET.SubElement(mujoco, "asset")
    worldbody = ET.SubElement(mujoco, "worldbody")
    ET.SubElement(worldbody, "light", {"pos": "0 0 3", "dir": "0 0 -1", "diffuse": "0.8 0.8 0.8"})
    ET.SubElement(worldbody, "geom", {"type": "plane", "size": "2 2 0.1",
                                      "rgba": "0.2 0.25 0.3 1"})

    # gray link meshes for context
    mesh_assets = {}
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
            mf = c[4].decode() if isinstance(c[4], bytes) else c[4]
            if mf not in mesh_assets:
                mname = f"mesh_{len(mesh_assets)}"
                mesh_assets[mf] = mname
                ET.SubElement(asset, "mesh", {"name": mname, "file": mf})
            wpos, worn = p.multiplyTransforms(lw_pos, lw_orn, list(c[5]), list(c[6]))
            ET.SubElement(worldbody, "geom", {
                "type": "mesh", "mesh": mesh_assets[mf],
                "pos": fmt_vec(wpos), "quat": fmt_vec(quat_xyzw_to_wxyz(worn)),
                "rgba": "0.7 0.7 0.78 1"})

    # red capsule primitives
    drawn = 0
    for link, body in caps.items():
        idx = name2idx.get(link)
        if idx is None:
            continue
        if idx < 0:
            lw_pos, lw_orn = p.getBasePositionAndOrientation(robot)
        else:
            ls = p.getLinkState(robot, idx, computeForwardKinematics=True)
            lw_pos, lw_orn = ls[4], ls[5]
        R = np.array(p.getMatrixFromQuaternion(lw_orn)).reshape(3, 3)
        origin = np.array(lw_pos)
        for cp in body["capsules"]:
            P0 = origin + R @ np.array(cp["p0"], dtype=float)
            P1 = origin + R @ np.array(cp["p1"], dtype=float)
            ET.SubElement(worldbody, "geom", {
                "type": "capsule",
                "fromto": fmt_vec(np.concatenate([P0, P1])),
                "size": f"{float(cp['radius']):.6f}",
                "rgba": "1.0 0.15 0.15 0.5"})
            drawn += 1

    tree = ET.ElementTree(mujoco)
    ET.indent(tree, space="  ")
    tree.write(out_mjcf, encoding="utf-8", xml_declaration=True)
    print(f"wrote {out_mjcf} ({drawn} capsules, {len(mesh_assets)} link meshes)")
    p.disconnect()


def main():
    write_capsule_mjcf(FR3_URDF, CAPS_JSON, OUT_MJCF)


if __name__ == "__main__":
    main()
