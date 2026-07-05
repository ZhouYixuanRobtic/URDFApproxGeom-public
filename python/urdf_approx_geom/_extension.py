"""Load the private CMake-built pybind11 extension."""

from __future__ import annotations

import importlib
import sys

from ._paths import source_root


def load_extension():
    # 1. extension shipped inside the package (wheel / installed image staged
    #    the .so next to _extension.py).
    try:
        return importlib.import_module("._urdf_approx_geom", __package__)
    except ImportError:
        pass
    # 2. already on sys.path at top level (dev conftest inserts build/python,
    #    or the container placed the .so in site-packages root).
    try:
        return importlib.import_module("_urdf_approx_geom")
    except ImportError:
        pass
    # 3. dev source tree: <root>/build/python — insert and retry once.
    root = source_root()
    if root is not None:
        build_python = root / "build" / "python"
        if build_python.is_dir() and str(build_python) not in sys.path:
            sys.path.insert(0, str(build_python))
        try:
            return importlib.import_module("_urdf_approx_geom")
        except ImportError:
            pass
    raise ImportError(
        "_urdf_approx_geom extension not found. Build it with "
        "-DCOMPILE_URDFApproxGeom_PYBINDING=ON (it lands in build/python), or "
        "install a wheel that ships it inside the package."
    )
