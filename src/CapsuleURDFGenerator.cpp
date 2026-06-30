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

#include <urdf_model/model.h>
#include <yaml-cpp/yaml.h>
#include "irmv/third_party/json.hpp"

#include <Eigen/Geometry>
#include <fstream>
#include <string>
#include <vector>

#include "irmv/bot_common/log/singleton_logger.h"

namespace {
// Resolve the sphere-tree config path relative to the capsule config dir, and
// the simplify flag, so the parent (SphereTreeURDFGenerator) ctor can read
// Method/SimplifyRatio from the real sphere-tree config.
std::string resolveSphereTreeConfig(const std::string& capsule_config_path) {
    std::string rel = "../sphereTree/sphereTreeConfig.yml";
    try {
        YAML::Node doc = YAML::LoadFile(capsule_config_path);
        rel = doc["SphereTreeConfigPath"].as<std::string>(rel);
    } catch (...) {
    }
    namespace fs = std::filesystem;
    return fs::weakly_canonical(fs::path(capsule_config_path).parent_path() / rel).string();
}
bool resolveSimplify(const std::string& capsule_config_path) {
    try {
        return YAML::LoadFile(capsule_config_path)["Simplify"].as<bool>(true);
    } catch (...) {
        return true;
    }
}
}  // namespace

CapsuleURDFGenerator::CapsuleURDFGenerator(const std::string& capsule_config_path)
    : SphereTreeURDFGenerator(resolveSphereTreeConfig(capsule_config_path),
                              resolveSimplify(capsule_config_path)) {
    config_path_ = capsule_config_path;
    try {
        YAML::Node doc = YAML::LoadFile(capsule_config_path);
        max_capsules_ = doc["MaxCapsulesPerLink"].as<int>(3);
        cluster_gap_ = doc["ClusterGap"].as<double>(0.02);
        min_cluster_size_ = doc["MinClusterSize"].as<int>(2);
        fat_split_ratio_ = doc["FatSplitRatio"].as<double>(0.6);
    } catch (...) {
        // keep defaults
    }
}

CapsuleURDFGenerator::~CapsuleURDFGenerator() = default;

irmv_core::bot_common::ErrorInfo
CapsuleURDFGenerator::run(const std::string& urdf_path, const std::string& output_path,
                          const std::vector<std::pair<std::string, std::string>>& replace_pairs) {
    // 1. In-memory sphere build (inherited): populates m_model with per-link
    //    sub-sphere collisions in link frame. No intermediate files.
    auto ret = buildSphereModel(urdf_path, replace_pairs);
    if (!ret.isOk()) {
        IRMV_ERROR("{}", ret.message());
        return ret;
    }

    // 2. Post-process: read the sub-spheres back from m_model, cluster, refit
    //    capsules, and replace each link's collisions with cylinder+sphere.
    nlohmann::json json;
    for (auto& link_pair : m_model->links_) {
        const auto& link_name = link_pair.first;
        auto& link = link_pair.second;

        std::vector<Eigen::Vector3d> centers;
        std::vector<double> radii;
        for (const auto& col : link->collision_array) {
            if (!col || col->geometry->type != urdf::Geometry::SPHERE) continue;
            const auto* sph = dynamic_cast<const urdf::Sphere*>(col->geometry.get());
            centers.emplace_back(col->origin.position.x, col->origin.position.y, col->origin.position.z);
            radii.push_back(sph->radius);
        }
        if (centers.empty()) continue;

        auto fit = urdf_approx_geom::fitCapsulesFromSpheres(
            centers, radii, cluster_gap_, min_cluster_size_, fat_split_ratio_, max_capsules_);

        link->collision_array.clear();
        nlohmann::json cap_arr = nlohmann::json::array();
        for (const auto& cap : fit.capsules) {
            Eigen::Vector3d axis = cap.p1 - cap.p0;
            double len = axis.norm();

            auto cyl_col = std::make_shared<urdf::Collision>();
            auto cyl = std::make_shared<urdf::Cylinder>();
            cyl->radius = cap.radius;
            cyl->length = len;
            cyl_col->geometry = cyl;
            Eigen::Vector3d mid = 0.5 * (cap.p0 + cap.p1);
            cyl_col->origin.position.x = mid.x();
            cyl_col->origin.position.y = mid.y();
            cyl_col->origin.position.z = mid.z();
            if (len > 1e-9) {
                Eigen::Quaterniond q = Eigen::Quaterniond::FromTwoVectors(Eigen::Vector3d::UnitZ(), axis / len);
                cyl_col->origin.rotation.x = q.x();
                cyl_col->origin.rotation.y = q.y();
                cyl_col->origin.rotation.z = q.z();
                cyl_col->origin.rotation.w = q.w();
            } else {
                cyl_col->origin.rotation.clear();
            }
            link->collision_array.push_back(cyl_col);

            for (const Eigen::Vector3d* end : {&cap.p0, &cap.p1}) {
                auto sph_col = std::make_shared<urdf::Collision>();
                auto sph = std::make_shared<urdf::Sphere>();
                sph->radius = cap.radius;
                sph_col->geometry = sph;
                sph_col->origin.position.x = end->x();
                sph_col->origin.position.y = end->y();
                sph_col->origin.position.z = end->z();
                sph_col->origin.rotation.clear();
                link->collision_array.push_back(sph_col);
            }

            nlohmann::json cp;
            cp["p0"] = std::vector<double>{cap.p0.x(), cap.p0.y(), cap.p0.z()};
            cp["p1"] = std::vector<double>{cap.p1.x(), cap.p1.y(), cap.p1.z()};
            cp["radius"] = cap.radius;
            cap_arr.push_back(cp);
        }

        nlohmann::json sph_arr = nlohmann::json::array();
        for (const auto& s : fit.spheres) {
            auto sph_col = std::make_shared<urdf::Collision>();
            auto sph = std::make_shared<urdf::Sphere>();
            sph->radius = s.second;
            sph_col->geometry = sph;
            sph_col->origin.position.x = s.first.x();
            sph_col->origin.position.y = s.first.y();
            sph_col->origin.position.z = s.first.z();
            sph_col->origin.rotation.clear();
            link->collision_array.push_back(sph_col);

            nlohmann::json sp;
            sp["c"] = std::vector<double>{s.first.x(), s.first.y(), s.first.z()};
            sp["r"] = s.second;
            sph_arr.push_back(sp);
        }

        if (!link->collision_array.empty())
            link->collision = link->collision_array[0];

        nlohmann::json body;
        body["capsules"] = cap_arr;
        body["spheres"] = sph_arr;
        json[link_name] = body;
    }

    // 3. Emit capsule URDF + JSON sidecar (overrides the parent's spherized output).
    std::string json_path = output_path;
    replaceWith(json_path, ".urdf", ".json");
    std::ofstream json_file(json_path);
    json_file << json.dump(4);
    json_file.close();

    return writeURDF(output_path, m_model);
}
