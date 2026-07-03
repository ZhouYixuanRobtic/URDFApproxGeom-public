"""CLI entry point for URDF approximate collision geometry generation.

Run with ``python -m urdf_approx_geom.cli`` or via the global
``urdf-approx-geom`` console script (if installed).
"""

from __future__ import annotations

import argparse

from .api import generate


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(
        prog="urdf-approx-geom",
        description="Generate sphere / convex / capsule collision geometry from a mesh URDF.",
    )
    p.add_argument("-m", "--mode", required=True,
                   choices=["capsule", "convex", "spherized"], help="geometry mode")
    p.add_argument("-i", "--input", required=True, help="input mesh URDF")
    p.add_argument("-o", "--output", required=True, help="output URDF (capsule also writes <out>.json)")
    p.add_argument("-r", "--replace", nargs=2, action="append", default=[],
                   metavar=("KEY", "VALUE"), help="mesh filename replacement pairs (repeatable)")
    p.add_argument("--config", default="", help="capsule config yml (default: bundled)")
    p.add_argument("--simplify", type=int, default=1, help="spherized: mesh decimation 0/1 (default 1)")
    p.add_argument("--preset", default="default",
                   choices=["default", "single", "high_detail", "tight"],
                   help="named configuration preset (default: default)")
    args = p.parse_args(argv)

    result = generate(
        args.mode,
        args.input,
        args.output,
        config=args.config or None,
        replace_pairs=args.replace,
        simplify=bool(args.simplify),
        preset=args.preset,
    )
    print(result.message)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
