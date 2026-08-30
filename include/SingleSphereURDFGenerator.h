/*
 * SingleSphereURDFGenerator.h
 *
 * Edition-independent single-sphere generator.  The research edition also
 * provides the full sphere-tree generator; this class exists so the commercial
 * edition can still support `sphere/single` without any sphere_tree code.
 */

#ifndef URDFAPPROXGEOM_SINGLESPHEREURDFGENERATOR_H
#define URDFAPPROXGEOM_SINGLESPHEREURDFGENERATOR_H

#include "URDFGenerator.h"
#include "irmv/third_party/json.hpp"

class SingleSphereURDFGenerator : public URDFGenerator {
  public:
    explicit SingleSphereURDFGenerator(bool simplify = true, bool use_visual = true);
    ~SingleSphereURDFGenerator() override;

    irmv_core::bot_common::ErrorInfo run(
        const std::string& urdf_path, const std::string& output_path,
        const std::vector<std::pair<std::string, std::string>>& replace_pairs) override;

  private:
    bool use_visual_ = true;
    nlohmann::json spheres_json_;
};

#endif  // URDFAPPROXGEOM_SINGLESPHEREURDFGENERATOR_H
