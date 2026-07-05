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
#include "CapsuleCrossSection.h"

#include <urdf_model/model.h>
#include <ManifoldPlus/Manifold.h>
#include <yaml-cpp/yaml.h>
#include "irmv/third_party/json.hpp"

#include <Eigen/Geometry>
#include <filesystem>
#include <fstream>
#include <future>
#include <string>
#include <vector>

#include "irmv/bot_common/log/singleton_logger.h"

CapsuleURDFGenerator::CapsuleURDFGenerator(const std::string& capsule_config_path, bool use_visual) : URDFGenerator() {
    m_model = std::make_shared<urdf::ModelInterface>();
    config_path_ = capsule_config_path;
    use_visual_ = use_visual;
    loadConfigFrom(capsule_config_path);
}

CapsuleURDFGenerator::~CapsuleURDFGenerator() = default;

void CapsuleURDFGenerator::loadConfigFrom(const std::string& path) {
    config_path_ = path;
    try {
        YAML::Node doc = YAML::LoadFile(path);
        n_sections_ = doc["NSections"].as<int>(n_sections_);
        coa_threshold_ = doc["CoaThreshold"].as<double>(coa_threshold_);
        max_circles_per_section_ = doc["MaxCirclesPerSection"].as<int>(max_circles_per_section_);
        max_capsules_ = doc["MaxCapsulesPerLink"].as<int>(max_capsules_);
        max_radius_bin_ratio_ = doc["MaxRadiusBinRatio"].as<double>(max_radius_bin_ratio_);
        max_capv_aabb_ratio_ = doc["MaxCapVAabbRatio"].as<double>(max_capv_aabb_ratio_);
        min_split_volume_improvement_ = doc["MinSplitVolumeImprovement"].as<double>(min_split_volume_improvement_);
        adaptive_circle_count_ = doc["AdaptiveCircleCount"].as<bool>(adaptive_circle_count_);
        union_volume_samples_per_axis_ =
            doc["UnionVolumeSamplesPerAxis"].as<int>(union_volume_samples_per_axis_);
    } catch (...) {
        // keep defaults
    }
}

namespace {
// Emit one capsule = <cylinder> + two <sphere> end-caps (link frame).
// A degenerate (L~0) capsule is emitted as a single <sphere>.
void emitCapsuleInto(const urdf::LinkSharedPtr& link, const urdf_approx_geom::Capsule& cap,
                     nlohmann::json& cap_arr) {
    Eigen::Vector3d axis = cap.p1 - cap.p0;
    double len = axis.norm();
    if (len < 1e-9) {
        auto sph_col = std::make_shared<urdf::Collision>();
        auto sph = std::make_shared<urdf::Sphere>();
        sph->radius = cap.radius;
        sph_col->geometry = sph;
        sph_col->origin.position.x = cap.p0.x();
        sph_col->origin.position.y = cap.p0.y();
        sph_col->origin.position.z = cap.p0.z();
        sph_col->origin.rotation.clear();
        link->collision_array.push_back(sph_col);
        nlohmann::json cp;
        cp["p0"] = std::vector<double>{cap.p0.x(), cap.p0.y(), cap.p0.z()};
        cp["p1"] = cp["p0"];
        cp["radius"] = cap.radius;
        cap_arr.push_back(cp);
        return;
    }
    auto cyl_col = std::make_shared<urdf::Collision>();
    auto cyl = std::make_shared<urdf::Cylinder>();
    cyl->radius = cap.radius;
    cyl->length = len;
    cyl_col->geometry = cyl;
    Eigen::Vector3d mid = 0.5 * (cap.p0 + cap.p1);
    cyl_col->origin.position.x = mid.x();
    cyl_col->origin.position.y = mid.y();
    cyl_col->origin.position.z = mid.z();
    Eigen::Quaterniond q = Eigen::Quaterniond::FromTwoVectors(Eigen::Vector3d::UnitZ(), axis / len);
    cyl_col->origin.rotation.x = q.x();
    cyl_col->origin.rotation.y = q.y();
    cyl_col->origin.rotation.z = q.z();
    cyl_col->origin.rotation.w = q.w();
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
}  // namespace

void CapsuleURDFGenerator::fitAndEmit(const urdf::LinkSharedPtr& link,
                                      const Eigen::MatrixXd& Vlf, const Eigen::MatrixXi& F,
                                      const Eigen::MatrixXd& Vorig_lf, nlohmann::json& link_json) {
    urdf_approx_geom::CapsuleFitOptions fit_options;
    fit_options.n_sections = n_sections_;
    fit_options.coa_threshold = coa_threshold_;
    fit_options.max_circles_per_section = max_circles_per_section_;
    fit_options.max_capsules = max_capsules_;
    fit_options.max_radius_bin_ratio = max_radius_bin_ratio_;
    fit_options.max_capv_aabb_ratio = max_capv_aabb_ratio_;
    fit_options.min_split_volume_improvement = min_split_volume_improvement_;
    fit_options.adaptive_circle_count = adaptive_circle_count_;
    fit_options.union_volume_samples_per_axis = union_volume_samples_per_axis_;

    auto caps = urdf_approx_geom::fitCapsulesByCrossSection(Vlf, F, fit_options);
    // The fit ran on the watertight (Manifold) mesh, which may differ from
    // the original; grow again against the ORIGINAL mesh vertices so the
    // real collision surface is fully covered.
    urdf_approx_geom::growCapsulesToCover(caps, Vorig_lf);

    link->collision_array.clear();
    nlohmann::json cap_arr = nlohmann::json::array();
    for (const auto& cap : caps) emitCapsuleInto(link, cap, cap_arr);
    if (!link->collision_array.empty()) link->collision = link->collision_array[0];
    link_json["capsules"] = cap_arr;
}

irmv_core::bot_common::ErrorInfo
CapsuleURDFGenerator::run(const std::string& urdf_path, const std::string& output_path,
                          const std::vector<std::pair<std::string, std::string>>& replace_pairs) {
    auto ret = loadURDF(urdf_path, m_model);
    if (!ret.isOk()) {
        IRMV_ERROR("{}", ret.message());
        return ret;
    }
    IRMV_INFO("Got {} links to process", m_model->links_.size());

    nlohmann::json json;
    for (auto& link_pair : m_model->links_) {
        const auto& link_name = link_pair.first;
        auto& link = link_pair.second;
        if (link->collision_array.size() > 1) {
            return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR,
                    std::string("only one collision mesh per link accepted (") + link_name + ")"};
        }
        MeshSource src;
        if (!resolveMeshSource(link, use_visual_, replace_pairs, src)) continue;

        Eigen::MatrixXd V;
        MatrixD OUT_V;
        Eigen::MatrixXi F, N;
        MatrixI OUT_F;
        bool alreadyOBJ = false;
        auto lret = loadedIntoIGL(src.filename, V, F, N, alreadyOBJ);
        if (!lret.isOk()) {
            IRMV_ERROR("{}", lret.message());
            return lret;
        }

        // Watertight manifold (cross-section slicing needs a closed surface).
        auto m_manifold = std::make_unique<Manifold>();
        m_manifold->ProcessManifold(V, F, 8, &OUT_V, &OUT_F);
        if (OUT_V.rows() < 4 || OUT_F.rows() == 0) continue;

        // Mesh-local -> link frame via the chosen source element's origin.
        Eigen::Matrix3d R = src.rotation.toRotationMatrix();
        const Eigen::Vector3d &T = src.translation;
        Eigen::MatrixXd Vlf(OUT_V.rows(), 3);
        for (int i = 0; i < OUT_V.rows(); ++i)
            Vlf.row(i) = (T + R * OUT_V.row(i).transpose()).transpose();
        Eigen::MatrixXd Vorig_lf(V.rows(), 3);
        for (int i = 0; i < V.rows(); ++i)
            Vorig_lf.row(i) = (T + R * V.row(i).transpose()).transpose();

        fitAndEmit(link, Vlf, OUT_F, Vorig_lf, json[link_name]);
    }

    std::string json_path = output_path;
    replaceWith(json_path, ".urdf", ".json");
    std::ofstream json_file(json_path);
    json_file << json.dump(4);
    json_file.close();

    return writeURDF(output_path, m_model);
}

irmv_core::bot_common::ErrorInfo
CapsuleURDFGenerator::runMulti(const std::string& urdf_path,
                               const std::vector<std::pair<std::string, std::string>>& presets,
                               const std::vector<std::pair<std::string, std::string>>& replace_pairs) {
    if (presets.empty()) {
        return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR, "runMulti: no presets requested"};
    }

    // Phase 1 -- load the URDF once, resolve + load + Manifold each link's mesh
    // once (parallel across links). The expensive part (mesh IO + watertight
    // pass) happens exactly once per link regardless of how many presets run.
    urdf::ModelInterfaceSharedPtr base_model;
    auto ret = loadURDF(urdf_path, base_model);
    if (!ret.isOk()) {
        IRMV_ERROR("{}", ret.message());
        return ret;
    }
    IRMV_INFO("runMulti: {} links, {} presets", base_model->links_.size(), presets.size());

    struct LinkMesh {
        std::string name;
        Eigen::MatrixXd Vlf;
        Eigen::MatrixXd Vorig_lf;
        Eigen::MatrixXi F;
        bool valid = false;
    };

    std::vector<std::pair<std::string, urdf::LinkSharedPtr>> links(base_model->links_.begin(),
                                                                   base_model->links_.end());
    std::vector<LinkMesh> meshes(links.size());

    std::vector<std::future<void>> futures;
    for (size_t idx = 0; idx < links.size(); ++idx) {
        futures.emplace_back(std::async(std::launch::async, [&, idx]() {
            auto &lp = links[idx];
            auto &out = meshes[idx];
            out.name = lp.first;
            const auto &link = lp.second;
            if (link->collision_array.size() > 1) return;  // validated later per-preset
            MeshSource src;
            if (!resolveMeshSource(link, use_visual_, replace_pairs, src)) return;

            Eigen::MatrixXd V;
            MatrixD OUT_V;
            Eigen::MatrixXi F, N;
            MatrixI OUT_F;
            bool alreadyOBJ = false;
            auto lret = loadedIntoIGL(src.filename, V, F, N, alreadyOBJ);
            if (!lret.isOk()) {
                IRMV_ERROR("link '{}': {}", link->name, lret.message());
                return;
            }
            auto m_manifold = std::make_unique<Manifold>();
            m_manifold->ProcessManifold(V, F, 8, &OUT_V, &OUT_F);
            if (OUT_V.rows() < 4 || OUT_F.rows() == 0) return;

            Eigen::Matrix3d R = src.rotation.toRotationMatrix();
            const Eigen::Vector3d &T = src.translation;
            Eigen::MatrixXd Vlf(OUT_V.rows(), 3);
            for (int i = 0; i < OUT_V.rows(); ++i)
                Vlf.row(i) = (T + R * OUT_V.row(i).transpose()).transpose();
            Eigen::MatrixXd Vorig_lf(V.rows(), 3);
            for (int i = 0; i < V.rows(); ++i)
                Vorig_lf.row(i) = (T + R * V.row(i).transpose()).transpose();

            out.Vlf = std::move(Vlf);
            out.Vorig_lf = std::move(Vorig_lf);
            out.F = OUT_F;
            out.valid = true;
        }));
    }
    for (auto &f : futures) f.get();

    // Phase 2 -- per preset: fresh URDF parse, fit each link on its cached mesh.
    for (const auto &preset : presets) {
        const std::string &output_path = preset.first;
        const std::string &config_path = preset.second;
        loadConfigFrom(config_path);

        urdf::ModelInterfaceSharedPtr model;
        loadURDF(urdf_path, model);
        nlohmann::json json;
        for (auto &link_pair : model->links_) {
            const auto &link_name = link_pair.first;
            auto &link = link_pair.second;
            if (link->collision_array.size() > 1) {
                return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR,
                        std::string("only one collision mesh per link accepted (") + link_name + ")"};
            }
            // find cached mesh for this link
            const LinkMesh *lm = nullptr;
            for (const auto &m : meshes) {
                if (m.valid && m.name == link_name) { lm = &m; break; }
            }
            if (!lm) continue;
            fitAndEmit(link, lm->Vlf, lm->F, lm->Vorig_lf, json[link_name]);
        }

        std::string json_path = output_path;
        replaceWith(json_path, ".urdf", ".json");
        std::ofstream json_file(json_path);
        json_file << json.dump(4);
        json_file.close();
        ret = writeURDF(output_path, model);
        if (!ret.isOk()) {
            IRMV_ERROR("{}", ret.message());
            return ret;
        }
        IRMV_INFO("runMulti: wrote {} ({})", output_path, config_path);
    }
    return {irmv_core::bot_common::ErrorCode::OK, ""};
}
