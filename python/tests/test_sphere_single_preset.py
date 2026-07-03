import json

from urdf_approx_geom import generate


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_single_sphere_preset_emits_one_sphere_per_link(tmp_path):
    out = tmp_path / "fr3_single_sphere.urdf"
    result = generate("sphere", FR3_URDF, out, preset="single", simplify=False)
    assert result.mode == "sphere"
    assert result.json_path is not None
    data = json.loads(result.json_path.read_text())
    assert data
    assert all(len(body.get("spheres", [])) == 1 for body in data.values())
