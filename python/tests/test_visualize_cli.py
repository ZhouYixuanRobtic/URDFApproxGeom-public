"""Tests for the visualize CLI subcommand."""

import subprocess
import sys

from urdf_approx_geom import generate

FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_visualize_capsule_mjcf_cli(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    result = generate("capsule", FR3_URDF, out, preset="default")
    mjcf = tmp_path / "fr3_capsules.xml"
    proc = subprocess.run(
        [
            sys.executable,
            "-m",
            "urdf_approx_geom.cli",
            "visualize",
            "--mode",
            "capsule",
            "--urdf",
            FR3_URDF,
            "--json",
            str(result.json_path),
            "--mjcf",
            str(mjcf),
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert mjcf.exists()
    assert "<mujoco" in mjcf.read_text()
