#!/usr/bin/env python3
# Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
# All Rights Reserved.

"""Compatibility wrapper for package visualization helpers."""

from urdf_approx_geom.robot_viewer import bundle, open_in_robot_viewer
from urdf_approx_geom.visualization import visualize_capsules

__all__ = ["bundle", "open_in_robot_viewer", "visualize_capsules"]
