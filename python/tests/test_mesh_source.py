"""Tests for visual-mesh fit source, spherized_pair, capsuleized_multi, and the
preset path regression."""

from __future__ import annotations

import json
import pathlib

import pytest

from urdf_approx_geom import generate, generate_capsule_multi, generate_sphere_pair
from urdf_approx_geom.api import _prepare_visual_meshes
from urdf_approx_geom.presets import resolve_preset


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def test_resolve_preset_no_double_config_prefix():
    """config_root() already returns the config dir; _PRESETS must not re-add
    a `config/` prefix (would yield /workspace/config/config/...)."""
    path = resolve_preset("sphere", "default")
    assert path.exists(), f"preset path missing (doubled config/?): {path}"
    assert "config/config" not in str(path), f"doubled config/ in {path}"


def test_prepare_visual_meshes_finds_and_converts_dae():
    """FR3 visuals are .dae; the helper must convert each to a loadable .obj."""
    pairs, tmp = _prepare_visual_meshes(FR3_URDF, None)
    assert pairs, "expected .dae visual meshes to be converted"
    assert tmp is not None and tmp.is_dir()
    # at least the link0 .dae -> .obj pair, and the .obj exists.
    assert any(p[0].endswith(".dae") and p[1].endswith(".obj") for p in pairs)
    for orig, conv in pairs:
        assert pathlib.Path(conv).is_file(), f"converted .obj missing: {conv}"


def test_prepare_visual_meshes_noop_when_already_obj(tmp_path):
    """A URDF whose visuals are already .obj needs no conversion."""
    urdf = tmp_path / "r.urdf"
    urdf.write_text(
        '<robot name="r">'
        '<link name="l"><visual><geometry><mesh filename="m.obj"/></geometry></visual>'
        '<collision><geometry><mesh filename="m.stl"/></geometry></collision></link>'
        '</robot>'
    )
    pairs, tmp = _prepare_visual_meshes(urdf, None)
    assert pairs == [] and tmp is None


def test_generate_rejects_invalid_mesh_source(tmp_path):
    with pytest.raises(ValueError):
        generate("sphere", FR3_URDF, tmp_path / "x.urdf", mesh_source="bogus")


def test_mesh_source_visual_diverges_from_collision(tmp_path):
    """Fitting the visual .dae must produce a different result from the
    collision .stl -- proves the source flag actually routes the fit."""
    vis = generate("sphere", FR3_URDF, tmp_path / "vis.urdf",
                   preset="single", simplify=True, mesh_source="visual")
    col = generate("sphere", FR3_URDF, tmp_path / "col.urdf",
                   preset="single", simplify=True, mesh_source="collision")
    vj = json.loads(vis.json_path.read_text())
    cj = json.loads(col.json_path.read_text())
    # single-sphere preset emits one sphere per link under "spheres"
    assert vj != cj, "visual and collision fits must differ"


def test_spherized_pair_single_matches_default_biggest(tmp_path):
    """generate_sphere_pair runs the tree once; the single output's per-link
    sphere must equal the default output's BiggestSphere entry."""
    default_out = tmp_path / "default.urdf"
    single_out = tmp_path / "single.urdf"
    default_res, single_res = generate_sphere_pair(
        FR3_URDF, default_out, single_out,
        preset="default", simplify=True, mesh_source="visual",
    )
    assert default_res.output_urdf.is_file() and single_res.output_urdf.is_file()
    assert default_res.json_path.is_file() and single_res.json_path.is_file()

    dj = json.loads(default_res.json_path.read_text())
    sj = json.loads(single_res.json_path.read_text())
    # multi-sphere default must have more primitives than single overall
    assert default_res.primitive_count > single_res.primitive_count
    # every single-output link's sphere == that link's BiggestSphere in default
    for link, body in sj.items():
        spheres = body.get("spheres", [])
        assert len(spheres) == 1, f"{link}: single output should have one sphere"
        bs = dj.get(link, {}).get("BiggestSphere")
        assert bs is not None, f"{link}: missing BiggestSphere in default output"
        # BiggestSphere = [x, y, z, r]; single sphere = {center:[x,y,z], radius:r}
        sph = spheres[0]
        assert sph["center"] == pytest.approx(bs[:3], abs=1e-9)
        assert sph["radius"] == pytest.approx(bs[3], rel=1e-9)


def test_capsule_multi_matches_separate_runs(tmp_path):
    """runMulti must produce the same per-link capsule counts as running each
    preset through generate() separately (one mesh load vs many)."""
    presets = ["single", "default"]
    multi = generate_capsule_multi(
        FR3_URDF, [(tmp_path / f"multi_{p}.urdf", p) for p in presets],
        mesh_source="visual",
    )
    by_preset = {p: r.primitive_count for r, p in zip(multi, presets)}
    # compare against separate runs
    for p in presets:
        sep = generate("capsule", FR3_URDF, tmp_path / f"sep_{p}.urdf",
                       preset=p, mesh_source="visual")
        assert by_preset[p] == sep.primitive_count, (
            f"preset {p}: multi={by_preset[p]} separate={sep.primitive_count}")
