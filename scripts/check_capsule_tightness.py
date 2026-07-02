#!/usr/bin/env python3
import argparse
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--max-capv-aabb", type=float, default=2.10)
    parser.add_argument("--max-r-binmed", type=float, default=1.45)
    args = parser.parse_args()

    proc = subprocess.run(
        [sys.executable, "scripts/check_capsule_coverage.py"],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    print(proc.stdout)
    if proc.returncode != 0:
        return proc.returncode
    if "ALL COVERED: True" not in proc.stdout:
        print("coverage gate failed: ALL COVERED was not True", file=sys.stderr)
        return 1

    failures = []
    for line in proc.stdout.splitlines():
        parts = line.split()
        if len(parts) < 8 or parts[0] in {"link", "ALL"}:
            continue
        link = parts[0]
        try:
            capv_aabb = float(parts[6])
            r_binmed = float(parts[7])
        except ValueError:
            continue
        if capv_aabb > args.max_capv_aabb:
            failures.append(f"{link}: capV/aabb {capv_aabb:.2f} > {args.max_capv_aabb:.2f}")
        if r_binmed > args.max_r_binmed:
            failures.append(f"{link}: r/binMed {r_binmed:.2f} > {args.max_r_binmed:.2f}")

    if failures:
        print("tightness gate failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
