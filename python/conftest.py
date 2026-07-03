"""Pytest bootstrap for source-tree package plus CMake-built extension."""

from __future__ import annotations

import pathlib
import sys

_HERE = pathlib.Path(__file__).resolve().parent
_REPO = _HERE.parent
_EXT = _REPO / "build" / "python"

for path in (str(_HERE), str(_EXT)):
    if path not in sys.path:
        sys.path.insert(0, path)
