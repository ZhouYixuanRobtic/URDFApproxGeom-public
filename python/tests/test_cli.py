"""Tests for the ergonomic generate() wrapper + CLI dispatch."""
import json
import pathlib

import pytest

from urdf_approx_geom_cli import generate

FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_generate_capsule(tmp_path):
    out = tmp_path / "caps.urdf"
    msg = generate("capsule", FR3_URDF, str(out))
    assert "successful" in msg.lower(), msg
    jpath = pathlib.Path(str(out).replace(".urdf", ".json"))
    assert json.loads(jpath.read_text()), "no JSON sidecar"


def test_generate_unknown_mode_raises():
    with pytest.raises(ValueError):
        generate("bogus", "in.urdf", "out.urdf")
