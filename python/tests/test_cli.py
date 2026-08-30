# Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
# All Rights Reserved.

"""Tests for the unified CLI with subcommands."""

from __future__ import annotations

import subprocess
import sys

from urdf_approx_geom._extension import load_extension


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"
HAS_SPHERE_TREE = hasattr(load_extension(), "spherized")


def run_cli(*args):
    return subprocess.run(
        [sys.executable, "-m", "urdf_approx_geom.cli", *args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def test_generate_capsule_cli(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    proc = run_cli(
        "generate", "--mode", "capsule", "-i", FR3_URDF, "-o", str(out), "--preset", "default"
    )
    assert proc.returncode == 0, proc.stdout
    assert out.exists()
    assert out.with_suffix(".json").exists()
    assert "capsule" in proc.stdout


def test_generate_all_cli(tmp_path):
    # Commercial edition has no sphere/default; single is the shared preset.
    preset = "default" if HAS_SPHERE_TREE else "single"
    proc = run_cli(
        "generate",
        "--mode",
        "all",
        "-i",
        FR3_URDF,
        "--output-dir",
        str(tmp_path),
        "--preset",
        preset,
    )
    assert proc.returncode == 0, proc.stdout
    assert (tmp_path / "fr3_convex.urdf").exists()
    assert (tmp_path / "fr3_spherized.urdf").exists()
    assert (tmp_path / "fr3_capsule.urdf").exists()


def test_list_presets_cli():
    proc = run_cli("presets")
    assert proc.returncode == 0, proc.stdout
    assert "capsule: default" in proc.stdout
    assert "single" in proc.stdout
