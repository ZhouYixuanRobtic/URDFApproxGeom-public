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
#include <filesystem>
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
    //    sub-sphere collisions in link frame. The spheres DECOMPOSE each link.
    auto ret = buildSphereModel(urdf_path, replace_pairs);
    if (!ret.isOk()) {
        IRMV_ERROR("{}", ret.message());
        return ret;
    }

    // 2. Load the ORIGINAL meshes: buildSphereModel replaced collisions with
    //    spheres, so the mesh path is gone from m_model. Capsules are fit TIGHT
    //    to the mesh surface per sphere-cluster (sphere radii over-cover ~1.4x,
    //    so fitting to spheres would inherit that fatness).
    urdf::ModelInterfaceSharedPtr mesh_model;
    auto mret = loadURDF(urdf_path, mesh_model);
    if (!mret.isOk()) {
        IRMV_ERROR("{}", mret.message());
        return mret;
    }

    // Emit one capsule = <cylinder> + two <sphere> end-caps.
    auto emit_capsule = [&](urdf::LinkSharedPtr& link, const urdf_approx_geom::Capsule& cap,
                            nlohmann::json& cap_arr) {
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
    };

    // 3. Per link: read spheres (decomposition) + mesh vertices (tight fit),
    //    fit capsules, replace collisions.
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

        auto mit = mesh_model->links_.find(link_name);
        if (centers.empty() || mit == mesh_model->links_.end() || !mit->second->collision ||
            mit->second->collision->geometry->type != urdf::Geometry::MESH) {
            continue;
        }

        auto* mesh = dynamic_cast<urdf::Mesh*>(mit->second->collision->geometry.get());
        std::string filename_raw = mesh->filename;
        for (const auto& rp : replace_pairs) replaceWith(filename_raw, rp.first, rp.second);
        Eigen::MatrixXd V;
        Eigen::MatrixXi F, N;
        bool alreadyOBJ = false;
        auto lret = loadedIntoIGL(std::filesystem::path(filename_raw), V, F, N, alreadyOBJ);
        if (!lret.isOk() || V.rows() == 0) {
            IRMV_ERROR("{}", lret.message());
            continue;
        }

        // Mesh-local -> link frame via the collision origin (same transform as
        // the sphere build uses).
        const auto& o = mit->second->collision->origin;
        Eigen::Matrix3d R = Eigen::Quaterniond(o.rotation.w, o.rotation.x, o.rotation.y,
                                                o.rotation.z).toRotationMatrix();
        Eigen::Vector3d T(o.position.x, o.position.y, o.position.z);
        Eigen::MatrixXd Vlf(V.rows(), 3);
        for (int i = 0; i < V.rows(); ++i)
            Vlf.row(i) = (T + R * V.row(i).transpose()).transpose();

        auto caps = urdf_approx_geom::fitCapsulesFromMesh(
            Vlf, centers, radii, cluster_gap_, min_cluster_size_, fat_split_ratio_, max_capsules_);

        link->collision_array.clear();
        nlohmann::json cap_arr = nlohmann::json::array();
        for (const auto& cap : caps) emit_capsule(link, cap, cap_arr);
        if (!link->collision_array.empty()) link->collision = link->collision_array[0];

        json[link_name]["capsules"] = cap_arr;
    }

    // 4. Emit capsule URDF + JSON sidecar.
    std::string json_path = output_path;
    replaceWith(json_path, ".urdf", ".json");
    std::ofstream json_file(json_path);
    json_file << json.dump(4);
    json_file.close();

    return writeURDF(output_path, m_model);
}
