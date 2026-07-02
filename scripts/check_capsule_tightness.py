#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--caps-json", default="resources/fr3/urdf/fr3_capsuleized.json")
    parser.add_argument("--urdf", default="resources/fr3/urdf/fr3.urdf")
    parser.add_argument("--max-capv-aabb", type=float, default=3.0)
    parser.add_argument("--max-r-binmed", type=float, default=2.0)
    parser.add_argument("--max-capsules", type=int, default=24)
    args = parser.parse_args()

    if not os.path.exists(args.caps_json):
        print(f"capsule json does not exist: {args.caps_json}", file=sys.stderr)
        return 2

    proc = subprocess.run(
        [
            sys.executable,
            "scripts/check_capsule_coverage.py",
            "--caps-json", args.caps_json,
            "--urdf", args.urdf,
            "--json",
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if proc.returncode != 0:
        print(proc.stdout)
        return proc.returncode

    result = json.loads(proc.stdout)
    print(json.dumps(result, indent=2, sort_keys=True))
    if not result["all_covered"]:
        print("coverage gate failed: not all links are covered", file=sys.stderr)
        return 1

    failures = []
    for row in result["links"]:
        if row["capsules"] > args.max_capsules:
            failures.append(f"{row['link']}: capsules {row['capsules']} > {args.max_capsules}")
        if row["capV_aabb"] > args.max_capv_aabb:
            failures.append(f"{row['link']}: capV/aabb {row['capV_aabb']:.2f} > {args.max_capv_aabb:.2f}")
        if row["r_binMed"] > args.max_r_binmed:
            failures.append(f"{row['link']}: r/binMed {row['r_binMed']:.2f} > {args.max_r_binmed:.2f}")

    if failures:
        print("tightness gate failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
