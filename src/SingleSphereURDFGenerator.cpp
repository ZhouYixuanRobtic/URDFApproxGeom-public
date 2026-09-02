/*
 * SingleSphereURDFGenerator.cpp
 *
 * Conservative single bounding sphere per mesh link.  This implementation is
 * intentionally independent of the sphere_tree backend so it can ship in both
 * research and commercial editions.
 */

#include "SingleSphereURDFGenerator.h"

#include <algorithm>
#include <urdf_model/model.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "irmv/bot_common/log/singleton_logger.h"

SingleSphereURDFGenerator::SingleSphereURDFGenerator(bool /*simplify*/, bool use_visual)
    : URDFGenerator(), use_visual_(use_visual) {
    m_model = std::make_shared<urdf::ModelInterface>();
}

SingleSphereURDFGenerator::~SingleSphereURDFGenerator() = default;

irmv_core::bot_common::ErrorInfo SingleSphereURDFGenerator::run(
    const std::string& urdf_path, const std::string& output_path,
    const std::vector<std::pair<std::string, std::string>>& replace_pairs) {
    auto ret = loadURDF(urdf_path, m_model);
    if (!ret.isOk()) {
        IRMV_ERROR("{}", ret.message());
        return ret;
    }

    nlohmann::json json;
    for (auto& link_pair : m_model->links_) {
        const auto& link_name = link_pair.first;
        auto& link = link_pair.second;
        auto& link_json = json[link_name];

        if (link->collision_array.size() > 1) {
            return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR,
                    "We only accept one collision mesh"};
        }

        MeshSource src;
        if (!resolveMeshSource(link, use_visual_, replace_pairs, src))
            continue;

        Eigen::MatrixXd V;
        Eigen::MatrixXd OUT_V;
        Eigen::MatrixXi F, N;
        Eigen::MatrixXi OUT_F;
        bool alreadyOBJ = false;
        ret = loadedIntoIGL(src.filename, V, F, N, alreadyOBJ);
        if (!ret.isOk()) {
            IRMV_ERROR("{}", ret.message());
            return ret;
        }
        applyMeshScale(V, src);

        auto vret = validateMeshForMode(V, F, MeshMode::SphereSingle, OUT_V, OUT_F, link_name,
                                        src.filename.string());
        if (!vret.isOk()) {
            IRMV_ERROR("{}", vret.message());
            return vret;
        }

        // Single-sphere fitting only needs the vertex set; no mesh simplification
        // is required (and the validated mesh is already cleaned).
        V = OUT_V;

        Eigen::Vector3d center = V.colwise().mean();
        double radius = 0.0;
        for (int i = 0; i < V.rows(); ++i)
            radius = std::max(radius, (V.row(i).transpose() - center).norm());

        Eigen::Matrix3d R = src.rotation.toRotationMatrix();
        const Eigen::Vector3d& T = src.translation;
        Eigen::Vector3d rotated_center = R * center;

        link->collision_array.clear();
        auto sphere_collision = std::make_shared<urdf::Collision>();
        sphere_collision->origin.position.x = T.x() + rotated_center.x();
        sphere_collision->origin.position.y = T.y() + rotated_center.y();
        sphere_collision->origin.position.z = T.z() + rotated_center.z();
        sphere_collision->origin.rotation.clear();
        auto sphere = std::make_shared<urdf::Sphere>();
        sphere->radius = radius;
        sphere_collision->geometry = sphere;
        link->collision_array.emplace_back(sphere_collision);
        link->collision = link->collision_array[0];

        nlohmann::json entry;
        entry["center"] = {sphere_collision->origin.position.x,
                           sphere_collision->origin.position.y,
                           sphere_collision->origin.position.z};
        entry["radius"] = sphere->radius;
        link_json["spheres"] = nlohmann::json::array({entry});
    }

    // Drop links that had no mesh geometry.
    for (auto it = json.begin(); it != json.end();) {
        if (it->is_null() || !it->contains("spheres")) {
            it = json.erase(it);
        } else {
            ++it;
        }
    }
    spheres_json_ = std::move(json);

    // Derive the sidecar from the path's real extension: replaceWith(".urdf")
    // silently no-ops for uppercase/extensionless output paths and would then
    // overwrite the URDF itself.
    std::filesystem::path json_path =
        std::filesystem::path(output_path).replace_extension(".json");
    std::ofstream json_file(json_path);
    json_file << spheres_json_.dump(4);
    if (!json_file.good()) {
        return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR,
                "failed to write JSON sidecar " + json_path.string()};
    }
    json_file.close();

    return writeURDF(output_path, m_model);
}
