#!/usr/bin/env python3
"""Shared visualization helpers for generated URDF approximation outputs."""

from __future__ import annotations

import os
import pathlib


def visualize_capsules(urdf: str, caps_json: str, *, png: str = "", mjcf: str = "") -> None:
    """Render capsule overlay or export MJCF for the given URDF + capsule JSON.

    Exactly one of *png* or *mjcf* should be set:
      - mjcf  → calls scripts.make_mjcf.write_capsule_mjcf to produce a MuJoCo XML scene.
      - png   → calls scripts.viz_capsules.render_capsule_overlay to render a PNG.
      - neither → opens an interactive pybullet GUI window.
    """
    if mjcf:
        from scripts.make_mjcf import write_capsule_mjcf

        write_capsule_mjcf(urdf, caps_json, mjcf)
        print(f"wrote {mjcf}")
        return
    from scripts.viz_capsules import render_capsule_overlay

    render_capsule_overlay(urdf, caps_json, png=png)
    if png:
        print(f"rendered {png}")
    else:
        print("opened capsule overlay viewer")


def default_output_path(path: str, suffix: str) -> str:
    """Replace the suffix of *path* with *suffix*."""
    p = pathlib.Path(path)
    return str(p.with_suffix(suffix))
