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

/// Emits a per-link covering-capsule representation as a JSON sidecar
/// (`p0`, `p1`, `radius` per capsule, in link frame). The URDF `<collision>`
/// elements are left as the original meshes (urdfdom has no `<capsule>` element).
class CapsuleURDFGenerator : public URDFGenerator {
public:
    explicit CapsuleURDFGenerator(const std::string& capsule_config_path);

    ~CapsuleURDFGenerator() override;

    irmv_core::bot_common::ErrorInfo run(const std::string& urdf_path, const std::string& output_path,
                                         const std::vector<std::pair<std::string, std::string>>& replace_pairs) override;

private:
    std::string config_path_;
    double split_volume_ratio_ = 5.0;
    int max_capsules_ = 2;
};

#endif  // URDFAPPROXGEOM_CAPSULEURDFGENERATOR_H
