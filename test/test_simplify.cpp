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

#include "simplify/Simplify.h"
#include <cstdio>
#include <ctime>  // clock_t, clock, CLOCKS_PER_SEC
#include <gtest/gtest.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <igl/copyleft/cgal/convex_hull.h>
#include <igl/readSTL.h>
#include <igl/writeSTL.h>
#include <igl/writeOBJ.h>
#include <fstream>

class SimplifyTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

public:


    static std::string toLowerCase(const std::string &str) {
        std::string result;
        result.reserve(str.size());
        std::transform(str.begin(), str.end(), std::back_inserter(result),
                       [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    static void fetchAllFilesWith(const std::filesystem::path &directory_path, const std::string &suffix,
                           std::vector<std::string> &results) {
        namespace fs = std::filesystem;
        if (!fs::exists(directory_path) || !fs::is_directory(directory_path)) {
            std::cerr << "Directory does not exist or is not a directory: " << directory_path << std::endl;
            return;
        }

        const std::string lower_suffix = toLowerCase(suffix);
        try {
            for (const auto &entry: fs::recursive_directory_iterator(directory_path)) {
                if (fs::is_regular_file(entry) && toLowerCase(entry.path().extension().string()) == lower_suffix) {
                    results.push_back(entry.path());
                }
            }
        } catch (const fs::filesystem_error &e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "General error: " << e.what() << std::endl;
        }
    }

    static bool replaceWith(std::string &src, const std::string &original, const std::string &now) {
        size_t pos = src.find(original);
        if (pos != std::string::npos) {
            src.replace(pos, original.length(), now);
            return true;
        }
        return false;
    }


    static int simplify(const char *file_path, const char *export_path, float reduceFraction, float agressiveness) {

        printf("Mesh Simplification (C)2014 by Sven Forstmann in 2014, MIT License (%zu-bit)\n", sizeof(size_t) * 8);

        Simplify::load_obj(file_path, true);
        printf("loading obj\n");

        if ((Simplify::triangles.size() < 3) || (Simplify::vertices.size() < 3)) {
            printf("triangles size or vertices size less than 3\n");
            return EXIT_FAILURE;
        }

        int target_count = Simplify::triangles.size() >> 1;

        if (reduceFraction > 1.0) reduceFraction = 1.0; //lossless only
        if (reduceFraction <= 0.0) {
            printf("Ratio must be BETWEEN zero and one.\n");
            return EXIT_FAILURE;
        }
        target_count = round((float) Simplify::triangles.size() * reduceFraction);

        if (target_count < 4) {
            printf("Object will not survive such extreme decimation\n");
            return EXIT_FAILURE;
        }
        clock_t start = clock();
        printf("Input: %zu vertices, %zu triangles (target %d)\n", Simplify::vertices.size(),
               Simplify::triangles.size(), target_count);
        int startSize = Simplify::triangles.size();
        Simplify::simplify_mesh(target_count, agressiveness, true);
        //Simplify::simplify_mesh_lossless( false);
        if (Simplify::triangles.size() >= startSize) {
            printf("Unable to reduce mesh.\n");
            return EXIT_FAILURE;
        }

        Simplify::write_obj(export_path);

        printf("Output: %zu vertices, %zu triangles (%f reduction; %.4f sec)\n", Simplify::vertices.size(),
               Simplify::triangles.size(), (float) Simplify::triangles.size() / (float) startSize,
               ((float) (clock() - start)) / CLOCKS_PER_SEC);
        return EXIT_SUCCESS;
    }

protected:
    std::string resourcePath = RESOURCE_PATH;
};

TEST_F(SimplifyTest, SpTest) {
    std::string meshPath = resourcePath + "/robots/panda/meshes/collision";
    std::vector<std::string> allOBJFiles;
    fetchAllFilesWith(meshPath, ".obj", allOBJFiles);
    for (const auto &objFile: allOBJFiles) {
        std::string output_path = objFile;
        replaceWith(output_path, "/collision", "/collision/simplified");
        simplify(objFile.c_str(), output_path.c_str(), 0.99, 7.);
    }
}

TEST_F(SimplifyTest, CVHTest) {
    std::string meshPath = resourcePath + "/robots/panda/meshes/visual";
    std::vector<std::string> allSTLFiles;
    fetchAllFilesWith(meshPath, ".stl", allSTLFiles);
    for (const auto &stlFile: allSTLFiles) {

        Eigen::MatrixXd V;
        Eigen::MatrixXi F;
        Eigen::MatrixXi N;

        std::ifstream file_input(stlFile);
        igl::readSTL(file_input, V, F, N);


        Eigen::MatrixXd CH_V;
        Eigen::MatrixXi CH_F;

        igl::copyleft::cgal::convex_hull(V, CH_V, CH_F);
        std::string output_path = stlFile;
        replaceWith(output_path, "/visual", "/collision/simple");
        replaceWith(output_path, ".stl", ".obj");
        if (!igl::writeOBJ(output_path, CH_V, CH_F)) {
            std::cerr << "Error: Unable to write OBJ file to " << stlFile << std::endl;
        } else
            std::cout << "Success: write watertight OBJ file for " << stlFile << std::endl;


    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}