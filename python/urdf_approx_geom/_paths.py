"""Path resolution shared across the package.

Centralizes locating the source-tree root and the bundled config tree so the
heuristics live in one place instead of being copied (and silently broken) in
every module. All of these previously used ``Path(__file__).parents[2]``
blindly, which only resolves correctly inside a source checkout and points
somewhere meaningless once the package is installed.
"""

from __future__ import annotations

import os
import pathlib


def source_root() -> pathlib.Path | None:
    """Return the repo source root when running from a checkout, else None.

    Source layout is ``<root>/python/urdf_approx_geom/_paths.py``: walk up two
    parents and check the intermediate dir is named ``python``. Editable
    installs (``pip install -e``) keep this layout, regular installs do not.
    """
    here = pathlib.Path(__file__).resolve().parent  # .../urdf_approx_geom
    python_dir = here.parent
    if python_dir.name != "python":
        return None
    root = python_dir.parent
    return root if root.is_dir() else None


def config_root() -> pathlib.Path:
    """Locate the ``config/`` tree.

    Resolution order:
      1. ``URDF_APPROX_GEOM_CONFIG_DIR`` env var (explicit override).
      2. ``<package_dir>/config`` — shipped as package data (wheel / installed
         image) or staged into the package dir at build time.
      3. ``<source_root>/config`` — running from a checkout.
    """
    env = os.environ.get("URDF_APPROX_GEOM_CONFIG_DIR")
    if env:
        return pathlib.Path(env)

    bundled = pathlib.Path(__file__).resolve().parent / "config"
    if bundled.is_dir():
        return bundled

    root = source_root()
    if root is not None and (root / "config").is_dir():
        return root / "config"

    raise FileNotFoundError(
        "urdf_approx_geom config tree not found. Set URDF_APPROX_GEOM_CONFIG_DIR "
        "or install the package with its config/ data alongside it."
    )


def require_source_root() -> pathlib.Path:
    """Return the source root or raise.

    For features that need a checkout (e.g. running ``scripts/`` helpers) and
    cannot work from an installed wheel.
    """
    root = source_root()
    if root is None:
        raise FileNotFoundError(
            "this command requires a urdf_approx_geom source checkout "
            "(cannot locate repo scripts/ from an installed package)"
        )
    return root
