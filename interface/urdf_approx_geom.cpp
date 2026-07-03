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

PYBIND11_MODULE(_urdf_approx_geom, m) {
    m.doc() = "URDF collision-geometry approximator (sphere / convex / capsule)";

    // Capsule -> per-link JSON sidecar (URDF collision left as original mesh).
    m.def(
        "capsuleized",
        [](const std::string& input, const std::string& output, const std::string& config,
           replace_pairs_t replace_pairs) {
            std::string cfg = config.empty()
                                  ? std::string(URDFApproxGeom_CONFIG_PATH) + "/capsule/capsuleConfig.yml"
                                  : config;
            CapsuleURDFGenerator g(cfg);
            return g.run(input, output, replace_pairs).message();
        },
        py::arg("input"), py::arg("output"), py::arg("config") = std::string(""),
        py::arg("replace_pairs") = replace_pairs_t{});

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
        [](const std::string& input, const std::string& output, replace_pairs_t replace_pairs,
           bool simplify) {
            SphereTreeURDFGenerator g(std::string(URDFApproxGeom_CONFIG_PATH) +
                                          "/sphereTree/sphereTreeConfig.yml",
                                      simplify);
            return g.run(input, output, replace_pairs).message();
        },
        py::arg("input"), py::arg("output"), py::arg("replace_pairs") = replace_pairs_t{},
        py::arg("simplify") = true);
}
