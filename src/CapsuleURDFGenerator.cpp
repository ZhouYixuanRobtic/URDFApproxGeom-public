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

#include "CapsuleURDFGenerator.h"
#include "CapsuleFitter.h"
#include "irmv/third_party/json.hpp"
#include "yaml-cpp/yaml.h"

#include <urdf_model/model.h>
#include <Eigen/Geometry>
#include <filesystem>
#include <fstream>

#include "irmv/bot_common/log/singleton_logger.h"

CapsuleURDFGenerator::CapsuleURDFGenerator(const std::string& capsule_config_path)
    : URDFGenerator() {
    m_model = std::make_shared<urdf::ModelInterface>();
    config_path_ = capsule_config_path;
    try {
        YAML::Node doc = YAML::LoadFile(capsule_config_path);
        split_volume_ratio_ = doc["SplitVolumeRatio"].as<double>(5.0);
        max_capsules_ = doc["MaxCapsulesPerLink"].as<int>(2);
    } catch (...) {
        // Missing/unreadable config -> keep defaults.
    }
}

CapsuleURDFGenerator::~CapsuleURDFGenerator() {
    m_model.reset();
}

irmv_core::bot_common::ErrorInfo
CapsuleURDFGenerator::run(const std::string& urdf_path, const std::string& output_path,
                          const std::vector<std::pair<std::string, std::string>>& replace_pairs) {
    auto ret = loadURDF(urdf_path, m_model);
    if (!ret.isOk()) {
        IRMV_ERROR("{}", ret.message());
        return ret;
    }

    nlohmann::json json;
    for (auto& link_pair : m_model->links_) {
        const auto& link = link_pair.second;
        if (link->collision_array.size() > 1) {
            return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR,
                    "We only accept one collision mesh per link"};
        }

        const auto& collision = link->collision;
        if (!collision) {
            continue;  // link without collision: skip
        }

        switch (collision->geometry->type) {
            case urdf::Geometry::MESH: {
                auto* mesh = dynamic_cast<urdf::Mesh*>(collision->geometry.get());
                std::string filename_raw = mesh->filename;
                for (const auto& replace_pair : replace_pairs) {
                    replaceWith(filename_raw, replace_pair.first, replace_pair.second);
                }
                std::filesystem::path filename = filename_raw;

                Eigen::MatrixXd V;
                Eigen::MatrixXi F, N;
                bool alreadyOBJ = false;
                ret = loadedIntoIGL(filename, V, F, N, alreadyOBJ);
                if (!ret.isOk()) {
                    IRMV_ERROR("{}", ret.message());
                    return ret;
                }

                auto caps = urdf_approx_geom::fitCoveringCapsules(V, split_volume_ratio_, max_capsules_);

                // Mesh-local -> link frame via the collision origin transform.
                const auto& o = collision->origin;
                Eigen::Matrix3d R =
                    Eigen::Quaterniond(o.rotation.w, o.rotation.x, o.rotation.y, o.rotation.z)
                        .toRotationMatrix();
                Eigen::Vector3d T(o.position.x, o.position.y, o.position.z);

                nlohmann::json arr = nlohmann::json::array();
                for (const auto& cap : caps) {
                    Eigen::Vector3d p0 = T + R * cap.p0;
                    Eigen::Vector3d p1 = T + R * cap.p1;
                    arr.push_back({{"p0", {p0.x(), p0.y(), p0.z()}},
                                   {"p1", {p1.x(), p1.y(), p1.z()}},
                                   {"radius", cap.radius}});
                }
                json[link_pair.first]["capsules"] = arr;
                break;
            }
            default:
                break;  // primitive geometry: leave untouched, no capsule entry
        }
    }

    // JSON sidecar: <output>.json
    std::string json_path = output_path;
    replaceWith(json_path, ".urdf", ".json");
    std::ofstream json_file(json_path);
    json_file << json.dump(4);
    json_file.close();

    // URDF: collision elements left as the original meshes.
    return writeURDF(output_path, m_model);
}
