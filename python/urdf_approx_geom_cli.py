"""Ergonomic Python layer + CLI over the compiled `urdf_approx_geom` extension.

The extension is built by CMake into ``<repo>/build/python``. When this module
is imported before install, it auto-discovers that directory so a researcher can
run ``python -m urdf_approx_geom_cli --mode capsule -i in.urdf -o out.urdf``
straight from a source checkout.
"""
import os
import sys

try:
    import urdf_approx_geom as _uag
except ImportError:  # auto-discover the CMake-built extension
    _repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    sys.path.insert(0, os.path.join(_repo, "build", "python"))
    import urdf_approx_geom as _uag

__all__ = ["generate", "capsuleized", "convex", "spherized"]

capsuleized = _uag.capsuleized
convex = _uag.convex
spherized = _uag.spherized

_MODES = {"capsule", "convex", "sphere", "spherized", "sphere_tree", "sphere-tree"}


def generate(mode, input, output, replace_pairs=None, simplify=True, config=""):
    """Run one collision-geometry approximation.

    mode: "capsule" | "convex" | "spherized"
    replace_pairs: list of (key, value) str pairs applied to mesh filenames.
    """
    rp = [tuple(p) for p in (replace_pairs or [])]
    m = mode.lower()
    if m == "capsule":
        return _uag.capsuleized(input, output, config, rp)
    if m == "convex":
        return _uag.convex(input, output, rp)
    if m in ("sphere", "spherized", "sphere_tree", "sphere-tree"):
        return _uag.spherized(input, output, rp, simplify)
    raise ValueError(f"unknown mode {mode!r}; expected one of {_MODES}")


def main(argv=None):
    import argparse

    p = argparse.ArgumentParser(
        prog="urdf_approx_geom",
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
    args = p.parse_args(argv)

    msg = generate(args.mode, args.input, args.output,
                   replace_pairs=args.replace, simplify=bool(args.simplify), config=args.config)
    print(msg)


if __name__ == "__main__":
    main()
