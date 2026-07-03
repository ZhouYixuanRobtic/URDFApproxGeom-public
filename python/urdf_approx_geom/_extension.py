"""Load the private CMake-built pybind11 extension."""

from __future__ import annotations

import importlib
import pathlib
import sys


def _repo_root() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[2]


def load_extension():
    try:
        return importlib.import_module("_urdf_approx_geom")
    except ImportError:
        build_python = _repo_root() / "build" / "python"
        if build_python.is_dir() and str(build_python) not in sys.path:
            sys.path.insert(0, str(build_python))
        return importlib.import_module("_urdf_approx_geom")
