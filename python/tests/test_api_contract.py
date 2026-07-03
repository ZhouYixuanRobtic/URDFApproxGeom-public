"""Tests for the structured generate / generate_all API."""

from __future__ import annotations

import json
import pathlib

import pytest

from urdf_approx_geom import GenerateResult, generate, generate_all


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_generate_capsule_returns_structured_result(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    result = generate("capsule", FR3_URDF, out, preset="default")
    assert isinstance(result, GenerateResult)
    assert result.mode == "capsule"
    assert result.output_urdf == out
    assert result.json_path == pathlib.Path(str(out).replace(".urdf", ".json"))
    assert result.json_path.exists()
    assert result.primitive_count > 0
    assert json.loads(result.json_path.read_text())


def test_generate_convex_has_no_required_json_sidecar(tmp_path):
    out = tmp_path / "fr3_convex.urdf"
    result = generate("convex", FR3_URDF, out)
    assert result.mode == "convex"
    assert result.output_urdf == out
    assert result.json_path is None
    assert result.primitive_count == 0


def test_generate_all_runs_requested_modes(tmp_path):
    results = generate_all(FR3_URDF, tmp_path, modes=["convex", "capsule"])
    assert [r.mode for r in results] == ["convex", "capsule"]
    assert (tmp_path / "fr3_convex.urdf").exists()
    assert (tmp_path / "fr3_capsule.urdf").exists()


def test_generate_rejects_unknown_mode(tmp_path):
    with pytest.raises(ValueError, match="unknown mode"):
        generate("meshlets", FR3_URDF, tmp_path / "out.urdf")
