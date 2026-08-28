# Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
# All Rights Reserved.

"""Public Python API for URDF approximate collision geometry generation."""

from __future__ import annotations

from ._extension import load_extension
from .api import (
    GenerateResult,
    generate,
    generate_all,
    generate_capsule_multi,
    generate_sphere_pair,
)


def _extension_function(name: str):
    def call(*args, **kwargs):
        return getattr(load_extension(), name)(*args, **kwargs)

    call.__name__ = name
    return call


capsuleized = _extension_function("capsuleized")
convex = _extension_function("convex")
single_spherized = _extension_function("single_spherized")
spherized = _extension_function("spherized")

__all__ = [
    "GenerateResult",
    "capsuleized",
    "convex",
    "generate",
    "generate_all",
    "generate_capsule_multi",
    "generate_sphere_pair",
    "single_spherized",
    "spherized",
]
