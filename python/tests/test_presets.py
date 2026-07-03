import pathlib

import pytest

from urdf_approx_geom.presets import available_presets, resolve_preset


def test_resolve_capsule_presets():
    assert resolve_preset("capsule", "single").name == "single.yml"
    assert resolve_preset("capsule", "default").name == "default.yml"
    assert resolve_preset("capsule", "high_detail").name == "high_detail.yml"


def test_resolve_sphere_presets():
    assert resolve_preset("sphere", "single").name == "single.yml"
    assert resolve_preset("sphere", "default").name == "default.yml"


def test_convex_default_preset_exists_for_uniform_cli():
    path = resolve_preset("convex", "default")
    assert path.name == "default.yml"
    assert path.exists()


def test_unknown_preset_error_lists_choices():
    with pytest.raises(ValueError, match="available presets"):
        resolve_preset("capsule", "maximum")


def test_available_presets_are_paths():
    presets = available_presets("capsule")
    assert isinstance(presets["default"], pathlib.Path)
    assert presets["default"].exists()
