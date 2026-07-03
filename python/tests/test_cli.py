"""Tests for the structured generate() wrapper and CLI dispatch."""

from __future__ import annotations

import json
import pathlib

import pytest

from urdf_approx_geom import generate

FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_generate_capsule(tmp_path):
    out = tmp_path / "caps.urdf"
    result = generate("capsule", FR3_URDF, str(out))
    assert "successful" in result.message.lower(), result.message
    assert result.json_path is not None
    assert json.loads(result.json_path.read_text()), "no JSON sidecar"


def test_generate_unknown_mode_raises():
    with pytest.raises(ValueError, match="unknown mode"):
        generate("bogus", "in.urdf", "out.urdf")
