#!/usr/bin/env python3
"""Compatibility wrapper for package validation helpers."""

from urdf_approx_geom.validation import (
    compare_capsule_files,
    validate_capsule_file,
    validate_capsule_metrics,
)

__all__ = ["compare_capsule_files", "validate_capsule_file", "validate_capsule_metrics"]
