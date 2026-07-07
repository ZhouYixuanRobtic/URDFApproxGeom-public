#!/usr/bin/env bash
# run_benchmark.sh — Run the distance-query benchmark.
#
# Phase 1: Analytic-only benchmark (fast, ~1 minute for all 5 JSONs)
# Phase 2: Mesh baseline benchmark (slower, run per-JSON)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BENCH_SCRIPT="$SCRIPT_DIR/benchmark_distance_queries.py"
OUTDIR="$SCRIPT_DIR/../data"
mkdir -p "$OUTDIR"

echo "=== Phase 1: Analytic-only benchmark ==="
PYTHONUNBUFFERED=1 python3 -u "$BENCH_SCRIPT" --analytic-only 2>&1
echo "Done. CSV at $OUTDIR/distance_query_timing.csv"
