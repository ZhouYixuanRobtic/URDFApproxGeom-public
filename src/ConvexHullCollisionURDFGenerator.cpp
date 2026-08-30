/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#include "ConvexHullCollisionURDFGenerator.h"
#include "ConvexHullBackend.h"
#include <igl/moments.h>
#include <urdf_model/model.h>
#include <filesystem>
#include <string>
#include <vector>

#include "irmv/bot_common/log/singleton_logger.h"

ConvexHullCollisionURDFGenerator::ConvexHullCollisionURDFGenerator() : URDFGenerator() {
    m_model = std::make_shared<urdf::ModelInterface>();
}

ConvexHullCollisionURDFGenerator::~ConvexHullCollisionURDFGenerator() {
    m_model.reset();
    m_model = nullptr;
}

irmv_core::bot_common::ErrorInfo ConvexHullCollisionURDFGenerator::run(
    const std::string& urdf_path, const std::string& output_path,
    const std::vector<std::pair<std::string, std::string>>& replace_pairs) {
    auto ret = loadURDF(urdf_path, m_model);
    if (!ret.isOk()) {
        IRMV_ERROR("{}", ret.message());
        return ret;
    }
    for (auto& link_pair : m_model->links_) {
        if (link_pair.second->collision_array.size() > 1) {
            return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR,
                    "We only accept one collision mesh"};
        } else {
            auto& collision = link_pair.second->collision;
            if (collision != nullptr) {
                switch (collision->geometry->type) {
                case urdf::Geometry::MESH: {
                    auto* mesh = dynamic_cast<urdf::Mesh*>(collision->geometry.get());
                    std::string filename_raw = mesh->filename;
                    for (const auto& replace_pair : replace_pairs) {
                        replaceWith(filename_raw, replace_pair.first, replace_pair.second);
                    }
                    std::filesystem::path filename = filename_raw;
                    Eigen::MatrixXd V;
                    Eigen::MatrixXi F, N;
                    bool alreadyOBJ = false;
                    ret = loadedIntoIGL(filename, V, F, N, alreadyOBJ);
                    if (!ret.isOk()) {
                        IRMV_ERROR("{}", ret.message());
                        return ret;
                    } else {
                        // Loaders ignore the URDF <scale>; apply it so the
                        // convex hull matches the rendered mesh.
                        V = V * Eigen::Vector3d(mesh->scale.x, mesh->scale.y, mesh->scale.z)
                                     .asDiagonal();
                        Eigen::MatrixXd CH_V;
                        Eigen::MatrixXi CH_F;

                        urdf_approx_geom::computeConvexHull3D(V, CH_V, CH_F);
                        if (CH_V.rows() < 4 || CH_F.rows() == 0) {
                            return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR,
                                    "convex hull failed: input mesh is degenerate or coplanar (" +
                                        filename.filename().string() + ")"};
                        }

                        ret = saveCollisionGeometry(filename, CH_V, CH_F);
                        if (!ret.isOk()) {
                            IRMV_ERROR("{}", ret.message());
                            return ret;
                        } else {
                            // compute inertia and write collision into URDF
                            mesh->filename = filename.string();
                            for (const auto& replace_pair : replace_pairs) {
                                replaceWith(mesh->filename, replace_pair.second,
                                            replace_pair.first);
                            }
                            double volume;
                            Eigen::Vector3d centroid;
                            Eigen::Matrix3d inertia;
                            igl::moments(CH_V, CH_F, volume, centroid, inertia);
                            // igl::moments divides by the signed volume; a
                            // degenerate-but-nonempty hull yields volume <= 0
                            // or NaN and would corrupt the URDF inertial block.
                            if (!(volume > 0.0) || !inertia.allFinite()) {
                                return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR,
                                        "convex hull produced invalid volume for " +
                                            filename.filename().string()};
                            }
                            if (link_pair.second->inertial)
                                inertia *= link_pair.second->inertial->mass;
                            else
                                link_pair.second->inertial = std::make_shared<urdf::Inertial>();
                            auto& inertia_out = link_pair.second->inertial;
                            inertia_out->ixx = inertia(0, 0);
                            inertia_out->ixy = inertia(0, 1);
                            inertia_out->ixz = inertia(0, 2);
                            inertia_out->iyy = inertia(1, 1);
                            inertia_out->iyz = inertia(1, 2);
                            inertia_out->izz = inertia(2, 2);
                        }
                    }
                    break;
                }
                // do nothing for simple geometry primitives
                default:
                    break;
                }
            }
        }
    }
    return writeURDF(output_path, m_model);
}
