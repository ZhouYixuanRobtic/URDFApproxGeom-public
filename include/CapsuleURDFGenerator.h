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

#include "SphereTreeURDFGenerator.h"

/// Sphere-based capsule approximation. Inherits the sphere-tree build from
/// SphereTreeURDFGenerator (no duplication): run() calls the parent pipeline to
/// populate m_model with per-link sub-spheres, then post-processes -- clusters
/// the spheres, fits a covering capsule per cluster, and writes the capsules as
/// NATIVE URDF primitives (one <cylinder> + two <sphere> end-caps per capsule,
/// leftover spheres as <sphere>) plus a JSON sidecar of capsule params.
/// Cylinder+sphere is the URDF-native form of a capsule (loadable in ROS /
/// pybullet / MuJoCo).
class CapsuleURDFGenerator : public SphereTreeURDFGenerator {
public:
    explicit CapsuleURDFGenerator(const std::string& capsule_config_path);

    ~CapsuleURDFGenerator() override;

    irmv_core::bot_common::ErrorInfo run(const std::string& urdf_path, const std::string& output_path,
                                         const std::vector<std::pair<std::string, std::string>>& replace_pairs) override;

private:
    double cluster_gap_ = 0.02;
    int min_cluster_size_ = 2;
    double fat_split_ratio_ = 0.6;
    int max_capsules_ = 3;
};

#endif  // URDFAPPROXGEOM_CAPSULEURDFGENERATOR_H
