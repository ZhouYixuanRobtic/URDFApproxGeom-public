"""Backward-compatible module wrapper.

New code should import from ``urdf_approx_geom`` or run
``python -m urdf_approx_geom.cli``.
"""

from __future__ import annotations

from urdf_approx_geom import GenerateResult, capsuleized, convex, generate, generate_all, spherized
from urdf_approx_geom.cli import main

__all__ = [
    "GenerateResult",
    "capsuleized",
    "convex",
    "generate",
    "generate_all",
    "main",
    "spherized",
]


if __name__ == "__main__":
    raise SystemExit(main())
