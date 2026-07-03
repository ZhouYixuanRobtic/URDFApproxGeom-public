"""Public Python API for URDF approximate collision geometry generation."""

from __future__ import annotations

from ._extension import load_extension
from .api import GenerateResult, generate, generate_all


def _extension_function(name: str):
    def call(*args, **kwargs):
        return getattr(load_extension(), name)(*args, **kwargs)

    call.__name__ = name
    return call


capsuleized = _extension_function("capsuleized")
convex = _extension_function("convex")
spherized = _extension_function("spherized")

__all__ = [
    "GenerateResult",
    "capsuleized",
    "convex",
    "generate",
    "generate_all",
    "spherized",
]
