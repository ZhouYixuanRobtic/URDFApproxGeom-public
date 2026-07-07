# Result: distance-query-benchmark

status: done
source_request: doc/paper/requests/distance-query-benchmark.md

## Claim Under Test

Analytic sphere and capsule primitive distance queries are substantially faster than mesh-based signed-distance queries, providing timing evidence that the constant-time primitive formulas translate to real throughput advantage.

## Commands

```bash
# Phase 1 — Analytic-only benchmark (all 5 sidecar JSONs, ~1 minute)
python3 doc/paper/artifacts/distance-query-benchmark/scripts/benchmark_distance_queries.py --analytic-only

# Phase 2 — Mesh baseline, one link per process (avoids trimesh cumulative instability)
for link in fr3_link0 fr3_link1 fr3_link2 fr3_link3 fr3_link4 \
            fr3_link5 fr3_link6 fr3_link7 hand_hand hand_leftfinger hand_rightfinger; do
  python3 doc/paper/artifacts/distance-query-benchmark/scripts/bench_mesh_link.py "$link"
done

# Phase 3 — Merge results
python3 doc/paper/artifacts/distance-query-benchmark/scripts/merge_results.py

# Scripts also available:
#   benchmark_distance_queries.py — full benchmark (analytic + mesh, crashes on sustained mesh)
#   run_benchmark.sh             — one-shot launch script
```

## Artifacts

- `data/distance_query_timing.csv`: Complete timing dataset (110 rows: 55 analytic + 55 mesh, across 5 presets x 11 links)
- `data/mesh_timing.csv`: Raw mesh-only timing (11 links)
- `scripts/benchmark_distance_queries.py`: Main benchmark script
- `scripts/bench_mesh_link.py`: Per-link mesh benchmark (process isolation)
- `scripts/merge_results.py`: Merges analytic + mesh CSVs
- `scripts/run_benchmark.sh`: One-shot launch

## Results

### Per-Backend Timing (all links aggregated)

| Backend | Count | Median ns/query | Min ns/query | Max ns/query |
|---|---|---|---:|---:|
| analytic_capsule (all presets) | 33 | 38.5 | 32.8 | 281.8 |
| analytic_sphere (all presets) | 22 | 183.9 | 12.4 | 246.6 |
| mesh (signed_distance, trimesh) | 55 | 27,596 | 19,194 | 72,515 |

### By Mode and Preset (median across 11 links)

| Mode | Preset | Backend | Median ns/query | Range (min-max) |
|---|---|---|---|---:|---:|
| capsule | single | analytic | 36.5 | 32.8 — 38.9 |
| capsule | high_detail | analytic | 42.3 | 37.8 — 281.8 |
| capsule | default | analytic | 51.2 | 37.8 — 273.4 |
| sphere | single | analytic | 12.6 | 12.4 — 12.7 |
| sphere | default | analytic | 229.5 | 183.9 — 246.6 |
| any | any | mesh | 27,596 | 19,194 — 72,515 |

### Speedup: Analytic vs Mesh per Link (median across presets)

| Link | Capsule ns/q | Capsule speedup vs mesh | Sphere ns/q | Sphere speedup vs mesh | Mesh ns/q |
|---|---|---|---:|---:|---:|---:|
| fr3_link0 | 53 | 563x | 240 | 124x | 29,888 |
| fr3_link1 | 50 | 555x | 210 | 131x | 27,596 |
| fr3_link2 | 38 | 677x | 240 | 108x | 25,858 |
| fr3_link3 | 38 | 716x | 184 | 150x | 27,538 |
| fr3_link4 | 38 | 729x | 214 | 130x | 27,788 |
| fr3_link5 | 39 | 831x | 247 | 130x | 32,137 |
| fr3_link6 | 42 | 454x | 213 | 90x | 19,194 |
| fr3_link7 | 38 | 575x | 244 | 89x | 21,726 |
| hand_hand | 38 | 617x | 245 | 96x | 23,604 |
| hand_leftfinger | 216 | 336x | 211 | 344x | 72,515 |
| hand_rightfinger | 226 | 272x | 230 | 267x | 61,307 |

### Key Metrics

| Metric | Value | Unit |
|---|---|---:|---|
| Query points per link (deterministic) | 100,000 | points |
| Random seed | 42 | |
| Analytic warmup / repeats | 3 / 11 | |
| Mesh warmup / repeats | 2 / 5 | |
| Links benchmarked | 11 | |
| Sidecar JSON files processed | 5 | |
| Fastest analytic: sphere single | 12.4 | ns/query |
| Slowest analytic: capsule default (hand) | 281.8 | ns/query |
| Slowest mesh: hand_leftfinger | 72,515 | ns/query |
| Best speedup: capsule default vs mesh (fr3_link5) | 831x | |
| Worst speedup: sphere vs mesh (fr3_link6) | 89x | |

### Methodology Notes

- **Query points**: 100,000 deterministic points per link, generated via numpy `default_rng(seed=42 + hash(link))`. 90% sampled uniformly within the link's AABB padded by 20%; 10% sampled in a wider ring (3x padding) for far-field coverage.
- **Analytic distance**: Fully vectorized numpy implementations. Sphere: `||p - c||_2 - r`, min over all spheres per link. Capsule: project onto segment axis, clamp, compute closest-point distance minus radius, min over all capsules.
- **Mesh baseline**: trimesh `ProximityQuery.signed_distance()`, which uses an AABB tree internally. The mesh was loaded from the same STL files used in the URDF collision geometry.
- **Timing**: `time.perf_counter()` with warm-up then timed repeats. Reported median and p95 across repeats.
- **Mesh baseline note**: trimesh's `signed_distance` becomes unstable under sustained batch workload (signal 16 crash after ~2 JSON files processed). Each link was therefore benchmarked in an isolated process. Mesh timing is independent of the analytic mode/preset, so the same mesh timing is replicated across all mode/preset rows.

## Claim Support Summary

- **supported**: The evidence strongly supports the claim that analytic sphere and capsule primitive distance queries are substantially faster than mesh signed-distance queries.
  - Single-sphere queries: ~12.6 ns/query median (89-150x faster than mesh).
  - Single-capsule queries: ~36-51 ns/query median (272-831x faster than mesh).
  - Multi-primitive (min-over-primitives) queries: ~38-282 ns/query, still 90-729x faster.
- The speedup is consistent across all 11 links of the Franka FR3 arm, from the small gripper links to the large hand assembly.
- The mesh baseline (trimesh AABB-tree signed distance at 19-73 us/query) is representative but not exhaustive; a dedicated collision-checking library (e.g., FCL) would likely be faster than trimesh's pure-Python-wrapped implementation.

## Limitations

1. **Mesh baseline backend**: trimesh was used as the mesh signed-distance backend. While trimesh is a widely-used geometry library, its signed-distance implementation is not optimized for minimum-latency queries (it prioritizes correctness and generality). A dedicated C++ collision checker like FCL or Bullet would likely give faster mesh queries, narrowing the speedup gap. The timings here represent an upper bound on the mesh advantage, not a lower bound.
2. **trimesh instability**: trimesh `signed_distance` repeatedly crashed (signal 16) after processing ~2 JSON files worth of link queries in a single process. Each link was isolated in a separate process to work around this. This instability itself is a practical argument against mesh-based distance in production pipelines.
3. **No kinematic chain**: These benchmarks test static per-link distance queries in link-local frames. They do not measure the end-to-end cost of a full robot-environment collision check with kinematic transforms, broad-phase filtering, or self-collision culling.
4. **Single robot**: Results are for the Franka FR3 only. Other robots with different link geometries may show different ratios.
5. **Vectorized analytic queries**: The analytic implementations use numpy vectorization over all primitives simultaneously. In a C++ simulator, the per-query cost would be even lower (no Python overhead, no numpy dispatch).

## Next Actions For Writer

### Directly usable facts

- Analytic sphere distance queries cost **12-250 ns/query** depending on primitive count (1 sphere: ~13 ns; 8 spheres: ~240 ns).
- Analytic capsule distance queries cost **33-282 ns/query** depending on primitive count (1 capsule: ~36 ns; 3 capsules: ~220 ns).
- Mesh signed-distance queries (trimesh) cost **19,000-72,500 ns/query**.
- Speedup of analytic primitives over mesh is consistently **89-831x**.
- Fastest combination: single-sphere (12.4 ns/query), fastest mesh: fr3_link6 (19,194 ns/query).
- The speedup is largest for the single-capsule preset (36.5 ns median, ~700x over mesh) and remains substantial for the default and high-detail presets with multiple primitives.
- These numbers measure per-primitive distance only, not end-to-end collision checking.

### Statements to avoid

- Do NOT claim "three orders of magnitude" — the speedup is 89-831x (solidly 2 orders, approaching 3 for some links but not all).
- Do NOT claim "C++ FCL would be similarly slow" — FCL is likely faster than trimesh.
- Do NOT claim "distance query timing proves real-time collision checking" — these are micro-benchmarks of a single operation, not end-to-end system performance.
- Do NOT claim the mesh baseline represents all mesh backends.
