from __future__ import annotations

import os
import subprocess
import sys


def _python_only_env() -> dict[str, str]:
    env = os.environ.copy()
    env["PYTHONPATH"] = "/workspace/python"
    return env


def test_cli_presets_works_without_built_extension_from_tmp() -> None:
    proc = subprocess.run(
        [sys.executable, "-m", "urdf_approx_geom.cli", "presets"],
        cwd="/tmp",
        env=_python_only_env(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert "convex: default" in proc.stdout
    assert "sphere: default, single" in proc.stdout
    assert "capsule:" in proc.stdout


def test_cli_help_works_without_built_extension_from_tmp() -> None:
    proc = subprocess.run(
        [sys.executable, "-m", "urdf_approx_geom.cli", "--help"],
        cwd="/tmp",
        env=_python_only_env(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert proc.returncode == 0, proc.stdout
    assert "Generate convex, sphere, or capsule" in proc.stdout
