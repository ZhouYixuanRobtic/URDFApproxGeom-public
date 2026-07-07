#!/usr/bin/env python3
"""Generate URDF wrapper files for each synthetic mesh.

Each URDF contains a single link with the mesh as visual geometry,
plus a dummy world link and fixed joint.

Outputs:
  doc/paper/artifacts/synthetic-morphology-benchmark/urdf/*.urdf
"""
import pathlib
import xml.etree.ElementTree as ET

ARTIFACT_DIR = pathlib.Path(__file__).resolve().parents[1]
MESH_DIR = ARTIFACT_DIR / "meshes"
URDF_DIR = ARTIFACT_DIR / "urdf"
URDF_DIR.mkdir(parents=True, exist_ok=True)

SHAPES = [
    "straight_tube",
    "tapered_tube",
    "elbow_tube",
    "t_branch",
    "flat_plate",
]


def _make_urdf(name: str, mesh_abs: str) -> str:
    """Create a minimal single-link URDF with absolute mesh path."""
    root = ET.Element("robot", attrib={"name": name})

    # World link
    ET.SubElement(root, "link", attrib={"name": "world"})

    # Main link
    link = ET.SubElement(root, "link", attrib={"name": name})

    # Visual
    visual = ET.SubElement(link, "visual")
    visual_geom = ET.SubElement(visual, "geometry")
    ET.SubElement(visual_geom, "mesh", attrib={"filename": mesh_abs, "scale": "1 1 1"})

    # Collision (same as visual for capsule fitting)
    collision = ET.SubElement(link, "collision")
    coll_geom = ET.SubElement(collision, "geometry")
    ET.SubElement(coll_geom, "mesh", attrib={"filename": mesh_abs, "scale": "1 1 1"})

    # Fixed joint from world to link
    joint = ET.SubElement(root, "joint", attrib={"name": f"joint_world_{name}", "type": "fixed"})
    ET.SubElement(joint, "parent", attrib={"link": "world"})
    ET.SubElement(joint, "child", attrib={"link": name})
    ET.SubElement(joint, "origin", attrib={"xyz": "0 0 0", "rpy": "0 0 0"})

    ET.indent(root, space="  ")
    return ET.tostring(root, encoding="unicode", xml_declaration=True)


def main():
    for name in SHAPES:
        mesh_path = MESH_DIR / f"{name}.obj"
        if not mesh_path.exists():
            print(f"WARNING: mesh not found for {name}, skipping")
            continue

        urdf_content = _make_urdf(name, str(mesh_path))
        urdf_path = URDF_DIR / f"{name}.urdf"
        urdf_path.write_text(urdf_content)
        print(f"Written: {urdf_path}  (mesh={mesh_path})")


if __name__ == "__main__":
    main()
