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

/// Wu2018 cross-section capsule approximation. Per link: make the mesh
/// watertight, slice it into cross-sections perpendicular to its PCA axis,
/// fit COA-minimized circles per section (Lloyd), link adjacent-section circles
/// into capsules, grow to cover, and write the capsules as NATIVE URDF
/// primitives (one <cylinder> + two <sphere> end-caps per capsule) + a JSON
/// sidecar of capsule params. Cylinder+sphere is the URDF-native form of a
/// capsule (loadable in ROS / pybullet / MuJoCo).
class CapsuleURDFGenerator : public URDFGenerator {
public:
    explicit CapsuleURDFGenerator(const std::string& capsule_config_path);

    ~CapsuleURDFGenerator() override;

    irmv_core::bot_common::ErrorInfo run(const std::string& urdf_path, const std::string& output_path,
                                         const std::vector<std::pair<std::string, std::string>>& replace_pairs) override;

private:
    std::string config_path_;
    int n_sections_ = 4;
    double coa_threshold_ = 0.005;
    int max_circles_per_section_ = 4;
    int max_capsules_ = 6;
};

#endif  // URDFAPPROXGEOM_CAPSULEURDFGENERATOR_H
