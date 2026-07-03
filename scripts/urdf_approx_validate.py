#!/usr/bin/env python3
"""Shared validation helpers for generated approximation outputs."""

from __future__ import annotations

import json
import subprocess
import sys


def _capsule_metrics(caps_json: str, urdf: str = "resources/fr3/urdf/fr3.urdf") -> dict:
    proc = subprocess.run(
        [
            sys.executable,
            "scripts/check_capsule_coverage.py",
            "--caps-json",
            caps_json,
            "--urdf",
            urdf,
            "--json",
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if proc.returncode != 0:
        print(proc.stdout)
        raise SystemExit(proc.returncode)
    return json.loads(proc.stdout)


def _worst(metrics: dict, key: str) -> float:
    rows = metrics.get("links", [])
    return max((float(row[key]) for row in rows), default=0.0)


def _count(metrics: dict) -> int:
    return sum(int(row["capsules"]) for row in metrics.get("links", []))


def validate_capsule_metrics(metrics: dict, max_capv_aabb: float, max_r_binmed: float) -> list[str]:
    failures: list[str] = []
    if not metrics.get("all_covered", False):
        failures.append("not all links are covered")
    for row in metrics.get("links", []):
        if float(row["capV_aabb"]) > max_capv_aabb:
            failures.append(f"{row['link']}: capV/aabb {row['capV_aabb']:.2f} > {max_capv_aabb:.2f}")
        if float(row["r_binMed"]) > max_r_binmed:
            failures.append(f"{row['link']}: r/binMed {row['r_binMed']:.2f} > {max_r_binmed:.2f}")
    return failures


def validate_capsule_file(caps_json: str, urdf: str, max_capv_aabb: float, max_r_binmed: float) -> int:
    metrics = _capsule_metrics(caps_json, urdf)
    print(json.dumps(metrics, indent=2, sort_keys=True))
    failures = validate_capsule_metrics(metrics, max_capv_aabb, max_r_binmed)
    if failures:
        print("validation failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


def compare_capsule_files(
    baseline_json: str,
    candidate_json: str,
    max_capv_aabb: float,
    max_r_binmed: float,
) -> int:
    baseline = _capsule_metrics(baseline_json)
    candidate = _capsule_metrics(candidate_json)
    summary = {
        "baseline_count": _count(baseline),
        "candidate_count": _count(candidate),
        "baseline_worst_capV_aabb": _worst(baseline, "capV_aabb"),
        "candidate_worst_capV_aabb": _worst(candidate, "capV_aabb"),
        "baseline_worst_r_binMed": _worst(baseline, "r_binMed"),
        "candidate_worst_r_binMed": _worst(candidate, "r_binMed"),
    }
    print(json.dumps(summary, indent=2, sort_keys=True))
    failures = validate_capsule_metrics(candidate, max_capv_aabb, max_r_binmed)
    if failures:
        print("candidate validation failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0
