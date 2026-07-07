#!/usr/bin/env python3
"""
benchmark_distance_queries.py — Benchmark distance-query timing for analytic sphere/capsule
primitives vs. mesh baselines.

Reads sidecar JSON files from exp-fr3-mode-comparison, loads the corresponding
collision meshes, generates deterministic query points around each link's AABB,
and reports per-query timing.
"""

import json
import os
import sys
import time
import csv
import random
import warnings
from pathlib import Path

import numpy as np

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
# Script lives in the worktree:
#   .claude/worktrees/paper-visual-mesh/doc/paper/artifacts/distance-query-benchmark/scripts/
# parents[8] = SpherizedURDFGenerator repo root
# parents[5] = worktree root (paper-visual-mesh/)
# parents[3] = worktree's doc/paper/
_script = Path(__file__).resolve()
REPO_ROOT = _script.parents[8]
WORKTREE_ROOT = _script.parents[5]
PAPER_DIR = _script.parents[3]  # worktree's doc/paper/

# Input JSON sidecars live in the MAIN repo tree (not the worktree):
#   <REPO_ROOT>/doc/paper/artifacts/exp-fr3-mode-comparison/work/
WORK_DIR = REPO_ROOT / "doc" / "paper" / "artifacts" / "exp-fr3-mode-comparison" / "work"

# Output artifacts go to the worktree's artifacts tree:
#   doc/paper/artifacts/distance-query-benchmark/
ARTIFACT_DIR = PAPER_DIR / "artifacts" / "distance-query-benchmark"
DATA_DIR = ARTIFACT_DIR / "data"
SCRIPT_DIR = ARTIFACT_DIR / "scripts"

RESOURCES = REPO_ROOT / "resources"
FR3_MESH_DIR = RESOURCES / "fr3" / "meshes" / "fr3" / "collision"
HAND_MESH_DIR = RESOURCES / "fr3" / "meshes" / "franka_hand" / "collision"
PLATE_MESH_DIR = RESOURCES / "fr3" / "meshes" / "plate" / "collision"

# ---------------------------------------------------------------------------
# Link → mesh filename mapping
# ---------------------------------------------------------------------------
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


# ===================================================================
# Analytic distance helpers  (vectorised numpy)
# ===================================================================


def dist_sphere_points(pts, centers, radii):
    """Signed distance from each point to each sphere; return min per point.
    pts:      (N, 3)
    centers:  (M, 3)
    radii:    (M,)
    returns:  (N,)  signed distance to closest sphere
    """
    # ||pt - center||_2 - radius, for all pairs
    diff = pts[:, np.newaxis, :] - centers[np.newaxis, :, :]  # (N, M, 3)
    dist = np.linalg.norm(diff, axis=2) - radii[np.newaxis, :]  # (N, M)
    return np.min(dist, axis=1)


def dist_capsule_points(pts, p0s, p1s, radii):
    """Signed distance from each point to each capsule; return min per point.
    pts:     (N, 3)
    p0s:     (M, 3)
    p1s:     (M, 3)
    radii:   (M,)
    returns: (N,)  signed distance to closest capsule
    """
    N = pts.shape[0]
    M = p0s.shape[0]
    axes = p1s - p0s  # (M, 3)
    lensq = np.sum(axes * axes, axis=1)  # (M,)

    # For each capsule, compute closest point on segment
    # Expand: pts  (N,1,3),  p0s (1,M,3),  axes (1,M,3)
    p_exp = pts[:, np.newaxis, :]  # (N,1,3)
    p0_exp = p0s[np.newaxis, :, :]  # (1,M,3)
    ax_exp = axes[np.newaxis, :, :]  # (1,M,3)
    lensq_exp = lensq[np.newaxis, :]  # (1,M)

    t = np.sum((p_exp - p0_exp) * ax_exp, axis=2)  # (N,M)
    # Avoid divide-by-zero for zero-length axis (degenerate → treat as sphere)
    safe_lensq = np.where(lensq_exp == 0, 1.0, lensq_exp)
    t = t / safe_lensq
    t = np.clip(t, 0.0, 1.0)
    closest = p0_exp + t[:, :, np.newaxis] * ax_exp  # (N,M,3)

    diff = p_exp - closest  # (N,M,3)
    dist = np.linalg.norm(diff, axis=2) - radii[np.newaxis, :]  # (N,M)
    # For zero-length capsules, use original sphere distance
    zero_len = lensq_exp == 0
    if np.any(zero_len):
        sphere_dist = np.linalg.norm(p_exp - p0_exp, axis=2) - radii[np.newaxis, :]
        dist = np.where(zero_len, sphere_dist, dist)

    return np.min(dist, axis=1)


# ===================================================================
# Mesh distance baseline  (lazy import trimesh)
# ===================================================================


def _load_mesh(mesh_dir, mesh_name):
    """Load a mesh file via trimesh. Returns trimesh.Trimesh or None."""
    path = mesh_dir / mesh_name
    if not path.exists():
        return None
    import trimesh

    m = trimesh.load(str(path))
    if isinstance(m, trimesh.Scene):
        # Concatenate scene geometry into a single mesh
        meshes = [g for g in m.geometry.values() if isinstance(g, trimesh.Trimesh)]
        if not meshes:
            return None
        m = trimesh.util.concatenate(meshes)
    return m


def _proximity_query(mesh, pts_batch):
    """Batch signed-distance query on a mesh via trimesh ProximityQuery."""
    return mesh.nearest.signed_distance(pts_batch)  # (N,) signed distances


def build_mesh_sampler(mesh_dir, mesh_name):
    """Build a callable that returns signed distances for query points.
    Returns None if mesh not available.
    """
    m = _load_mesh(mesh_dir, mesh_name)
    if m is None:
        return None
    # Force AABB tree build by calling once
    try:
        _ = m.nearest.signed_distance(np.array([[0.0, 0.0, 0.0]], dtype=np.float64))
    except Exception:
        pass

    def query(pts):
        return _proximity_query(m, pts)

    return query


# ===================================================================
# Query-point generation  (deterministic, sampled around link AABB)
# ===================================================================

RANDOM_SEED = 42
QUERIES_PER_LINK = 100_000


def generate_query_points(mesh_dir, mesh_name, num_points, seed=RANDOM_SEED, expansion=0.2):
    """Generate deterministic query points around the mesh AABB.

    Points are sampled uniformly within the AABB expanded by `expansion`
    fraction on each side, plus a small fraction of outliers to exercise
    far-field queries.

    Returns (N,3) array of query points.
    """
    m = _load_mesh(mesh_dir, mesh_name)
    if m is None:
        return None
    bounds = m.bounds  # (2, 3)  [[min], [max]]
    lo = bounds[0]
    hi = bounds[1]
    extents = hi - lo
    pad = extents * expansion
    lo_pad = lo - pad
    hi_pad = hi + pad

    rng = np.random.default_rng(seed)
    # 90% uniform inside padded AABB, 10% in wider region for far-field
    n_inner = int(num_points * 0.9)
    n_outer = num_points - n_inner

    inner = rng.uniform(lo_pad, hi_pad, size=(n_inner, 3))
    outer_lo = lo_pad - pad * 2
    outer_hi = hi_pad + pad * 2
    outer = rng.uniform(outer_lo, outer_hi, size=(n_outer, 3))
    pts = np.vstack([inner, outer])
    rng.shuffle(pts, axis=0)
    return pts


# ===================================================================
# Timing helpers
# ===================================================================


def bench_call(fn, pts_batch, warmup=3, repeats=11):
    """Benchmark `fn(pts_batch)` with warm-up and repeats.

    Returns dict with timing info in nanoseconds.
    """
    b = np.asarray(pts_batch, dtype=np.float64)
    n = b.shape[0]

    # Warm-up
    for _ in range(warmup):
        fn(b)

    # Timed repeats
    elapsed = []
    for _ in range(repeats):
        t0 = time.perf_counter()
        fn(b)
        t1 = time.perf_counter()
        elapsed.append((t1 - t0) * 1e9)  # ns

    elapsed = np.array(elapsed)
    ns_per = elapsed / n
    ns_per_sorted = np.sort(ns_per)
    median = float(np.median(ns_per))
    p95 = float(ns_per_sorted[int(len(ns_per_sorted) * 0.95)])
    mean = float(np.mean(ns_per))
    std = float(np.std(ns_per))
    total_ns = float(np.sum(elapsed))

    return {
        "total_time_ns": total_ns,
        "ns_per_query_mean": mean,
        "ns_per_query_median": median,
        "ns_per_query_p95": p95,
        "ns_per_query_std": std,
        "warmup": warmup,
        "repeats": repeats,
        "query_count": n,
    }


# ===================================================================
# Parse sidecar JSON → primitives
# ===================================================================


def parse_sidecar(filepath):
    """Parse a sidecar JSON and return {link_name: {type: data}}."""
    with open(filepath) as f:
        raw = json.load(f)
    out = {}
    for link_name, val in raw.items():
        if isinstance(val, dict):
            # Capsule format: {"capsules": [...]}
            # Sphere format: {"spheres": [...], "BiggestSphere": [...], "SubSpheres": [...]}
            if "capsules" in val:
                caps = np.array(
                    [(c["p0"], c["p1"], c["radius"]) for c in val["capsules"]], dtype=object
                )
                out[link_name] = {
                    "type": "capsule",
                    "p0": np.array([c["p0"] for c in val["capsules"]], dtype=np.float64),
                    "p1": np.array([c["p1"] for c in val["capsules"]], dtype=np.float64),
                    "radius": np.array([c["radius"] for c in val["capsules"]], dtype=np.float64),
                    "count": len(val["capsules"]),
                }
            elif "spheres" in val:
                out[link_name] = {
                    "type": "sphere",
                    "center": np.array([s["center"] for s in val["spheres"]], dtype=np.float64),
                    "radius": np.array([s["radius"] for s in val["spheres"]], dtype=np.float64),
                    "count": len(val["spheres"]),
                }
            else:
                # Unknown dict format — skip
                pass
        elif (
            isinstance(val, list)
            and len(val) > 0
            and isinstance(val[0], dict)
            and "center" in val[0]
        ):
            # Sphere format (flat list variant)
            out[link_name] = {
                "type": "sphere",
                "center": np.array([s["center"] for s in val], dtype=np.float64),
                "radius": np.array([s["radius"] for s in val], dtype=np.float64),
                "count": len(val),
            }
    return out


# ===================================================================
# Main benchmark loop
# ===================================================================


def benchmark_sidecar(json_path, output_csv_writer, analytic_only=False):
    """Run all benchmarks for one sidecar JSON file."""
    json_path = Path(json_path)
    basename = json_path.stem  # e.g. fr3_capsule_default

    # Parse mode and preset from filename
    # Expected format: fr3_{mode}_{preset}  or  fr3_{mode}{preset}
    parts = basename.split("_")
    if len(parts) >= 3 and parts[0] == "fr3":
        if parts[1] in ("sphere", "capsule"):
            mode = parts[1]
            preset = "_".join(parts[2:]) if parts[2:] else "default"
        else:
            print(f"  [SKIP] Cannot parse mode from {basename}")
            return
    else:
        print(f"  [SKIP] Unexpected filename format: {basename}")
        return

    print(f"\n{'='*70}")
    print(f"Benchmarking: {basename}  (mode={mode}, preset={preset})")
    print(f"{'='*70}")

    primitives = parse_sidecar(json_path)
    if not primitives:
        print("  [FAIL] No primitives found in sidecar.")
        return

    print(f"  Links with primitives: {len(primitives)}")
    for lnk, pdata in primitives.items():
        print(f"    {lnk}: {pdata['count']} {pdata['type']}(s)")

    # For each link, benchmark analytic + mesh
    for link_name in sorted(primitives.keys()):
        pdata = primitives[link_name]
        count = pdata["count"]

        print(f"\n  --- {link_name} ({count} {pdata['type']}s) ---")

        # ---- Generate query points (from mesh) ----
        mesh_info = LINK_MESH_MAP.get(link_name)
        if mesh_info is None:
            print(f"    [SKIP] No mesh mapping for {link_name}")
            continue
        mesh_dir, mesh_name = mesh_info

        pts = generate_query_points(
            mesh_dir, mesh_name, QUERIES_PER_LINK, seed=RANDOM_SEED + hash(link_name) % (2**31)
        )
        if pts is None:
            print(f"    [SKIP] Cannot load mesh {mesh_name} for {link_name}")
            continue

        # ---- Analytic benchmark ----
        if pdata["type"] == "sphere":

            def make_analytic_fn(center, radius):
                return lambda q: dist_sphere_points(q, center, radius)

            analytic_fn = make_analytic_fn(pdata["center"], pdata["radius"])
        elif pdata["type"] == "capsule":

            def make_analytic_fn(p0, p1, radius):
                return lambda q: dist_capsule_points(q, p0, p1, radius)

            analytic_fn = make_analytic_fn(pdata["p0"], pdata["p1"], pdata["radius"])
        else:
            print(f"    [SKIP] Unknown primitive type: {pdata['type']}")
            continue

        # Time analytic
        try:
            a_timing = bench_call(analytic_fn, pts, warmup=3, repeats=11)
        except Exception as e:
            print(f"    [FAIL] Analytic benchmark error: {e}")
            a_timing = None

        if a_timing:
            print(
                f"    Analytic:  median={a_timing['ns_per_query_median']:.1f} ns/query, "
                f"p95={a_timing['ns_per_query_p95']:.1f} ns/query"
            )

        # ---- Mesh baseline ----
        m_timing = None
        if not analytic_only:
            mesh_query_fn = build_mesh_sampler(mesh_dir, mesh_name)
            if mesh_query_fn is None:
                print(f"    Mesh:      [BLOCKED] mesh not loadable")
            else:
                try:
                    # Fewer repeats for mesh (slower backend)
                    m_timing = bench_call(mesh_query_fn, pts, warmup=2, repeats=5)
                    print(
                        f"    Mesh:      median={m_timing['ns_per_query_median']:.1f} ns/query, "
                        f"p95={m_timing['ns_per_query_p95']:.1f} ns/query"
                    )
                except Exception as e:
                    print(f"    [FAIL] Mesh benchmark error: {e}")
        else:
            print(
                f"    Mesh:      [BLOCKED] trimesh signed_distance crashes on sustained batch workload"
            )

        # ---- Write row(s) ----
        row = {
            "mode": mode,
            "preset": preset,
            "link": link_name,
            "primitive_count": count,
            "query_count": QUERIES_PER_LINK,
            "random_seed": RANDOM_SEED,
        }

        if a_timing:
            row_a = row.copy()
            row_a["backend"] = f"analytic_{pdata['type']}"
            row_a["total_time_ms"] = round(a_timing["total_time_ns"] / 1e6, 3)
            row_a["ns_per_point_query"] = round(a_timing["ns_per_query_median"], 2)
            row_a["ns_per_point_query_p95"] = round(a_timing["ns_per_query_p95"], 2)
            row_a["ns_per_point_query_std"] = round(a_timing["ns_per_query_std"], 2)
            row_a["warmup"] = a_timing["warmup"]
            row_a["repeats"] = a_timing["repeats"]
            output_csv_writer.writerow(row_a)

        if m_timing:
            row_m = row.copy()
            row_m["backend"] = "mesh"
            row_m["total_time_ms"] = round(m_timing["total_time_ns"] / 1e6, 3)
            row_m["ns_per_point_query"] = round(m_timing["ns_per_query_median"], 2)
            row_m["ns_per_point_query_p95"] = round(m_timing["ns_per_query_p95"], 2)
            row_m["ns_per_point_query_std"] = round(m_timing["ns_per_query_std"], 2)
            row_m["warmup"] = m_timing["warmup"]
            row_m["repeats"] = m_timing["repeats"]
            output_csv_writer.writerow(row_m)
        else:
            row_m = row.copy()
            row_m["backend"] = "mesh"
            row_m["total_time_ms"] = ""
            row_m["ns_per_point_query"] = ""
            row_m["ns_per_point_query_p95"] = ""
            row_m["ns_per_point_query_std"] = ""
            row_m["warmup"] = ""
            row_m["repeats"] = ""
            row_m["note"] = "BLOCKED: mesh file not loadable with trimesh"
            output_csv_writer.writerow(row_m)


# ===================================================================
# Main
# ===================================================================


def main():
    os.makedirs(DATA_DIR, exist_ok=True)
    os.makedirs(SCRIPT_DIR, exist_ok=True)

    analytic_only = "--analytic-only" in sys.argv

    csv_path = DATA_DIR / "distance_query_timing.csv"

    # Find JSON sidecar files
    json_files = sorted(WORK_DIR.glob("*.json"))
    if not json_files:
        print(f"[FATAL] No JSON sidecar files found in {WORK_DIR}")
        print("Checked paths:")
        print(f"  WORK_DIR = {WORK_DIR}")
        print(f"  Resolved = {WORK_DIR.resolve()}")
        sys.exit(1)

    print(f"Found {len(json_files)} sidecar JSON files:")
    for jf in json_files:
        print(f"  {jf.name}")
    print(f"Analytic-only mode: {analytic_only}")

    # Write CSV header
    fieldnames = [
        "mode",
        "preset",
        "link",
        "primitive_count",
        "query_count",
        "total_time_ms",
        "ns_per_point_query",
        "ns_per_point_query_p95",
        "ns_per_point_query_std",
        "backend",
        "warmup",
        "repeats",
        "random_seed",
        "note",
    ]

    with open(csv_path, mode="w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()

        for jf in json_files:
            benchmark_sidecar(jf, writer, analytic_only)

    print(f"\n{'='*70}")
    print(f"Done! Results saved to: {csv_path}")
    print(f"{'='*70}")


if __name__ == "__main__":
    main()
