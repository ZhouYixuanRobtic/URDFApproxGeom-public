/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


//
// Created by zyx on 24-11-1.
//

#ifndef URDFAPPROXGEOM_SPHERETREEURDFGENERATOR_H
#define URDFAPPROXGEOM_SPHERETREEURDFGENERATOR_H

#include "URDFGenerator.h"
#include "irmv/third_party/json.hpp"

class SphereTreeURDFGenerator : public URDFGenerator {
  public:
    /// @param use_visual  fit the visual mesh (true, ground-truth geometry) or
    ///                   the collision mesh (false). Falls back to collision
    ///                   when the requested source has no mesh on a link.
    SphereTreeURDFGenerator(const std::string& st_config_path, bool simplify = true,
                            bool use_visual = true);

    ~SphereTreeURDFGenerator() override;

  protected:
    bool doSimplify = false;
    double simplify_ratio = 0.01;
    bool use_visual_ = true;
    int type_ = 0;  // SphereTreeMethod::STMethodType, kept opaque in the public header
    std::string config_path_;
    nlohmann::json spheres_json_;  // per-link data, filled by buildSphereModel

    /// In-memory sphere-tree build: loads the URDF, validates the mesh for the
    /// multi-sphere mode and runs sphere tree per mesh link, populates m_model
    /// with sub-sphere collisions, m_biggest_model with the biggest sphere, and
    /// spheres_json_ with the data.
    /// No file writes -- subclasses (CapsuleURDFGenerator) reuse this and emit
    /// their own representation.
    irmv_core::bot_common::ErrorInfo buildSphereModel(
        const std::string& urdf_path,
        const std::vector<std::pair<std::string, std::string>>& replace_pairs);

    /// Single-sphere baseline: one conservative bounding sphere per mesh link.
    /// Fills m_model, m_biggest_model, and spheres_json_ the same way as
    /// buildSphereModel, but each link gets exactly one sphere instead of a tree.
    irmv_core::bot_common::ErrorInfo buildSingleSphereModel(
        const std::string& urdf_path,
        const std::vector<std::pair<std::string, std::string>>& replace_pairs);

  public:
    irmv_core::bot_common::ErrorInfo run(
        const std::string& urdf_path, const std::string& output_path,
        const std::vector<std::pair<std::string, std::string>>& replace_pairs) override;

    /// One mesh load + one tree build, two outputs: the multi-sphere URDF at
    /// `output_path` (same as run()) AND a single-sphere URDF at
    /// `single_output_path` built from m_biggest_model (tree.biggest_sphere).
    /// Saves the compare-all path from running the generator twice.
    irmv_core::bot_common::ErrorInfo runPair(
        const std::string& urdf_path, const std::string& output_path,
        const std::string& single_output_path,
        const std::vector<std::pair<std::string, std::string>>& replace_pairs);
};

#endif  // URDFAPPROXGEOM_SPHERETREEURDFGENERATOR_H
