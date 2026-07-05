from __future__ import annotations

import os
import subprocess
import sys

from urdf_approx_geom import generate


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def _env():
    env = os.environ.copy()
    env["PYTHONPATH"] = "/workspace/python:/workspace/build/python"
    return env


def test_validate_command_works_outside_repo_root(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    result = generate("capsule", FR3_URDF, out, preset="default")

    proc = subprocess.run(
        [
            sys.executable,
            "-m",
            "urdf_approx_geom.cli",
            "validate",
            "--mode",
            "capsule",
            "--json",
            str(result.json_path),
            "--urdf",
            FR3_URDF,
            "--max-capv-aabb",
            "2.50",
            "--max-r-binmed",
            "1.50",
        ],
        cwd="/tmp",
        env=_env(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert '"all_covered": true' in proc.stdout


def test_visualize_mjcf_command_works_outside_repo_root(tmp_path):
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
            "--viewer",
            "mjcf",
            "--json",
            str(result.json_path),
            "--mjcf",
            str(mjcf),
        ],
        cwd="/tmp",
        env=_env(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert mjcf.exists()
    assert "<mujoco" in mjcf.read_text()
