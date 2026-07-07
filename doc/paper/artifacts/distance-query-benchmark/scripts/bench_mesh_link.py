#!/usr/bin/env python3
"""bench_mesh_link.py — Benchmark ONE link's mesh signed-distance.

Usage: python3 bench_mesh_link.py <link_name>
Example: python3 bench_mesh_link.py fr3_link0

This isolates each link in its own process to avoid trimesh cumulative crashes.
"""

import sys, os, time, csv
from pathlib import Path
import numpy as np

_script = Path(__file__).resolve()
REPO_ROOT = _script.parents[8]
PAPER_DIR = _script.parents[3]
DATA_DIR = PAPER_DIR / "artifacts" / "distance-query-benchmark" / "data"
RESOURCES = REPO_ROOT / "resources"

FR3_MESH_DIR = RESOURCES / "fr3" / "meshes" / "fr3" / "collision"
HAND_MESH_DIR = RESOURCES / "fr3" / "meshes" / "franka_hand" / "collision"
PLATE_MESH_DIR = RESOURCES / "fr3" / "meshes" / "plate" / "collision"

LINK_MESH_MAP = {
    "fr3_link0": (FR3_MESH_DIR, "link0.stl"),
    "fr3_link1": (FR3_MESH_DIR, "link1.stl"),
    "fr3_link2": (FR3_MESH_DIR, "link2.stl"),
    "fr3_link3": (FR3_MESH_DIR, "link3.stl"),
    "fr3_link4": (FR3_MESH_DIR, "link4.stl"),
    "fr3_link5": (FR3_MESH_DIR, "link5.stl"),
    "fr3_link6": (FR3_MESH_DIR, "link6.stl"),
    "fr3_link7": (FR3_MESH_DIR, "link7.stl"),
    "hand_hand": (HAND_MESH_DIR, "hand.stl"),
    "hand_leftfinger": (PLATE_MESH_DIR, "flex_griper_connect.stl"),
    "hand_rightfinger": (PLATE_MESH_DIR, "flex_griper_connect.stl"),
}

RANDOM_SEED = 42
QUERIES_PER_LINK = 100_000


def main():
    import trimesh

    if len(sys.argv) < 2:
        print("Usage: bench_mesh_link.py <link_name>")
        sys.exit(1)
    link_name = sys.argv[1]

    if link_name not in LINK_MESH_MAP:
        print(f"Unknown link: {link_name}")
        sys.exit(1)

    mesh_dir, mesh_name = LINK_MESH_MAP[link_name]
    path = mesh_dir / mesh_name
    if not path.exists():
        print(f"Mesh not found: {path}")
        sys.exit(1)

    # Load mesh
    m = trimesh.load(str(path))
    if isinstance(m, trimesh.Scene):
        meshes = [g for g in m.geometry.values() if isinstance(g, trimesh.Trimesh)]
        if not meshes:
            print(f"No Trimesh in scene: {path}")
            sys.exit(1)
        m = trimesh.util.concatenate(meshes)

    # Generate points
    lo, hi = m.bounds
    pad = (hi - lo) * 0.2
    rng = np.random.default_rng(RANDOM_SEED + hash(link_name) % (2**31))
    n_inner = int(QUERIES_PER_LINK * 0.9)
    n_outer = QUERIES_PER_LINK - n_inner
    inner = rng.uniform(lo - pad, hi + pad, size=(n_inner, 3))
    outer = rng.uniform(lo - pad * 3, hi + pad * 3, size=(n_outer, 3))
    pts = np.vstack([inner, outer]).astype(np.float64)

    fd = m.nearest.signed_distance

    # Measure
    import time

    for _ in range(2):
        fd(pts)

    elapsed = []
    for _ in range(5):
        t0 = time.perf_counter()
        fd(pts)
        t1 = time.perf_counter()
        elapsed.append((t1 - t0) * 1e9)

    elapsed = np.array(elapsed)
    ns_per = elapsed / pts.shape[0]
    ns_per_sorted = np.sort(ns_per)

    result = {
        "link": link_name,
        "query_count": QUERIES_PER_LINK,
        "total_time_ms": round(float(np.sum(elapsed)) / 1e6, 3),
        "ns_per_point_query": round(float(np.median(ns_per)), 2),
        "ns_per_point_query_p95": round(float(ns_per_sorted[int(len(ns_per_sorted) * 0.95)]), 2),
        "ns_per_point_query_std": round(float(np.std(ns_per)), 2),
        "backend": "mesh",
        "warmup": 2,
        "repeats": 5,
        "random_seed": RANDOM_SEED,
    }

    # Write result
    os.makedirs(DATA_DIR, exist_ok=True)
    csv_path = DATA_DIR / "mesh_timing.csv"
    write_header = not csv_path.exists()
    with open(csv_path, "a", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(result.keys()))
        if write_header:
            w.writeheader()
        w.writerow(result)

    print(
        f"{link_name}: median={result['ns_per_point_query']:.0f} ns/query, p95={result['ns_per_point_query_p95']:.0f} ns/query"
    )


if __name__ == "__main__":
    main()
