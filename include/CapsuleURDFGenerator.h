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

#ifndef URDFAPPROXGEOM_CAPSULEURDFGENERATOR_H
#define URDFAPPROXGEOM_CAPSULEURDFGENERATOR_H

#include "URDFGenerator.h"
#include "irmv/third_party/json.hpp"

/// Wu2018 cross-section capsule approximation. Per link: make the mesh
/// watertight, slice it into cross-sections perpendicular to its PCA axis,
/// fit COA-minimized circles per section (Lloyd), link adjacent-section circles
/// into capsules, grow to cover, and write the capsules as NATIVE URDF
/// primitives (one <cylinder> + two <sphere> end-caps per capsule) + a JSON
/// sidecar of capsule params. Cylinder+sphere is the URDF-native form of a
/// capsule (loadable in ROS / pybullet / MuJoCo).
class CapsuleURDFGenerator : public URDFGenerator {
public:
    /// @param use_visual  fit the visual mesh (true) or collision mesh (false).
    explicit CapsuleURDFGenerator(const std::string& capsule_config_path, bool use_visual = true);

    ~CapsuleURDFGenerator() override;

    irmv_core::bot_common::ErrorInfo run(const std::string& urdf_path, const std::string& output_path,
                                         const std::vector<std::pair<std::string, std::string>>& replace_pairs) override;

    /// One URDF load + one mesh load + one Manifold watertight pass per link,
    /// then fits every preset on the cached link meshes and writes one
    /// (URDF, JSON) pair per preset. @p presets is (output_path, config_path).
    /// Parallelizes the per-link mesh load across links (std::async, like the
    /// sphere generator).
    irmv_core::bot_common::ErrorInfo runMulti(const std::string& urdf_path,
                                              const std::vector<std::pair<std::string, std::string>>& presets,
                                              const std::vector<std::pair<std::string, std::string>>& replace_pairs);

private:
    void loadConfigFrom(const std::string& path);

    /// Build fit options from current members, fit capsules on the link-frame
    /// watertight mesh (Vlf, F), grow to cover the original link-frame mesh
    /// (Vorig_lf), and emit URDF collision primitives + JSON entries into @p
    /// link_json. Shared by run() and runMulti() so preset fits stay identical.
    void fitAndEmit(const urdf::LinkSharedPtr& link,
                    const Eigen::MatrixXd& Vlf, const Eigen::MatrixXi& F,
                    const Eigen::MatrixXd& Vorig_lf, nlohmann::json& link_json);

    std::string config_path_;
    bool use_visual_ = true;
    int n_sections_ = 4;
    double coa_threshold_ = 0.005;
    int max_circles_per_section_ = 1;
    int max_capsules_ = 12;
    double max_radius_bin_ratio_ = 1.45;
    double max_capv_aabb_ratio_ = -1.0;
    double min_split_volume_improvement_ = 0.005;
    bool adaptive_circle_count_ = false;
    int union_volume_samples_per_axis_ = 32;
};

#endif  // URDFAPPROXGEOM_CAPSULEURDFGENERATOR_H
