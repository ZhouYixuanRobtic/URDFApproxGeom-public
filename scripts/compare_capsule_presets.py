#!/usr/bin/env python3
import argparse
import json
import subprocess
import sys


def load_metrics(path):
    proc = subprocess.run(
        [sys.executable, "scripts/check_capsule_coverage.py", "--caps-json", path, "--json"],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if proc.returncode != 0:
        print(proc.stdout)
        raise SystemExit(proc.returncode)
    return json.loads(proc.stdout)


def worst(metrics, key):
    return max(row[key] for row in metrics["links"])


def count(metrics):
    return sum(row["capsules"] for row in metrics["links"])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sparse-json", required=True)
    ap.add_argument("--tight-json", required=True)
    ap.add_argument("--max-capv-aabb", type=float, default=2.35,
                    help="absolute capV/aabb ceiling for tight preset")
    ap.add_argument("--max-r-binmed", type=float, default=1.45,
                    help="absolute r/binMed ceiling for tight preset")
    args = ap.parse_args()

    sparse = load_metrics(args.sparse_json)
    tight = load_metrics(args.tight_json)
    if not sparse["all_covered"] or not tight["all_covered"]:
        print("coverage failed for sparse or tight preset", file=sys.stderr)
        return 1

    sparse_capv = worst(sparse, "capV_aabb")
    tight_capv = worst(tight, "capV_aabb")
    sparse_ratio = worst(sparse, "r_binMed")
    tight_ratio = worst(tight, "r_binMed")
    sparse_count = count(sparse)
    tight_count = count(tight)

    print(json.dumps({
        "sparse_count": sparse_count,
        "tight_count": tight_count,
        "sparse_worst_capV_aabb": sparse_capv,
        "tight_worst_capV_aabb": tight_capv,
        "sparse_worst_r_binMed": sparse_ratio,
        "tight_worst_r_binMed": tight_ratio,
    }, indent=2, sort_keys=True))

    print("comparison is informational; pass/fail uses absolute tight preset ceilings")
    if tight_capv > args.max_capv_aabb:
        print(f"tight preset capV/aabb {tight_capv:.2f} exceeds ceiling {args.max_capv_aabb:.2f}",
              file=sys.stderr)
        return 1
    if tight_ratio > args.max_r_binmed:
        print(f"tight preset r/binMed {tight_ratio:.2f} exceeds ceiling {args.max_r_binmed:.2f}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
