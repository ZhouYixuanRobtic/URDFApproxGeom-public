/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#ifndef URDFAPPROXGEOM_URDFGNEREATOR_H
#define URDFAPPROXGEOM_URDFGNEREATOR_H

#include <urdf_model/model.h>
#include <urdf_world/types.h>
#include <Eigen/Dense>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include "irmv/bot_common/state/error_code.h"

enum class MeshMode {
    Convex,
    Capsule,
    SphereSingle,
    SphereTree,
};

class URDFGenerator {
  public:
    URDFGenerator() = default;

    virtual ~URDFGenerator() = default;

  protected:
    urdf::ModelInterfaceSharedPtr m_model;
    urdf::ModelInterfaceSharedPtr m_biggest_model;

  public:
    virtual irmv_core::bot_common::ErrorInfo run(
        const std::string& urdf_path, const std::string& output_path,
        const std::vector<std::pair<std::string, std::string>>& replace_pairs) = 0;

  protected:
    irmv_core::bot_common::ErrorInfo loadURDF(const std::string& urdf_path,
                                              urdf::ModelInterfaceSharedPtr& robot_model);

    irmv_core::bot_common::ErrorInfo writeURDF(const std::string& output_path,
                                               const urdf::ModelInterfaceSharedPtr& robot_mode);

    std::string toLowerCase(const std::string& str);

    irmv_core::bot_common::ErrorInfo loadedIntoIGL(const std::filesystem::path& file_path,
                                                   Eigen::MatrixXd& V, Eigen::MatrixXi& F,
                                                   Eigen::MatrixXi& N, bool& alreadyOBJ);

    /// Clean a mesh (merge duplicate vertices, remove degenerate faces) and,
    /// for modes that require a closed surface, reject meshes with boundary or
    /// non-manifold edges. Errors include actionable context for the caller.
    irmv_core::bot_common::ErrorInfo validateMeshForMode(
        const Eigen::MatrixXd& V, const Eigen::MatrixXi& F, MeshMode mode,
        Eigen::MatrixXd& outV, Eigen::MatrixXi& outF, const std::string& link_name,
        const std::string& file_path) const;

    irmv_core::bot_common::ErrorInfo saveCollisionGeometry(std::filesystem::path& filename,
                                                           const Eigen::MatrixXd& V,
                                                           const Eigen::MatrixXi& F);

    static bool replaceWith(std::string& src, const std::string& original, const std::string& now);

    /// Resolved mesh fit target for a link: filename (replace_pairs applied) +
    /// link-frame transform from the chosen element's origin. `use_visual` picks
    /// the visual mesh (ground-truth geometry); falls back to collision when the
    /// requested source has no mesh. `found=false` when neither has a mesh.
    struct MeshSource {
        std::filesystem::path filename;
        Eigen::Vector3d translation{Eigen::Vector3d::Zero()};
        Eigen::Quaterniond rotation{Eigen::Quaterniond::Identity()};
        // URDF <mesh scale="x y z">: loaders ignore it, so callers must scale
        // the loaded vertices for the collision geometry to match the mesh.
        Eigen::Vector3d scale{Eigen::Vector3d::Ones()};
        bool found = false;
    };

    /// Apply a mesh source's URDF scale to loaded vertices (in place).
    void applyMeshScale(Eigen::MatrixXd& V, const MeshSource& src) const;
    bool resolveMeshSource(const urdf::LinkSharedPtr& link, bool use_visual,
                           const std::vector<std::pair<std::string, std::string>>& replace_pairs,
                           MeshSource& out);
};

#endif  // URDFAPPROXGEOM_URDFGNEREATOR_H
