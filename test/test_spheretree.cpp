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

#include <cstdio>
#include <ctime>  // clock_t, clock, CLOCKS_PER_SEC
#include <gtest/gtest.h>
#include <filesystem>
#include "irmv/bot_common/log/singleton_logger.h"
#include "sphereTreeWrapper/sphereTreeBase.h"
#include "sphereTreeWrapper/sphereTreeSpawn.h"
#include "sphereTreeWrapper/sphereTreeOctree.h"
#include "sphereTreeWrapper/sphereTreeHubbard.h"
#include "sphereTreeWrapper/sphereTreeMedial.h"
#include "sphereTreeWrapper/sphereTreeGrid.h"

class SphereTreeTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

public:
    // ponytail: returns the first existing public FR3 OBJ; empty string if none found.
    std::string publicFr3Obj(const std::string& name = "finger.obj") const {
        namespace fs = std::filesystem;
        const std::vector<std::string> candidates = {
            resourcePath + "/fr3/meshes/franka_hand/collision/collision/" + name,
            resourcePath + "/fr3/meshes/fr3/collision/collision/link7.obj",
            resourcePath + "/fr3/meshes/plate/collision/collision/flex_griper_connect.obj",
        };
        for (const auto& candidate : candidates) {
            if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
                return fs::absolute(candidate).string();
            }
        }
        return "";
    }

protected:
    std::string resourcePath =  URDFApproxGeom_RESOURCE_PATH;
    std::string configPath = URDFApproxGeom_CONFIG_PATH;
    SphereTreeMethod::SphereTreeUniquePtr m_method;
};

TEST_F(SphereTreeTest, MedialTest) {
    const std::string test_obj = publicFr3Obj();
    ASSERT_FALSE(test_obj.empty()) << "FR3 public OBJ test asset is missing";
    m_method = SphereTreeMethod::SphereTreeMethodMedial::create(configPath + "/sphereTree/sphereTreeConfig.yml");
    SphereTreeMethod::MySphereTree tree;
    auto ret = m_method->constructTree(test_obj, tree);
    IRMV_INFO("{}", ret.message());
    ASSERT_TRUE(ret.isOk());
}

TEST_F(SphereTreeTest, GridTest) {
    const std::string test_obj = publicFr3Obj();
    ASSERT_FALSE(test_obj.empty()) << "FR3 public OBJ test asset is missing";
    m_method = SphereTreeMethod::SphereTreeMethodGrid::create(configPath + "/sphereTree/sphereTreeConfig.yml");
    SphereTreeMethod::MySphereTree tree;
    auto ret = m_method->constructTree(test_obj, tree);
    IRMV_INFO("{}", ret.message());
    ASSERT_TRUE(ret.isOk());
}

TEST_F(SphereTreeTest, SpawnTest) {
    const std::string test_obj = publicFr3Obj();
    ASSERT_FALSE(test_obj.empty()) << "FR3 public OBJ test asset is missing";
    m_method = SphereTreeMethod::SphereTreeMethodSpawn::create(configPath + "/sphereTree/sphereTreeConfig.yml");
    SphereTreeMethod::MySphereTree tree;
    auto ret = m_method->constructTree(test_obj, tree);
    IRMV_INFO("{}", ret.message());
    ASSERT_TRUE(ret.isOk());
}


TEST_F(SphereTreeTest, HubbardTest) {
    const std::string test_obj = publicFr3Obj();
    ASSERT_FALSE(test_obj.empty()) << "FR3 public OBJ test asset is missing";
    m_method = SphereTreeMethod::SphereTreeMethodHubbard::create(configPath + "/sphereTree/sphereTreeConfig.yml");
    SphereTreeMethod::MySphereTree tree;
    auto ret = m_method->constructTree(test_obj, tree);
    IRMV_INFO("{}", ret.message());
    ASSERT_TRUE(ret.isOk());
}

TEST_F(SphereTreeTest, OctreeTest) {
    const std::string test_obj = publicFr3Obj();
    ASSERT_FALSE(test_obj.empty()) << "FR3 public OBJ test asset is missing";
    m_method = SphereTreeMethod::SphereTreeMethodOctree::create(configPath + "/sphereTree/sphereTreeConfig.yml");
    SphereTreeMethod::MySphereTree tree;
    auto ret = m_method->constructTree(test_obj, tree);
    IRMV_INFO("{}", ret.message());
    ASSERT_TRUE(ret.isOk());
}

#include <yaml-cpp/yaml.h>

TEST(SphereTreeConfig, SingleSpherePresetHasExplicitFlag) {
    YAML::Node config = YAML::LoadFile("/workspace/config/sphereTree/single.yml");
    ASSERT_TRUE(config["SingleSphere"]);
    EXPECT_TRUE(config["SingleSphere"].as<bool>());
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}