#!/usr/bin/env python3
"""Run capsule fitting benchmark on all 5 synthetic shapes.

For each shape, fits capsules using both "default" and "high_detail"
presets, then computes coverage and tightness metrics.

Usage:
  ./run_env.sh scripts/run_benchmark.py

Outputs:
  data/synthetic_morphology_metrics.csv
  data/runtimes.json
  data/diagnostics/*.txt
"""
import csv
import json
import math
import pathlib
import sys
import time
import traceback

import numpy as np

# Add repo paths
REPO = pathlib.Path(__file__).resolve().parents[4]
sys.path.insert(0, str(REPO / "python"))
sys.path.insert(0, str(REPO / "build" / "python"))

from urdf_approx_geom.api import generate
from urdf_approx_geom.presets import resolve_preset

ARTIFACT_DIR = pathlib.Path(__file__).resolve().parents[1]
MESH_DIR = ARTIFACT_DIR / "meshes"
URDF_DIR = ARTIFACT_DIR / "urdf"
DATA_DIR = ARTIFACT_DIR / "data"
DIAG_DIR = DATA_DIR / "diagnostics"
DIAG_DIR.mkdir(parents=True, exist_ok=True)

SHAPES = [
    "straight_tube",
    "tapered_tube",
    "elbow_tube",
    "t_branch",
    "flat_plate",
]
PRESETS = ["default", "high_detail"]
MESH_SOURCE = "collision"  # our URDFs have identical visual+collision


def load_mesh_vertices(shape: str) -> np.ndarray:
    """Load the mesh vertices for metric computation."""
    import trimesh

    mesh_path = MESH_DIR / f"{shape}.obj"
    mesh = trimesh.load_mesh(str(mesh_path))
    return mesh.vertices


def estimate_capsule_union_volume(capsules, samples_per_axis=64, chunk_size=200000):
    """Estimate union volume of a set of capsules via grid sampling."""
    if not capsules:
        return 0.0
    if len(capsules) == 1:
        p0, p1, radius = capsules[0]
        length = float(np.linalg.norm(p1 - p0))
        return math.pi * radius * radius * length + (4.0 / 3.0) * math.pi * radius**3

    # Bounds
    lows, highs = [], []
    for p0, p1, radius in capsules:
        r = np.array([radius, radius, radius], dtype=float)
        lows.append(np.minimum(p0, p1) - r)
        highs.append(np.maximum(p0, p1) + r)
    lo = np.min(np.vstack(lows), axis=0)
    hi = np.max(np.vstack(highs), axis=0)
    ext = hi - lo
    box_volume = float(np.prod(np.maximum(ext, 1e-12)))
    if box_volume <= 0.0:
        return 0.0

    n = max(1, int(samples_per_axis))
    coords = [lo[d] + (np.arange(n, dtype=float) + 0.5) * ext[d] / n for d in range(3)]
    total = n**3
    inside = 0
    for start in range(0, total, chunk_size):
        stop = min(total, start + chunk_size)
        ids = np.arange(start, stop, dtype=np.int64)
        ix = ids // (n * n)
        iy = (ids // n) % n
        iz = ids % n
        pts = np.column_stack((coords[0][ix], coords[1][iy], coords[2][iz]))
        covered = np.zeros(pts.shape[0], dtype=bool)
        for p0, p1, radius in capsules:
            axis = p1 - p0
            denom = float(axis @ axis)
            if denom < 1e-12:
                nearest = np.repeat(p0.reshape(1, 3), pts.shape[0], axis=0)
            else:
                t = np.clip(((pts - p0) @ axis) / denom, 0.0, 1.0)
                nearest = p0 + t[:, None] * axis
            covered |= np.linalg.norm(pts - nearest, axis=1) <= radius
        inside += int(np.count_nonzero(covered))
    return box_volume * (inside / float(total))


def point_to_seg_with_t(p, a, b):
    d = b - a
    denom = float(d @ d)
    t = 0.0 if denom < 1e-12 else float(np.clip((p - a) @ d / denom, 0.0, 1.0))
    q = a + t * d
    return float(np.linalg.norm(p - q)), t


def compute_vertex_metrics(V, capsules):
    """Compute per-vertex worst signed distance and overhang."""
    if not capsules:
        return {
            "worst_signed_distance": float("inf"),
            "r_binMed": float("inf"),
            "surface_worst_signed_distance": float("inf"),
            "surface_uncovered_fraction": 1.0,
        }

    signed = []
    raw = []
    assigned = []
    for v in V:
        best_signed = float("inf")
        best_raw = float("inf")
        best_idx = -1
        for ci, (p0, p1, radius) in enumerate(capsules):
            d = point_to_seg_with_t(v, p0, p1)
            sd = d[0] - radius
            if sd < best_signed:
                best_signed = sd
                best_raw = d[0]
                best_idx = ci
        signed.append(best_signed)
        raw.append(best_raw)
        assigned.append(best_idx)

    signed = np.array(signed, dtype=float)
    raw = np.array(raw, dtype=float)
    assigned = np.array(assigned, dtype=int)

    vertex_worst_sd = float(signed.max())
    surface_uncovered = float(np.count_nonzero(signed > 1e-6) / max(len(V), 1))

    # r_binMed: median of max per-decile distances normalized by radius
    worst_ratio = 0.0
    for ci, (p0, p1, radius) in enumerate(capsules):
        mask = assigned == ci
        if not np.any(mask):
            continue
        if radius < 1e-12:
            continue
        bin_max = []
        for v in V[mask]:
            d, t = point_to_seg_with_t(v, p0, p1)
            slot = min(9, max(0, int(t * 10.0)))
            while len(bin_max) <= slot:
                bin_max.append(0.0)
            bin_max[slot] = max(bin_max[slot], d)
        nonzero = [v for v in bin_max if v > 1e-12]
        if nonzero:
            worst_ratio = max(worst_ratio, radius / float(np.median(nonzero)))

    # capV_aabb ratio
    primitive_sum = sum(
        math.pi * r * r * float(np.linalg.norm(p1 - p0)) + (4.0 / 3.0) * math.pi * r**3
        for p0, p1, r in capsules
    )
    ext = V.max(axis=0) - V.min(axis=0)
    aabb_volume = float(np.prod(np.maximum(ext, 1e-12)))
    # union volume estimation
    union_volume = estimate_capsule_union_volume(capsules, samples_per_axis=64)
    capV_aabb = union_volume / aabb_volume if aabb_volume > 1e-12 else 0.0

    return {
        "worst_signed_distance": vertex_worst_sd,
        "r_binMed": worst_ratio,
        "capV_aabb": capV_aabb,
        "surface_uncovered_fraction": surface_uncovered,
        "primitive_sum": primitive_sum,
        "aabb_volume": aabb_volume,
        "union_volume": union_volume,
    }


def run_fit(shape: str, preset: str) -> dict:
    """Run capsule fitting for one shape+preset and return metrics."""
    urdf_path = URDF_DIR / f"{shape}.urdf"
    out_path = DATA_DIR / f"{shape}_{preset}.urdf"

    if out_path.exists():
        print(f"  Output {out_path} exists, overwriting")

    t0 = time.time()
    result = generate(
        "capsule",
        str(urdf_path),
        str(out_path),
        preset=preset,
        mesh_source=MESH_SOURCE,
    )
    elapsed = time.time() - t0

    # Load JSON sidecar
    json_path = out_path.with_suffix(".json")
    if not json_path.exists():
        return {
            "shape": shape,
            "preset": preset,
            "mode": "capsule",
            "status": "failed",
            "error": f"JSON not found: {json_path}",
            "runtime_s": elapsed,
        }

    caps_data = json.loads(json_path.read_text())
    link_name = shape  # our single-link URDF uses the shape name as link name
    body = caps_data.get(link_name)
    if body is None:
        return {
            "shape": shape,
            "preset": preset,
            "mode": "capsule",
            "status": "failed",
            "error": f"Link {link_name!r} not in JSON keys: {list(caps_data.keys())}",
            "runtime_s": elapsed,
        }

    capsules_raw = body.get("capsules", [])
    if not capsules_raw:
        return {
            "shape": shape,
            "preset": preset,
            "mode": "capsule",
            "status": "failed",
            "error": "No capsules in fit result",
            "runtime_s": elapsed,
        }

    # Convert to numpy arrays
    capsules = [
        (np.array(c["p0"], dtype=float), np.array(c["p1"], dtype=float), float(c["radius"]))
        for c in capsules_raw
    ]

    V = load_mesh_vertices(shape)
    metrics = compute_vertex_metrics(V, capsules)

    return {
        "shape_name": shape,
        "mode": "capsule",
        "preset": preset,
        "status": "ok",
        "primitive_count": len(capsules),
        "vertex_worst_signed_distance_m": metrics["worst_signed_distance"],
        "surface_worst_signed_distance_m": metrics["worst_signed_distance"],
        "surface_uncovered_fraction": metrics["surface_uncovered_fraction"],
        "capV_aabb": metrics["capV_aabb"],
        "r_binMed": metrics["r_binMed"],
        "runtime_s": elapsed,
        "error": "",
    }


def main():
    rows = []
    failures = []

    print("=" * 60)
    print("Synthetic Morphology Benchmark")
    print("=" * 60)

    for shape in SHAPES:
        for preset in PRESETS:
            label = f"{shape} / {preset}"
            print(f"\n[{label}]", flush=True)
            try:
                row = run_fit(shape, preset)
                if row.get("status") == "failed":
                    print(f"  FAILED: {row.get('error', 'unknown error')}")
                    failures.append(row)
                else:
                    print(
                        f"  OK: {row['primitive_count']} capsules, "
                        f"worst_sd={row['vertex_worst_signed_distance_m']:.6f}m, "
                        f"capV/aabb={row['capV_aabb']:.4f}, "
                        f"r/binMed={row['r_binMed']:.4f}, "
                        f"runtime={row['runtime_s']:.2f}s"
                    )
                rows.append(row)
            except Exception as e:
                print(f"  EXCEPTION: {e}")
                traceback.print_exc()
                rows.append(
                    {
                        "shape_name": shape,
                        "mode": "capsule",
                        "preset": preset,
                        "status": "failed",
                        "primitive_count": 0,
                        "vertex_worst_signed_distance_m": float("nan"),
                        "surface_worst_signed_distance_m": float("nan"),
                        "surface_uncovered_fraction": float("nan"),
                        "capV_aabb": float("nan"),
                        "r_binMed": float("nan"),
                        "runtime_s": 0.0,
                        "error": str(e),
                    }
                )

    # Write CSV
    csv_path = DATA_DIR / "synthetic_morphology_metrics.csv"
    fieldnames = [
        "shape_name",
        "mode",
        "preset",
        "status",
        "primitive_count",
        "vertex_worst_signed_distance_m",
        "surface_worst_signed_distance_m",
        "surface_uncovered_fraction",
        "capV_aabb",
        "r_binMed",
        "runtime_s",
        "error",
    ]
    with open(csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow({k: row.get(k, "") for k in fieldnames})
    print(f"\nCSV written: {csv_path}")

    # Summary
    print("\n" + "=" * 60)
    print("Summary")
    print("=" * 60)
    for row in rows:
        if row.get("status") == "ok":
            print(
                f"  {row['shape_name']:15} {row['preset']:12} "
                f"{row['primitive_count']:2} caps  "
                f"worst_sd={row['vertex_worst_signed_distance_m']:+.6f}  "
                f"capV/aabb={row['capV_aabb']:.4f}  "
                f"r/binMed={row['r_binMed']:.4f}  "
                f"t={row['runtime_s']:.2f}s"
            )
        else:
            print(f"  {row['shape_name']:15} {row['preset']:12} FAILED: {row.get('error', '')}")


if __name__ == "__main__":
    main()
