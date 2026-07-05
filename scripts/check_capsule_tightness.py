#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import sys


def parse_link_limits(values, cast, option_name):
    limits = {}
    for value in values:
        if "=" not in value:
            raise SystemExit(f"{option_name} must use LINK=VALUE, got: {value}")
        link, raw = value.split("=", 1)
        link = link.strip()
        if not link:
            raise SystemExit(f"{option_name} has empty link name: {value}")
        try:
            limits[link] = cast(raw)
        except ValueError as exc:
            raise SystemExit(f"{option_name} has invalid value for {link}: {raw}") from exc
    return limits


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--caps-json", required=True)
    parser.add_argument("--urdf", default="resources/fr3/urdf/fr3.urdf")
    parser.add_argument("--mesh-source", default="visual", choices=["visual", "collision"])
    parser.add_argument("--volume-samples", type=int, default=64)
    parser.add_argument("--max-capv-aabb", type=float, default=2.50)
    parser.add_argument("--max-r-binmed", type=float, default=1.45)
    parser.add_argument("--max-capsules", type=int, default=24)
    parser.add_argument("--link-max-capv-aabb", action="append", default=[],
                        help="per-link capV/aabb ceiling as LINK=VALUE")
    parser.add_argument("--link-max-r-binmed", action="append", default=[],
                        help="per-link r/binMed ceiling as LINK=VALUE")
    parser.add_argument("--link-min-capsules", action="append", default=[],
                        help="per-link minimum capsule count as LINK=VALUE")
    parser.add_argument("--link-max-axis-overhang-r", action="append", default=[],
                        help="per-link max assigned axial overhang divided by radius as LINK=VALUE")
    args = parser.parse_args()

    link_max_capv = parse_link_limits(args.link_max_capv_aabb, float, "--link-max-capv-aabb")
    link_max_ratio = parse_link_limits(args.link_max_r_binmed, float, "--link-max-r-binmed")
    link_min_capsules = parse_link_limits(args.link_min_capsules, int, "--link-min-capsules")
    link_max_overhang = parse_link_limits(args.link_max_axis_overhang_r, float, "--link-max-axis-overhang-r")

    if not os.path.exists(args.caps_json):
        print(f"capsule json does not exist: {args.caps_json}", file=sys.stderr)
        return 2

    proc = subprocess.run(
        [
            sys.executable,
            "scripts/check_capsule_coverage.py",
            "--caps-json", args.caps_json,
            "--urdf", args.urdf,
            "--mesh-source", args.mesh_source,
            "--volume-samples", str(args.volume_samples),
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
    seen_links = set()
    for row in result["links"]:
        link = row["link"]
        seen_links.add(link)
        max_capv = link_max_capv.get(link, args.max_capv_aabb)
        max_ratio = link_max_ratio.get(link, args.max_r_binmed)
        min_capsules = link_min_capsules.get(link)

        if min_capsules is not None and row["capsules"] < min_capsules:
            failures.append(f"{link}: capsules {row['capsules']} < {min_capsules}")
        if row["capsules"] > args.max_capsules:
            failures.append(f"{link}: capsules {row['capsules']} > {args.max_capsules}")
        if row["capV_aabb"] > max_capv:
            failures.append(f"{link}: capV/aabb {row['capV_aabb']:.2f} > {max_capv:.2f}")
        if row["r_binMed"] > max_ratio:
            failures.append(f"{link}: r/binMed {row['r_binMed']:.2f} > {max_ratio:.2f}")
        max_overhang = link_max_overhang.get(link)
        if max_overhang is not None and row.get("axis_overhang_r", 0.0) > max_overhang:
            failures.append(f"{link}: axis_overhang/r {row['axis_overhang_r']:.2f} > {max_overhang:.2f}")

    requested_links = set(link_max_capv) | set(link_max_ratio) | set(link_min_capsules) | set(link_max_overhang)
    for missing in sorted(requested_links - seen_links):
        failures.append(f"{missing}: link not present in capsule metrics")

    if failures:
        print("tightness gate failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
