#!/usr/bin/env python3
"""benchmark_mesh_only.py — Benchmark mesh signed-distance queries in isolation.

Run once per JSON file (trimesh becomes unstable after ~2 JSON files of batch queries).
Usage: python3 benchmark_mesh_only.py [--json fr3_capsule_default.json]
"""

import sys, json, os, time, csv
from pathlib import Path

import numpy as np

# Paths
_script = Path(__file__).resolve()
REPO_ROOT = _script.parents[8]
WORKTREE_ROOT = _script.parents[5]
PAPER_DIR = _script.parents[3]
WORK_DIR = REPO_ROOT / "doc" / "paper" / "artifacts" / "exp-fr3-mode-comparison" / "work"
ARTIFACT_DIR = PAPER_DIR / "artifacts" / "distance-query-benchmark"
DATA_DIR = ARTIFACT_DIR / "data"

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


def generate_points(mesh_dir, mesh_name, n=QUERIES_PER_LINK, seed=RANDOM_SEED):
    import trimesh

    path = mesh_dir / mesh_name
    if not path.exists():
        return None
    m = trimesh.load(str(path))
    if isinstance(m, trimesh.Scene):
        meshes = [g for g in m.geometry.values() if isinstance(g, trimesh.Trimesh)]
        if not meshes:
            return None
        m = trimesh.util.concatenate(meshes)
    lo, hi = m.bounds
    pad = (hi - lo) * 0.2
    rng = np.random.default_rng(seed)
    n_inner = int(n * 0.9)
    n_outer = n - n_inner
    inner = rng.uniform(lo - pad, hi + pad, size=(n_inner, 3))
    outer = rng.uniform(lo - pad * 3, hi + pad * 3, size=(n_outer, 3))
    pts = np.vstack([inner, outer])
    rng.shuffle(pts, axis=0)
    return pts.astype(np.float64)


def bench_ns(fn, pts, warmup=2, repeats=5):
    """Return dict of timing stats in ns."""
    for _ in range(warmup):
        fn(pts)
    elapsed = []
    for _ in range(repeats):
        t0 = time.perf_counter()
        fn(pts)
        t1 = time.perf_counter()
        elapsed.append((t1 - t0) * 1e9)
    elapsed = np.array(elapsed)
    ns_per = elapsed / pts.shape[0]
    ns_per_sorted = np.sort(ns_per)
    return {
        "total_time_ns": float(np.sum(elapsed)),
        "ns_per_query_median": float(np.median(ns_per)),
        "ns_per_query_p95": float(ns_per_sorted[int(len(ns_per_sorted) * 0.95)]),
        "ns_per_query_std": float(np.std(ns_per)),
        "warmup": warmup,
        "repeats": repeats,
        "query_count": pts.shape[0],
    }


def main():
    import trimesh

    os.makedirs(DATA_DIR, exist_ok=True)
    csv_path = DATA_DIR / "mesh_timing.csv"

    # Determine which JSON to use for link list
    if len(sys.argv) > 2 and sys.argv[1] == "--json":
        json_name = sys.argv[2]
    else:
        json_name = "fr3_capsule_default.json"

    json_path = WORK_DIR / json_name
    if not json_path.exists():
        print(f"[FATAL] JSON not found: {json_path}")
        sys.exit(1)

    with open(json_path) as f:
        raw = json.load(f)

    link_names = sorted(k for k in raw.keys() if k in LINK_MESH_MAP)
    print(f"Mesh-only benchmark for {len(link_names)} links from {json_name}")

    fieldnames = [
        "link",
        "query_count",
        "total_time_ms",
        "ns_per_point_query",
        "ns_per_point_query_p95",
        "ns_per_point_query_std",
        "backend",
        "warmup",
        "repeats",
        "random_seed",
    ]

    with open(csv_path, mode="w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()

        for link_name in link_names:
            mesh_dir, mesh_name = LINK_MESH_MAP[link_name]
            print(f"\n  {link_name} -> {mesh_name} ...", end=" ", flush=True)
            pts = generate_points(mesh_dir, mesh_name, seed=RANDOM_SEED + hash(link_name) % (2**31))
            if pts is None:
                print("BLOCKED (mesh not found)")
                writer.writerow({"link": link_name, "backend": "mesh", "note": "BLOCKED"})
                continue
            try:
                m = trimesh.load(str(mesh_dir / mesh_name))
                fd = m.nearest.signed_distance
                # warmup the AABB tree
                fd(np.array([[0.0, 0.0, 0.0]], dtype=np.float64))
                timing = bench_ns(fd, pts, warmup=2, repeats=5)
                row = {
                    "link": link_name,
                    "query_count": QUERIES_PER_LINK,
                    "total_time_ms": round(timing["total_time_ns"] / 1e6, 3),
                    "ns_per_point_query": round(timing["ns_per_query_median"], 2),
                    "ns_per_point_query_p95": round(timing["ns_per_query_p95"], 2),
                    "ns_per_point_query_std": round(timing["ns_per_query_std"], 2),
                    "backend": "mesh",
                    "warmup": timing["warmup"],
                    "repeats": timing["repeats"],
                    "random_seed": RANDOM_SEED,
                }
                writer.writerow(row)
                print(f"median={timing['ns_per_query_median']:.0f} ns/q")
            except Exception as e:
                print(f"FAIL: {e}")
                writer.writerow({"link": link_name, "backend": "mesh", "note": f"FAIL: {e}"})

    print(f"\nDone. Mesh timing saved to {csv_path}")


if __name__ == "__main__":
    main()
