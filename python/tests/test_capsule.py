"""Smoke test for the urdf_approx_geom pybind11 extension (capsule path)."""
import json
import pathlib

import urdf_approx_geom as uag

FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_capsuleized_emits_json_sidecar(tmp_path):
    out = tmp_path / "fr3_capsuleized.urdf"
    msg = uag.capsuleized(FR3_URDF, str(out))
    assert "successful" in msg.lower(), msg

    jpath = pathlib.Path(str(out).replace(".urdf", ".json"))
    data = json.loads(jpath.read_text())
    assert data, "no links emitted"

    for link, body in data.items():
        assert "capsules" in body and len(body["capsules"]) >= 1
        for cp in body["capsules"]:
            assert len(cp["p0"]) == 3 and len(cp["p1"]) == 3
            assert cp["radius"] > 0.0
