/*
 ************************************************************************\

                              C O P Y R I G H T

   Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
                         All Rights Reserved.

   Licensed under the Creative Commons Attribution-NonCommercial 4.0
   International License (CC BY-NC 4.0).
   You are free to use, copy, modify, and distribute this software and its
   documentation for educational, research, and other non-commercial purposes,
   provided that appropriate credit is given to the original author(s) and
   copyright holder(s).

   For commercial use or licensing inquiries, please contact:
   IRMV lab, Shanghai Jiao Tong University at: https://irmv.sjtu.edu.cn/

                              D I S C L A I M E R

   IN NO EVENT SHALL TRINITY COLLEGE DUBLIN BE LIABLE TO ANY PARTY FOR
   DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING,
   BUT NOT LIMITED TO, LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF TRINITY COLLEGE DUBLIN HAS BEEN ADVISED OF
   THE POSSIBILITY OF SUCH DAMAGES.

   TRINITY COLLEGE DUBLIN DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE. THE SOFTWARE PROVIDED HEREIN IS ON AN "AS IS" BASIS, AND TRINITY
   COLLEGE DUBLIN HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
   ENHANCEMENTS, OR MODIFICATIONS.

   The authors may be contacted at the following e-mail addresses:

           YX.E.Z yixuanzhou@sjtu.edu.cn

   Further information about the IRMV and its projects can be found at the ISG web site :

          https://irmv.sjtu.edu.cn/

 \*************************************************************************

 */

#include "SphereTreeURDFGenerator.h"
#include "sphereTreeWrapper/sphereTreeSpawn.h"
#include "sphereTreeWrapper/sphereTreeOctree.h"
#include "sphereTreeWrapper/sphereTreeHubbard.h"
#include "sphereTreeWrapper/sphereTreeMedial.h"
#include "sphereTreeWrapper/sphereTreeGrid.h"
#include "yaml-cpp/yaml.h"
#include <urdf_model/model.h>
#include <filesystem>
#include <vector>
#include <string>
#include <igl/decimate.h>
#include "irmv/bot_common/log/singleton_logger.h"
#include "irmv/third_party/json.hpp"
#include "ManifoldPlus/Manifold.h"
#include <future>
#include <igl/volume.h>
#include <fstream>

SphereTreeURDFGenerator::SphereTreeURDFGenerator(const std::string &st_config_path, bool simplify) {
    YAML::Node doc = YAML::LoadFile(st_config_path);
    config_path_ = st_config_path;
    type_ = static_cast<SphereTreeMethod::STMethodType>(doc["Method"].as<int>());
    doSimplify = simplify;
    simplify_ratio = doc["SimplifyRatio"].as<double>();
    simplify_ratio = std::max(0.001, std::min(1., simplify_ratio)); //clamp to [0.001, 1]
}

SphereTreeURDFGenerator::~SphereTreeURDFGenerator() {

}

irmv_core::bot_common::ErrorInfo SphereTreeURDFGenerator::run(const std::string &urdf_path, const std::string &output_path,
                                                   const std::vector<std::pair<std::string, std::string>> &replace_pairs) {

    auto ret = loadURDF(urdf_path, m_model);
    if (!ret.isOk()) {
        IRMV_ERROR("{}", ret.message());
        return ret;
    }
    IRMV_INFO("Got {} links to process", m_model->links_.size());
    //do deep copy
    loadURDF(urdf_path, m_biggest_model);

// Inside SphereTreeURDFGenerator::run
    std::vector<std::future<irmv_core::bot_common::ErrorInfo>> futures;
    int link_count = 0;
    //json
    nlohmann::json json;
    for (auto &link_pair: m_model->links_) {
        json[link_pair.first] = nlohmann::json();
        auto& link_json = json[link_pair.first];
        futures.emplace_back(std::async(std::launch::async, [this, &link_pair, &replace_pairs, &link_count, &link_json]() -> irmv_core::bot_common::ErrorInfo {
            if (link_pair.second->collision_array.size() > 1) {
                return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR, "We only accept one collision mesh"};
            } else {
                auto &collision = link_pair.second->collision;

                if (collision != nullptr) {
                    switch (collision->geometry->type) {
                        case urdf::Geometry::MESH: {
                            auto *mesh = dynamic_cast<urdf::Mesh *>(collision->geometry.get());
                            std::string filename_raw = mesh->filename;
                            for (const auto &replace_pair: replace_pairs) {
                                replaceWith(filename_raw, replace_pair.first, replace_pair.second);
                            }
                            std::filesystem::path filename = filename_raw;
                            Eigen::MatrixXd V;
                            MatrixD OUT_V;
                            Eigen::MatrixXi F, N;
                            MatrixI OUT_F;
                            bool alreadyOBJ = false;
                            auto ret = loadedIntoIGL(filename, V, F, N, alreadyOBJ);
                            if (!ret.isOk()) {
                                IRMV_ERROR("{}", ret.message());
                                return ret;
                            } else {
                                //do manifold watertight process
                                IRMV_INFO("-------------------Start Processing Watertight Manifold----------------");
                                auto m_manifold = std::make_unique<Manifold>();
                                m_manifold->ProcessManifold(V, F, 8, &OUT_V, &OUT_F);
                                IRMV_INFO("Got {} faces after watertight manifold processing", OUT_F.rows());
                                IRMV_INFO("-------------------End Processing Watertight Manifold----------------");
                                //compute volume;
                                Eigen::VectorXd vol;
                                Eigen::MatrixXi T(F.rows(), 4);
                                // Assuming the mesh is already tetrahedralized, otherwise you need to tetrahedralize it
                                // Here we just copy F to T for demonstration purposes
                                T.leftCols(3) = OUT_F;
                                T.col(3).setZero();
                                igl::volume(OUT_V, T, vol);
                                Eigen::Vector3d centroid;
                                if (doSimplify) {
                                    IRMV_INFO("-------------------Start Simplify----------------");
                                    Eigen::VectorXi J;
                                    igl::decimate(OUT_V, OUT_F, static_cast<size_t>(simplify_ratio * (double) OUT_F.rows()), V, F, J);
                                    IRMV_INFO("Simplify from {} to {}", OUT_F.rows(), F.rows());
                                    IRMV_INFO("-------------------End Simplify----------------");
                                } else {
                                    V = OUT_V;
                                    F = OUT_F;
                                }

                                for(unsigned i=0; i<3;++i){
                                    centroid(i) = (V.col(i).maxCoeff() + V.col(i).minCoeff()) * 0.5;
                                }

                                if (!ret.isOk()) {
                                    IRMV_ERROR("{}", ret.message());
                                    return ret;
                                } else {
                                    SphereTreeMethod::MySphereTree tree;
                                    //todo: handle construct errors;
                                    IRMV_INFO("-------------------Start Sphere Tree Approximation for {}-th link ----------------", link_count++);
                                    SphereTreeMethod::SphereTreeUniquePtr m_method;
                                    switch (type_) {
                                        case SphereTreeMethod::Grid:
                                            m_method = SphereTreeMethod::SphereTreeMethodGrid::create(config_path_);
                                            break;
                                        case SphereTreeMethod::Hubbard:
                                            m_method = SphereTreeMethod::SphereTreeMethodHubbard::create(config_path_);
                                            break;
                                        case SphereTreeMethod::Medial:
                                            m_method = SphereTreeMethod::SphereTreeMethodMedial::create(config_path_);
                                            break;
                                        case SphereTreeMethod::Octree:
                                            m_method = SphereTreeMethod::SphereTreeMethodOctree::create(config_path_);
                                            break;
                                        case SphereTreeMethod::Spawn:
                                            m_method = SphereTreeMethod::SphereTreeMethodSpawn::create(config_path_);
                                            break;
                                        default:
                                            m_method = SphereTreeMethod::SphereTreeMethodMedial::create(config_path_);
                                    }

                                    m_method->constructTree(V, F, tree);
                                    //convert original rpy into rotation matrix in case of non-zero rpy
                                    Eigen::Matrix3d original_rotation = Eigen::Quaterniond(collision->origin.rotation.w,
                                                                                           collision->origin.rotation.x,
                                                                                           collision->origin.rotation.y,
                                                                                           collision->origin.rotation.z).toRotationMatrix();
                                    Eigen::Vector3d origin_trans(collision->origin.position.x, collision->origin.position.y, collision->origin.position.z);
                                    //do single sphere approximation
                                    auto sphere = std::make_shared<urdf::Sphere>();
                                    sphere->radius = tree.biggest_sphere.R();
                                    //find the corresponding collision in biggest model
                                    long i = std::find(link_pair.second->collision_array.begin(),
                                                      link_pair.second->collision_array.end(), collision) -
                                            link_pair.second->collision_array.begin();

                                    auto biggest_collision = m_biggest_model->links_[link_pair.first]->collision_array[i];
                                    Eigen::Vector3d rotated_vec = original_rotation * (centroid + tree.biggest_sphere.getData().head(3));
                                    biggest_collision->origin.position.x =  origin_trans.x() + rotated_vec.x();
                                    biggest_collision->origin.position.y =  origin_trans.y() + rotated_vec.y();
                                    biggest_collision->origin.position.z =  origin_trans.z() + rotated_vec.z();
                                    biggest_collision->origin.rotation.clear();
                                    biggest_collision->geometry = sphere;
                                    link_json["BiggestSphere"] = std::vector<double>{biggest_collision->origin.position.x,
                                                                              biggest_collision->origin.position.y,
                                                                              biggest_collision->origin.position.z,
                                                                              tree.biggest_sphere.R()};

                                    // do spheres approximation
                                    link_pair.second->collision_array.clear();
                                    link_json["SubSpheres"] = nlohmann::json();
                                    auto& spheres_json = link_json["SubSpheres"];
                                    for (const SphereTreeMethod::Sphere &sub_sphere: tree.sub_spheres) {
                                        auto sphere_collision = std::make_shared<urdf::Collision>();
                                        rotated_vec = original_rotation * (centroid + sub_sphere.getData().head(3));
                                        sphere_collision->origin.position.x =  origin_trans.x() + rotated_vec.x();
                                        sphere_collision->origin.position.y =  origin_trans.y() + rotated_vec.y();
                                        sphere_collision->origin.position.z =  origin_trans.z() + rotated_vec.z();
                                        sphere_collision->origin.rotation.clear();
                                        sphere = std::make_shared<urdf::Sphere>();
                                        sphere->radius = std::abs(sub_sphere.R());
                                        sphere_collision->geometry = sphere;
                                        if (sphere->radius > 0.005){
                                            spheres_json[("r" + std::to_string(i++))] = std::vector<double>{sphere_collision->origin.position.x,
                                                                                                      sphere_collision->origin.position.y,
                                                                                                      sphere_collision->origin.position.z,
                                                                                                      sphere->radius};
                                            link_pair.second->collision_array.emplace_back(sphere_collision);
                                        }
                                    }
                                    link_pair.second->collision = link_pair.second->collision_array[0];
                                    IRMV_INFO("-------------------End Sphere Tree Approximation----------------");
                                }
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
            return {irmv_core::bot_common::ErrorCode::OK, ""};
        }));
    }

    for (auto &&future : futures) {
        auto fut_ret = future.get();
        if (!fut_ret.isOk()) {
            return fut_ret;
        }
    }
    //write to json file.
    std::string json_output_path = output_path;
    replaceWith(json_output_path, ".urdf", ".json");
    std::ofstream json_file(json_output_path);
    json_file << json.dump(4);
    json_file.close();
    //change xxx.urdf to xxx_spherized.urdf
    std::string biggest_output_path = output_path;
    //replace with new name
    replaceWith(biggest_output_path, ".urdf", "_1.urdf");
    writeURDF(biggest_output_path, m_biggest_model);

    return writeURDF(output_path, m_model);
}