/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#ifndef URDFAPPROXGEOM_CONVEXHULLCOLLISIONURDFGENERATOR_H
#define URDFAPPROXGEOM_CONVEXHULLCOLLISIONURDFGENERATOR_H

#include "URDFGenerator.h"

class ConvexHullCollisionURDFGenerator : public URDFGenerator {
  public:
    ConvexHullCollisionURDFGenerator();

    ~ConvexHullCollisionURDFGenerator() override;

  public:
    irmv_core::bot_common::ErrorInfo run(
        const std::string& urdf_path, const std::string& output_path,
        const std::vector<std::pair<std::string, std::string>>& replace_pairs) override;
};

#endif  // URDFAPPROXGEOM_CONVEXHULLCOLLISIONURDFGENERATOR_H
