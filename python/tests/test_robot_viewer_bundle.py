"""Bundle + CLI smoke for the robot_viewer visualizer."""

from __future__ import annotations

import pathlib
import subprocess
import sys
import xml.etree.ElementTree as ET

from urdf_approx_geom import generate
from urdf_approx_geom.robot_viewer import bundle


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def _bundle_has_only_relative_meshes(urdf_path: pathlib.Path) -> bool:
    tree = ET.parse(urdf_path)
    for mesh in tree.iter("mesh"):
        fn = mesh.get("filename", "")
        if fn.startswith("/") or fn.startswith("package://"):
            return False
    return True


def test_bundle_copies_meshes_and_rewrites_paths(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    result = generate("capsule", FR3_URDF, out, preset="default")
    bundle_dir = tmp_path / "rv"
    bundled = bundle(result.output_urdf, bundle_dir)

    assert bundled.is_file()
    assert (bundle_dir / "meshes").is_dir()
    mesh_files = list((bundle_dir / "meshes").iterdir())
    assert mesh_files, "no meshes copied into bundle"
    assert any(p.suffix == ".dae" for p in mesh_files), "visual .dae not bundled"
    assert _bundle_has_only_relative_meshes(bundled), "bundle still has absolute/package mesh paths"


def test_cli_visualize_robot_viewer_no_launch(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    result = generate("capsule", FR3_URDF, out, preset="default")
    bundle_dir = tmp_path / "cli_bundle"
    proc = subprocess.run(
        [
            sys.executable, "-m", "urdf_approx_geom.cli", "visualize",
            "--mode", "capsule",
            "--urdf", str(result.output_urdf),
            "--viewer", "robot_viewer",
            "--bundle-dir", str(bundle_dir),
            "--no-launch",
        ],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
        env={**__import__("os").environ,
             "PYTHONPATH": "/workspace/python:/workspace/build/python"},
    )
    assert proc.returncode == 0, proc.stdout
    assert "bundle ready" in proc.stdout
    bundled = bundle_dir / "fr3_capsule.urdf"
    assert bundled.is_file()
    assert _bundle_has_only_relative_meshes(bundled)
