# QuickHull replacement

This directory vendors the self-contained QuickHull-style convex hull used by
both research and commercial editions.

- `quickhull.hpp` — Eigen-only incremental 3D convex hull implementation.
- No CGAL, GMP, or sphere_tree dependency.
- Public wrapper: `include/ConvexHullBackend.h` / `src/ConvexHullBackend.cpp`.
