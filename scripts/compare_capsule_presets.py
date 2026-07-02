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

    if tight_count <= sparse_count:
        print("tight preset did not add detail", file=sys.stderr)
        return 1
    if tight_capv > sparse_capv:
        print("tight preset worsened worst capV/aabb", file=sys.stderr)
        return 1
    if tight_ratio > sparse_ratio:
        print("tight preset worsened worst r/binMed", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
