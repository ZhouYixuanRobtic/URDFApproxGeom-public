"""Public Python API for URDF approximate collision geometry generation."""

from __future__ import annotations

from ._extension import load_extension

_ext = load_extension()

capsuleized = _ext.capsuleized
convex = _ext.convex
spherized = _ext.spherized

__all__ = ["capsuleized", "convex", "spherized"]
