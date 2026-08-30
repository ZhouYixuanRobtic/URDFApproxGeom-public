#!/usr/bin/env python3
# Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
# All Rights Reserved.

"""Generate the OBJ test fixtures expected by test/test_spheretree.cpp.

The sphere-tree tests look for a small set of public FR3 OBJ meshes.  This
repository ships the equivalent STL files; this script converts them so the
research test suite can run from a clean checkout.

Run from the repository root:

    python3 scripts/prepare_test_assets.py
"""

from __future__ import annotations

import pathlib
import sys

import trimesh

ROOT = pathlib.Path(__file__).resolve().parents[1]

JOBS = [
    (
        ROOT / "resources/fr3/meshes/franka_hand/collision/finger.stl",
        ROOT / "resources/fr3/meshes/franka_hand/collision/collision/finger.obj",
    ),
    (
        ROOT / "resources/fr3/meshes/fr3/collision/link7.stl",
        ROOT / "resources/fr3/meshes/fr3/collision/collision/link7.obj",
    ),
    (
        ROOT / "resources/fr3/meshes/plate/collision/flex_griper_connect.stl",
        ROOT / "resources/fr3/meshes/plate/collision/collision/flex_griper_connect.obj",
    ),
]


def main() -> int:
    for src, dst in JOBS:
        if not src.exists():
            # CI runs this before test_spheretree; fail here so a partial asset
            # checkout surfaces at the point of detection, not as a vague
            # "test asset missing" assertion later.
            print(f"error: missing source {src}", file=sys.stderr)
            return 1
        dst.parent.mkdir(parents=True, exist_ok=True)
        mesh = trimesh.load(src, force="mesh")
        mesh.export(dst)
        print(f"wrote {dst}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
