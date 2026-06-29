"""Pytest bootstrap: make the CMake-built extension importable without PYTHONPATH.

The compiled `urdf_approx_geom` extension lives in <repo>/build/python (built
with -DCOMPILE_URDFApproxGeom_PYBINDING=ON). Add it to sys.path so
`pytest python/tests` works from a fresh checkout.
"""
import os
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.dirname(_HERE)
_EXT = os.path.join(_REPO, "build", "python")
if os.path.isdir(_EXT) and _EXT not in sys.path:
    sys.path.insert(0, _EXT)
