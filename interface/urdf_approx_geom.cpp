/*
 ************************************************************************\

                              C O P Y R I G H T

   Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
                         All Rights Reserved.

   Licensed under the Creative Commons Attribution-NonCommercial 4.0
   International License (CC BY-NC 4.0).

   For commercial use or licensing inquiries, please contact:
   IRMV lab, Shanghai Jiao Tong University at: https://irmv.sjtu.edu.cn/

 \*************************************************************************

 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "CapsuleURDFGenerator.h"
#include "ConvexHullCollisionURDFGenerator.h"
#include "SphereTreeURDFGenerator.h"

namespace py = pybind11;
using replace_pairs_t = std::vector<std::pair<std::string, std::string>>;

// "visual" (default) -> true; "collision" -> false.
static inline bool parse_use_visual(const std::string &mesh_source) {
    return mesh_source != "collision";
}

PYBIND11_MODULE(_urdf_approx_geom, m) {
    m.doc() = "URDF collision-geometry approximator (sphere / convex / capsule)";

    // Capsule -> per-link JSON sidecar (URDF collision left as original mesh).
    // mesh_source: fit the visual mesh ("visual", default) or collision mesh
    // ("collision").
    m.def(
        "capsuleized",
        [](const std::string& input, const std::string& output, const std::string& config,
           replace_pairs_t replace_pairs, const std::string& mesh_source) {
            std::string cfg = config.empty()
                                  ? std::string(URDFApproxGeom_CONFIG_PATH) + "/capsule/capsuleConfig.yml"
                                  : config;
            CapsuleURDFGenerator g(cfg, parse_use_visual(mesh_source));
            return g.run(input, output, replace_pairs).message();
        },
        py::arg("input"), py::arg("output"), py::arg("config") = std::string(""),
        py::arg("replace_pairs") = replace_pairs_t{},
        py::arg("mesh_source") = std::string("visual"));

    // Convex hull -> convex-hull collision mesh URDF.
    m.def(
        "convex",
        [](const std::string& input, const std::string& output, replace_pairs_t replace_pairs) {
            ConvexHullCollisionURDFGenerator g;
            return g.run(input, output, replace_pairs).message();
        },
        py::arg("input"), py::arg("output"), py::arg("replace_pairs") = replace_pairs_t{});

    // Sphere tree -> spherized collision URDF + JSON sidecar.
    m.def(
        "spherized",
        [](const std::string& input, const std::string& output, const std::string& config,
           replace_pairs_t replace_pairs, bool simplify, const std::string& mesh_source) {
            std::string cfg = config.empty()
                                  ? std::string(URDFApproxGeom_CONFIG_PATH) + "/sphereTree/sphereTreeConfig.yml"
                                  : config;
            SphereTreeURDFGenerator g(cfg, simplify, parse_use_visual(mesh_source));
            return g.run(input, output, replace_pairs).message();
        },
        py::arg("input"), py::arg("output"), py::arg("config") = std::string(""),
        py::arg("replace_pairs") = replace_pairs_t{}, py::arg("simplify") = true,
        py::arg("mesh_source") = std::string("visual"));

    // Sphere tree, one mesh load + one tree build, two outputs: the multi-sphere
    // URDF at `default_output` and a single-sphere URDF (tree.biggest_sphere) at
    // `single_output`. Saves compare-all from running the generator twice.
    m.def(
        "spherized_pair",
        [](const std::string& input, const std::string& default_output, const std::string& single_output,
           const std::string& config, replace_pairs_t replace_pairs, bool simplify,
           const std::string& mesh_source) {
            std::string cfg = config.empty()
                                  ? std::string(URDFApproxGeom_CONFIG_PATH) + "/sphereTree/sphereTreeConfig.yml"
                                  : config;
            SphereTreeURDFGenerator g(cfg, simplify, parse_use_visual(mesh_source));
            return g.runPair(input, default_output, single_output, replace_pairs).message();
        },
        py::arg("input"), py::arg("default_output"), py::arg("single_output"),
        py::arg("config") = std::string(""), py::arg("replace_pairs") = replace_pairs_t{},
        py::arg("simplify") = true, py::arg("mesh_source") = std::string("visual"));

    // Capsule, multi-preset: one mesh load + one Manifold pass per link, then
    // every (output, config) preset is fit on the cached link meshes. presets is
    // a list of (output_path, config_path) tuples.
    m.def(
        "capsuleized_multi",
        [](const std::string& input, const std::vector<std::pair<std::string, std::string>>& presets,
           replace_pairs_t replace_pairs, const std::string& mesh_source) {
            std::string default_cfg = std::string(URDFApproxGeom_CONFIG_PATH) + "/capsule/capsuleConfig.yml";
            CapsuleURDFGenerator g(default_cfg, parse_use_visual(mesh_source));
            return g.runMulti(input, presets, replace_pairs).message();
        },
        py::arg("input"), py::arg("presets") = std::vector<std::pair<std::string, std::string>>{},
        py::arg("replace_pairs") = replace_pairs_t{},
        py::arg("mesh_source") = std::string("visual"));
}
