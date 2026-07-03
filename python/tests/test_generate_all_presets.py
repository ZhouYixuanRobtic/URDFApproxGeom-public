from __future__ import annotations

import json
import subprocess
import sys

from urdf_approx_geom import generate_all


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_generate_all_global_single_uses_default_for_convex(tmp_path):
    results = generate_all(FR3_URDF, tmp_path, preset="single")
    assert [r.mode for r in results] == ["convex", "sphere", "capsule"]
    assert (tmp_path / "fr3_convex.urdf").exists()

    sphere = next(r for r in results if r.mode == "sphere")
    capsule = next(r for r in results if r.mode == "capsule")

    sphere_data = json.loads(sphere.json_path.read_text())
    capsule_data = json.loads(capsule.json_path.read_text())
    assert all(len(body.get("spheres", [])) == 1 for body in sphere_data.values())
    assert all(len(body.get("capsules", [])) == 1 for body in capsule_data.values())


def test_generate_all_accepts_per_mode_presets(tmp_path):
    results = generate_all(
        FR3_URDF,
        tmp_path,
        presets={"sphere": "single", "capsule": "default", "convex": "default"},
    )
    assert [r.mode for r in results] == ["convex", "sphere", "capsule"]
    capsule = next(r for r in results if r.mode == "capsule")
    capsule_data = json.loads(capsule.json_path.read_text())
    assert sum(len(body.get("capsules", [])) for body in capsule_data.values()) > 1


def test_cli_all_accepts_per_mode_preset_flags(tmp_path):
    proc = subprocess.run(
        [
            sys.executable,
            "-m",
            "urdf_approx_geom.cli",
            "generate",
            "--mode",
            "all",
            "-i",
            FR3_URDF,
            "--output-dir",
            str(tmp_path),
            "--sphere-preset",
            "single",
            "--capsule-preset",
            "single",
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert "convex:" in proc.stdout
    assert "sphere:" in proc.stdout
    assert "capsule:" in proc.stdout
