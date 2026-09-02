# Third-Party Notices

URDFApproxGeom uses the following third-party components.  The commercial
edition is built without the sphere_tree backend and without the former
ManifoldPlus / CGAL dependencies.

| Component | License | Edition | Purpose |
|---|---|---|---|
| Eigen3 | MPL-2.0 | all | linear algebra |
| yaml-cpp | MIT | all | configuration parsing |
| urdfdom | BSD | all | URDF parsing/writing |
| tinyxml2 | Zlib | all | XML |
| pybind11 | BSD | all (Python bindings) | Python bindings |
| GoogleTest | BSD | test only | tests |
| libigl (trimmed subset) | MPL-2.0 | all | OBJ/STL I/O, volume, decimation |
| nlohmann/json | MIT | all | JSON sidecars |
| trimesh | MIT | Python | DAE conversion / mesh repair |
| numpy | BSD | Python | numerical support |
| quickhull.hpp (vendored, self-contained) | project code | all | replaces CGAL QuickHull |
| sphere_tree + STG + gdiam + qHull | research-only / GPL | research only | multi-sphere tree |

The research edition may include additional notices from `third_party/sphere_tree/LICENSE`.
