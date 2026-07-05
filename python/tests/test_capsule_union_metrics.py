import importlib.util
import pathlib

import numpy as np
import pytest


ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "check_capsule_coverage.py"


def load_metrics_module():
    spec = importlib.util.spec_from_file_location("check_capsule_coverage", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def test_capsule_union_volume_does_not_double_count_identical_capsules():
    mod = load_metrics_module()
    p0 = np.array([0.0, 0.0, 0.0], dtype=float)
    p1 = np.array([1.0, 0.0, 0.0], dtype=float)
    cap = (p0, p1, 0.10)

    single = mod.estimate_capsule_union_volume([cap], samples_per_axis=48)
    duplicate = mod.estimate_capsule_union_volume([cap, cap], samples_per_axis=48)
    primitive_sum = mod.capsule_volume(p0, p1, 0.10) * 2.0

    assert duplicate == pytest.approx(single, rel=0.04)
    assert duplicate < primitive_sum * 0.60


def test_tightness_metrics_reports_union_capv_aabb_not_primitive_sum():
    mod = load_metrics_module()
    p0 = np.array([0.0, 0.0, 0.0], dtype=float)
    p1 = np.array([1.0, 0.0, 0.0], dtype=float)
    cap = (p0, p1, 0.10)
    V = np.array(
        [
            [-0.2, -0.2, -0.2],
            [1.2, 0.2, 0.2],
        ],
        dtype=float,
    )
    assigned = np.array([0, 0], dtype=int)

    capv_aabb, radius_ratio, union_volume, primitive_sum, primitive_capv_aabb = (
        mod.tightness_metrics(V, [cap, cap], assigned, volume_samples=32)
    )

    aabb_volume = float(np.prod(np.maximum(V.max(axis=0) - V.min(axis=0), 1e-12)))
    assert capv_aabb == pytest.approx(union_volume / aabb_volume, rel=1e-12)
    assert primitive_capv_aabb == pytest.approx(primitive_sum / aabb_volume, rel=1e-12)
    assert capv_aabb < primitive_capv_aabb * 0.75


def test_parse_mesh_sources_prefers_visual_when_requested():
    mod = load_metrics_module()
    sources = mod.parse_mesh_sources(str(ROOT / "resources/fr3/urdf/fr3.urdf"), "visual")
    assert "fr3_link0" in sources
    assert sources["fr3_link0"][2].endswith("/visual/link0.dae")
