import subprocess
import sys

from urdf_approx_geom import generate


FR3_URDF = "/workspace/resources/fr3/urdf/fr3.urdf"


def run_cli(*args):
    return subprocess.run(
        [sys.executable, "-m", "urdf_approx_geom.cli", *args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def test_validate_capsule_cli_passes_default_preset(tmp_path):
    out = tmp_path / "fr3_capsule.urdf"
    result = generate("capsule", FR3_URDF, out, preset="default")
    proc = run_cli(
        "validate",
        "--mode",
        "capsule",
        "--json",
        str(result.json_path),
        "--urdf",
        FR3_URDF,
        "--max-capv-aabb",
        "2.50",
        "--max-r-binmed",
        "1.50",
    )
    assert proc.returncode == 0, proc.stdout
    assert "all_covered" in proc.stdout


def test_compare_capsule_cli_uses_absolute_candidate_contract(tmp_path):
    out_a = tmp_path / "a.urdf"
    out_b = tmp_path / "b.urdf"
    baseline = generate("capsule", FR3_URDF, out_a, preset="single")
    candidate = generate("capsule", FR3_URDF, out_b, preset="default")
    proc = run_cli(
        "compare",
        "--mode",
        "capsule",
        "--baseline-json",
        str(baseline.json_path),
        "--candidate-json",
        str(candidate.json_path),
        "--urdf",
        FR3_URDF,
        "--max-capv-aabb",
        "2.50",
        "--max-r-binmed",
        "1.50",
    )
    assert proc.returncode == 0, proc.stdout
    assert "candidate" in proc.stdout


def test_compare_capsule_cli_can_require_relative_improvement(tmp_path):
    out_a = tmp_path / "baseline.urdf"
    out_b = tmp_path / "candidate.urdf"
    baseline = generate("capsule", FR3_URDF, out_a, preset="default")
    candidate = generate("capsule", FR3_URDF, out_b, preset="single")
    proc = run_cli(
        "compare",
        "--mode",
        "capsule",
        "--baseline-json",
        str(baseline.json_path),
        "--candidate-json",
        str(candidate.json_path),
        "--urdf",
        FR3_URDF,
        "--max-capv-aabb",
        "2.50",
        "--max-r-binmed",
        "1.50",
        "--require-improvement",
    )
    assert proc.returncode == 0, proc.stdout
    assert "candidate_worst_capV_aabb" in proc.stdout


def test_compare_capsule_cli_fails_when_candidate_worsens_relative_metrics(tmp_path):
    out_a = tmp_path / "baseline.urdf"
    out_b = tmp_path / "candidate.urdf"
    baseline = generate("capsule", FR3_URDF, out_a, preset="single")
    candidate = generate("capsule", FR3_URDF, out_b, preset="default")
    proc = run_cli(
        "compare",
        "--mode",
        "capsule",
        "--baseline-json",
        str(baseline.json_path),
        "--candidate-json",
        str(candidate.json_path),
        "--urdf",
        FR3_URDF,
        "--max-capv-aabb",
        "2.50",
        "--max-r-binmed",
        "1.50",
        "--require-improvement",
    )
    assert proc.returncode == 1, proc.stdout
    assert "candidate capV/aabb worsened" in proc.stdout
    assert "candidate r/binMed worsened" in proc.stdout
