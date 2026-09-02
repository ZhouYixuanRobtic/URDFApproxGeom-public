/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "CapsuleURDFGenerator.h"
#include "ConvexHullCollisionURDFGenerator.h"
#include "SingleSphereURDFGenerator.h"
#ifdef URDFAPPROXGEOM_ENABLE_SPHERE_TREE
#include "SphereTreeURDFGenerator.h"
#endif

namespace py = pybind11;
using replace_pairs_t = std::vector<std::pair<std::string, std::string>>;

// "visual" (default) -> true; "collision" -> false.
static inline bool parse_use_visual(const std::string& mesh_source) {
    return mesh_source != "collision";
}

// Run a generator with the GIL released (mesh loading + tree building can take
// seconds to minutes on large URDFs and must not block the interpreter), and
// surface failures as Python exceptions instead of a bare message string.
template <typename Fn>
static std::string runAndReturnMessage(Fn&& fn) {
    py::gil_scoped_release release;
    auto ret = fn();
    if (!ret.isOk()) {
        throw std::runtime_error(ret.message());
    }
    return ret.message();
}

PYBIND11_MODULE(_urdf_approx_geom, m) {
    m.doc() = "URDF collision-geometry approximator (sphere / convex / capsule)";

    // Capsule -> per-link JSON sidecar (URDF collision left as original mesh).
    // mesh_source: fit the visual mesh ("visual", default) or collision mesh
    // ("collision").
    m.def(
        "capsuleized",
        [](const std::string& input, const std::string& output, const std::string& config,
           replace_pairs_t replace_pairs, const std::string& mesh_source, bool allow_open_mesh) {
            std::string cfg = config.empty() ? std::string(URDFApproxGeom_CONFIG_PATH) +
                                                   "/capsule/capsuleConfig.yml"
                                             : config;
            CapsuleURDFGenerator g(cfg, parse_use_visual(mesh_source), allow_open_mesh);
            return runAndReturnMessage([&] { return g.run(input, output, replace_pairs); });
        },
        py::arg("input"), py::arg("output"), py::arg("config") = std::string(""),
        py::arg("replace_pairs") = replace_pairs_t{},
        py::arg("mesh_source") = std::string("visual"), py::arg("allow_open_mesh") = false);

    // Convex hull -> convex-hull collision mesh URDF.
    m.def(
        "convex",
        [](const std::string& input, const std::string& output, replace_pairs_t replace_pairs) {
            ConvexHullCollisionURDFGenerator g;
            return runAndReturnMessage([&] { return g.run(input, output, replace_pairs); });
        },
        py::arg("input"), py::arg("output"), py::arg("replace_pairs") = replace_pairs_t{});

    // Single-sphere baseline, available in both editions.
    m.def(
        "single_spherized",
        [](const std::string& input, const std::string& output, replace_pairs_t replace_pairs,
           bool simplify, const std::string& mesh_source) {
            SingleSphereURDFGenerator g(simplify, parse_use_visual(mesh_source));
            return runAndReturnMessage([&] { return g.run(input, output, replace_pairs); });
        },
        py::arg("input"), py::arg("output"), py::arg("replace_pairs") = replace_pairs_t{},
        py::arg("simplify") = true, py::arg("mesh_source") = std::string("visual"));

#ifdef URDFAPPROXGEOM_ENABLE_SPHERE_TREE
    // Sphere tree -> spherized collision URDF + JSON sidecar.
    m.def(
        "spherized",
        [](const std::string& input, const std::string& output, const std::string& config,
           replace_pairs_t replace_pairs, bool simplify, const std::string& mesh_source,
           bool allow_open_mesh) {
            std::string cfg = config.empty() ? std::string(URDFApproxGeom_CONFIG_PATH) +
                                                   "/sphereTree/sphereTreeConfig.yml"
                                             : config;
            SphereTreeURDFGenerator g(cfg, simplify, parse_use_visual(mesh_source),
                                      allow_open_mesh);
            return runAndReturnMessage([&] { return g.run(input, output, replace_pairs); });
        },
        py::arg("input"), py::arg("output"), py::arg("config") = std::string(""),
        py::arg("replace_pairs") = replace_pairs_t{}, py::arg("simplify") = true,
        py::arg("mesh_source") = std::string("visual"), py::arg("allow_open_mesh") = false);

    // Sphere tree, one mesh load + one tree build, two outputs: the multi-sphere
    // URDF at `default_output` and a single-sphere URDF (tree.biggest_sphere) at
    // `single_output`. Saves compare-all from running the generator twice.
    m.def(
        "spherized_pair",
        [](const std::string& input, const std::string& default_output,
           const std::string& single_output, const std::string& config,
           replace_pairs_t replace_pairs, bool simplify, const std::string& mesh_source,
           bool allow_open_mesh) {
            std::string cfg = config.empty() ? std::string(URDFApproxGeom_CONFIG_PATH) +
                                                   "/sphereTree/sphereTreeConfig.yml"
                                             : config;
            SphereTreeURDFGenerator g(cfg, simplify, parse_use_visual(mesh_source),
                                      allow_open_mesh);
            return runAndReturnMessage(
                [&] { return g.runPair(input, default_output, single_output, replace_pairs); });
        },
        py::arg("input"), py::arg("default_output"), py::arg("single_output"),
        py::arg("config") = std::string(""), py::arg("replace_pairs") = replace_pairs_t{},
        py::arg("simplify") = true, py::arg("mesh_source") = std::string("visual"),
        py::arg("allow_open_mesh") = false);
#endif
    // Capsule, multi-preset: one mesh load + one validation pass per link, then
    // every (output, config) preset is fit on the cached link meshes. presets is
    // a list of (output_path, config_path) tuples.
    m.def(
        "capsuleized_multi",
        [](const std::string& input,
           const std::vector<std::pair<std::string, std::string>>& presets,
           replace_pairs_t replace_pairs, const std::string& mesh_source, bool allow_open_mesh) {
            std::string default_cfg =
                std::string(URDFApproxGeom_CONFIG_PATH) + "/capsule/capsuleConfig.yml";
            CapsuleURDFGenerator g(default_cfg, parse_use_visual(mesh_source), allow_open_mesh);
            return runAndReturnMessage([&] { return g.runMulti(input, presets, replace_pairs); });
        },
        py::arg("input"), py::arg("presets") = std::vector<std::pair<std::string, std::string>>{},
        py::arg("replace_pairs") = replace_pairs_t{},
        py::arg("mesh_source") = std::string("visual"), py::arg("allow_open_mesh") = false);
}
