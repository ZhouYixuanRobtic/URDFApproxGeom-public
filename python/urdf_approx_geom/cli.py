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


def _run_compare(args) -> int:
    """Generate convex + sphere + capsule variants and bundle them for robot_viewer.

    Reuses intermediates: the sphere single+default pair is one generator run
    (the single sphere is the default run's biggest_sphere), and every capsule
    preset shares one mesh load + one Manifold pass per link."""
    import tempfile

    from .api import generate, generate_capsule_multi, generate_sphere_pair
    from .robot_viewer import bundle_many

    work = pathlib.Path(tempfile.mkdtemp(prefix="urdf_approx_compare_src_"))
    bundle_dir = pathlib.Path(args.bundle_dir) if args.bundle_dir else pathlib.Path(
        tempfile.mkdtemp(prefix="urdf_approx_compare_"))
    bundle_dir.mkdir(parents=True, exist_ok=True)
    mesh_source = getattr(args, "mesh_source", "visual")
    pairs = [(str(a), str(b)) for a, b in (getattr(args, "replace", []) or [])]
    presets = [p.strip() for p in args.presets.split(",") if p.strip()]
    input_path = pathlib.Path(args.input)
    stem = input_path.stem

    sources: list[pathlib.Path] = []
    # Convex (no preset).
    sources.append(generate("convex", input_path, work / f"{stem}_convex.urdf",
                            replace_pairs=pairs).output_urdf)

    # Sphere: when both single + default are requested, run once via the pair
    # helper (single = default's biggest_sphere). Otherwise emit what's asked.
    sphere_presets = {p for p in presets if p in {"single", "default"}}
    if {"single", "default"} <= sphere_presets:
        default_res, single_res = generate_sphere_pair(
            input_path,
            work / f"{stem}_sphere_default.urdf",
            work / f"{stem}_sphere_single.urdf",
            simplify=True, mesh_source=mesh_source, replace_pairs=pairs,
        )
        sources += [default_res.output_urdf, single_res.output_urdf]
    else:
        for sp in sorted(sphere_presets):
            sources.append(generate("sphere", input_path, work / f"{stem}_sphere_{sp}.urdf",
                                    preset=sp, simplify=True, mesh_source=mesh_source,
                                    replace_pairs=pairs).output_urdf)

    # Capsule: one mesh load per link, every preset on the cached mesh.
    # ("single"/"default"/"high_detail" are valid for both sphere and capsule.)
    if presets:
        cap_outputs = [(work / f"{stem}_capsule_{p}.urdf", p) for p in presets]
        for res in generate_capsule_multi(input_path, cap_outputs, mesh_source=mesh_source,
                                          replace_pairs=pairs):
            sources.append(res.output_urdf)

    bundled = bundle_many(sources, bundle_dir)
    print(f"compare: bundled {len(bundled)} URDFs into {bundle_dir}")
    for src, out in zip(sources, bundled):
        print(f"  {src.name} -> {out.name}")
    print(
        "\nOpen robot_viewer and drag this directory onto the file tree:\n"
        f"  {bundle_dir}\n"
        "Then click each URDF to compare visual (.dae) vs the approximation.\n"
        "Install robot_viewer: https://github.com/fan-ziqi/robot_viewer"
    )
    return 0


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
    gen.add_argument("--mesh-source", default="visual", choices=["visual", "collision"],
                     help="sphere/capsule: fit the visual mesh (default) or the collision mesh")
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
    cmp_parser.add_argument("--urdf", required=True, help="source URDF the sidecars were generated from")
    cmp_parser.add_argument("--max-capv-aabb", type=float, default=2.50)
    cmp_parser.add_argument("--max-r-binmed", type=float, default=1.45)
    cmp_parser.add_argument(
        "--require-improvement",
        action="store_true",
        help="fail if candidate worst capV/aabb or r/binMed is worse than baseline",
    )

    viz = sub.add_parser("visualize", help="visualize generated geometry")
    viz.add_argument("--mode", required=True, choices=["capsule", "sphere", "convex"],
                     help="generated mode carried by the URDF")
    viz.add_argument("--urdf", required=True, help="generated output URDF to visualize")
    viz.add_argument("--viewer", default="robot_viewer", choices=["robot_viewer", "mjcf", "pybullet"],
                     help="visualizer backend; robot_viewer (default) shows visual+collision side by side")
    viz.add_argument("--json", default="", help="capsule JSON sidecar (required for mjcf/pybullet)")
    viz.add_argument("--bundle-dir", default="", help="robot_viewer bundle output dir (default: temp)")
    viz.add_argument("--png", default="", help="pybullet: render to PNG instead of GUI")
    viz.add_argument("--mjcf", default="", help="mjcf: output MJCF path")
    viz.add_argument("--no-launch", action="store_true", help="robot_viewer: do not launch the dev server")

    cmp = sub.add_parser(
        "compare-all",
        help="generate every approximation variant and bundle them for robot_viewer side-by-side comparison",
    )
    cmp.add_argument("-i", "--input", required=True, help="input mesh URDF")
    cmp.add_argument("--bundle-dir", default="", help="bundle output dir (default: temp)")
    cmp.add_argument("--presets", default="single,default,high_detail",
                     help="comma-separated capsule/sphere presets to include")
    cmp.add_argument("--mesh-source", default="visual", choices=["visual", "collision"],
                     help="sphere/capsule: fit the visual mesh (default) or the collision mesh")
    _add_replace_arg(cmp)
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
                mesh_source=args.mesh_source,
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
            mesh_source=args.mesh_source,
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
            args.urdf,
            args.max_capv_aabb,
            args.max_r_binmed,
            require_improvement=args.require_improvement,
        )

    if args.command == "compare-all":
        return _run_compare(args)

    if args.command == "visualize":
        if args.viewer == "robot_viewer":
            from .robot_viewer import bundle, open_in_robot_viewer

            import tempfile
            bundle_dir = args.bundle_dir or tempfile.mkdtemp(prefix="urdf_approx_rv_")
            bundle_urdf = bundle(args.urdf, bundle_dir)
            print(f"{args.mode}: bundled {args.urdf} -> {bundle_urdf}")
            return open_in_robot_viewer(bundle_urdf, launch=not args.no_launch)

        if not args.json:
            parser.error(f"--json is required for --viewer {args.viewer}")
        if args.viewer == "mjcf" and not args.mjcf:
            parser.error("--mjcf PATH is required for --viewer mjcf")

        from .visualization import visualize_capsules

        visualize_capsules(args.urdf, args.json, png=args.png, mjcf=args.mjcf)
        return 0

    parser.error(f"unknown command {args.command}")


if __name__ == "__main__":
    raise SystemExit(main())
