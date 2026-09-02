# Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
# All Rights Reserved.

from __future__ import annotations

import json

import pytest

from urdf_approx_geom import generate
from urdf_approx_geom._extension import load_extension


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"
HAS_SPHERE_TREE = hasattr(load_extension(), "spherized")


@pytest.mark.skipif(not HAS_SPHERE_TREE, reason="sphere-tree is research-edition only")
def test_default_sphere_schema_has_canonical_spheres_array(tmp_path):
    out = tmp_path / "fr3_spherized.urdf"
    result = generate("sphere", FR3_URDF, out, preset="default")

    assert result.json_path is not None
    assert result.json_path.exists()
    assert result.primitive_count > 0

    data = json.loads(result.json_path.read_text())
    mesh_links = [body for body in data.values() if isinstance(body, dict) and body.get("spheres")]
    assert mesh_links
    for body in mesh_links:
        assert isinstance(body["spheres"], list)
        assert body["spheres"]
        first = body["spheres"][0]
        assert sorted(first) == ["center", "radius"]
        assert len(first["center"]) == 3
        assert first["radius"] > 0.0
