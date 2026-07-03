"""Command line interface for URDF approximate collision geometry."""

from __future__ import annotations

import argparse
import pathlib

from .api import generate, generate_all
from .presets import available_presets


def _add_replace_arg(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "-r",
        "--replace",
        nargs=2,
        action="append",
        default=[],
        metavar=("KEY", "VALUE"),
        help="mesh filename replacement pair; repeat for multiple replacements",
    )


def _print_result(result) -> None:
    json_text = f", json={result.json_path}" if result.json_path else ""
    cfg_text = f", config={result.config_path}" if result.config_path else ""
    print(
        f"{result.mode}: output={result.output_urdf}{json_text}{cfg_text}, "
        f"primitives={result.primitive_count}, message={result.message}"
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="urdf-approx-geom",
        description="Generate convex, sphere, or capsule collision approximations from a mesh URDF.",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    gen = sub.add_parser("generate", help="generate one mode or all modes")
    gen.add_argument("--mode", required=True, choices=["convex", "sphere", "spherized", "capsule", "all"])
    gen.add_argument("-i", "--input", required=True, help="input mesh URDF")
    gen.add_argument("-o", "--output", help="output URDF for a single mode")
    gen.add_argument("--output-dir", help="output directory when --mode all is used")
    gen.add_argument("--preset", default="default", help="named preset for sphere/capsule")
    gen.add_argument("--convex-preset", default=None, help="convex preset for --mode all")
    gen.add_argument("--sphere-preset", default=None, help="sphere preset for --mode all")
    gen.add_argument("--capsule-preset", default=None, help="capsule preset for --mode all")
    gen.add_argument("--config", default=None, help="explicit config path for a single mode")
    gen.add_argument("--simplify", type=int, default=1, help="sphere mode mesh simplification flag 0/1")
    _add_replace_arg(gen)

    sub.add_parser("presets", help="list built-in named presets")

    val = sub.add_parser("validate", help="validate generated output metrics")
    val.add_argument("--mode", required=True, choices=["capsule", "sphere"])
    val.add_argument("--json", required=True, help="generated JSON sidecar")
    val.add_argument("--urdf", default="resources/fr3/urdf/fr3.urdf", help="source URDF for mesh metrics")
    val.add_argument("--max-capv-aabb", type=float, default=2.50)
    val.add_argument("--max-r-binmed", type=float, default=1.45)

    cmp_parser = sub.add_parser("compare", help="compare two generated JSON sidecars")
    cmp_parser.add_argument("--mode", required=True, choices=["capsule"])
    cmp_parser.add_argument("--baseline-json", required=True)
    cmp_parser.add_argument("--candidate-json", required=True)
    cmp_parser.add_argument("--max-capv-aabb", type=float, default=2.50)
    cmp_parser.add_argument("--max-r-binmed", type=float, default=1.45)

    viz = sub.add_parser("visualize", help="visualize generated geometry")
    viz.add_argument("--mode", required=True, choices=["capsule"])
    viz.add_argument("--urdf", required=True)
    viz.add_argument("--json", required=True)
    viz.add_argument("--png", default="")
    viz.add_argument("--mjcf", default="")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.command == "presets":
        for mode in ("convex", "sphere", "capsule"):
            names = sorted(available_presets(mode))
            print(f"{mode}: {', '.join(names)}")
        return 0

    if args.command == "generate":
        if args.mode == "all":
            if not args.output_dir:
                parser.error("--output-dir is required when --mode all is used")
            per_mode_presets = {
                mode: value
                for mode, value in {
                    "convex": args.convex_preset,
                    "sphere": args.sphere_preset,
                    "capsule": args.capsule_preset,
                }.items()
                if value is not None
            }
            results = generate_all(
                args.input,
                args.output_dir,
                preset=args.preset,
                presets=per_mode_presets,
                replace_pairs=args.replace,
                simplify=bool(args.simplify),
            )
            for result in results:
                _print_result(result)
            return 0
        if not args.output:
            parser.error("-o/--output is required for single-mode generation")
        result = generate(
            args.mode,
            args.input,
            args.output,
            preset=args.preset,
            config=args.config,
            replace_pairs=args.replace,
            simplify=bool(args.simplify),
        )
        _print_result(result)
        return 0

    if args.command == "validate":
        from .validation import validate_capsule_file

        if args.mode == "capsule":
            return validate_capsule_file(args.json, args.urdf, args.max_capv_aabb, args.max_r_binmed)
        parser.error("sphere validation is not implemented in this release")

    if args.command == "compare":
        from .validation import compare_capsule_files

        return compare_capsule_files(
            args.baseline_json,
            args.candidate_json,
            args.max_capv_aabb,
            args.max_r_binmed,
        )

    if args.command == "visualize":
        from .visualization import visualize_capsules

        visualize_capsules(args.urdf, args.json, png=args.png, mjcf=args.mjcf)
        return 0

    parser.error(f"unknown command {args.command}")


if __name__ == "__main__":
    raise SystemExit(main())
