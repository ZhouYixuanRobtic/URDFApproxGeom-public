/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#include <gtest/gtest.h>
#include "ConvexHullBackend.h"
#include <igl/readSTL.h>
#include <igl/writeOBJ.h>
#include <igl/writeSTL.h>
#include <cstdio>
#include <ctime>  // clock_t, clock, CLOCKS_PER_SEC
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include "irmv/bot_common/log/singleton_logger.h"

class SimplifyTest : public testing::Test {
  protected:
    void SetUp() override {}

    void TearDown() override {}

  public:
    static std::string toLowerCase(const std::string& str) {
        std::string result;
        result.reserve(str.size());
        std::transform(str.begin(), str.end(), std::back_inserter(result),
                       [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    static void fetchAllFilesWith(const std::filesystem::path& directory_path,
                                  const std::string& suffix, std::vector<std::string>& results) {
        namespace fs = std::filesystem;
        if (!fs::exists(directory_path) || !fs::is_directory(directory_path)) {
            IRMV_ERROR("Directory does not exist or is not a directory: {}",
                       directory_path.string());
            return;
        }

        const std::string lower_suffix = toLowerCase(suffix);
        try {
            for (const auto& entry : fs::recursive_directory_iterator(directory_path)) {
                if (fs::is_regular_file(entry) &&
                    toLowerCase(entry.path().extension().string()) == lower_suffix) {
                    results.push_back(entry.path());
                }
            }
        } catch (const fs::filesystem_error& e) {
            IRMV_ERROR("Filesystem error: {}", e.what());
        } catch (const std::exception& e) {
            IRMV_ERROR("General error: {}", e.what());
        }
    }

    static bool replaceWith(std::string& src, const std::string& original, const std::string& now) {
        size_t pos = src.find(original);
        if (pos != std::string::npos) {
            src.replace(pos, original.length(), now);
            return true;
        }
        return false;
    }

  protected:
    std::string resourcePath = URDFApproxGeom_RESOURCE_PATH;
};

TEST_F(SimplifyTest, CVHTest) {
    std::string meshPath = resourcePath + "/robots/panda/meshes/visual";
    std::vector<std::string> allSTLFiles;
    fetchAllFilesWith(meshPath, ".stl", allSTLFiles);
    for (const auto& stlFile : allSTLFiles) {

        Eigen::MatrixXd V;
        Eigen::MatrixXi F;
        Eigen::MatrixXi N;

        std::ifstream file_input(stlFile);
        igl::readSTL(file_input, V, F, N);

        Eigen::MatrixXd CH_V;
        Eigen::MatrixXi CH_F;

        urdf_approx_geom::computeConvexHull3D(V, CH_V, CH_F);
        ASSERT_GT(CH_V.rows(), 0) << "Convex hull produced no vertices for " << stlFile;
        ASSERT_GT(CH_F.rows(), 0) << "Convex hull produced no faces for " << stlFile;
        std::string output_path = stlFile;
        replaceWith(output_path, "/visual", "/collision/simple");
        replaceWith(output_path, ".stl", ".obj");
        if (!igl::writeOBJ(output_path, CH_V, CH_F)) {
            IRMV_ERROR("Error: Unable to write OBJ file to {}", stlFile);
        } else
            IRMV_INFO("Success: write watertight OBJ file for {}", stlFile);
    }
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
