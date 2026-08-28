/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#include <gtest/gtest.h>
#include <cstdio>
#include <ctime>  // clock_t, clock, CLOCKS_PER_SEC
#include <filesystem>
#include "irmv/bot_common/log/singleton_logger.h"
#include "sphereTreeWrapper/sphereTreeBase.h"
#include "sphereTreeWrapper/sphereTreeGrid.h"
#include "sphereTreeWrapper/sphereTreeHubbard.h"
#include "sphereTreeWrapper/sphereTreeMedial.h"
#include "sphereTreeWrapper/sphereTreeOctree.h"
#include "sphereTreeWrapper/sphereTreeSpawn.h"

class SphereTreeTest : public testing::Test {
  protected:
    void SetUp() override {}

    void TearDown() override {}

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
    std::string resourcePath = URDFApproxGeom_RESOURCE_PATH;
    std::string configPath = URDFApproxGeom_CONFIG_PATH;
    SphereTreeMethod::SphereTreeUniquePtr m_method;
};

TEST_F(SphereTreeTest, MedialTest) {
    const std::string test_obj = publicFr3Obj();
    ASSERT_FALSE(test_obj.empty()) << "FR3 public OBJ test asset is missing";
    m_method = SphereTreeMethod::SphereTreeMethodMedial::create(configPath +
                                                                "/sphereTree/sphereTreeConfig.yml");
    SphereTreeMethod::MySphereTree tree;
    auto ret = m_method->constructTree(test_obj, tree);
    IRMV_INFO("{}", ret.message());
    ASSERT_TRUE(ret.isOk());
}

TEST_F(SphereTreeTest, GridTest) {
    const std::string test_obj = publicFr3Obj();
    ASSERT_FALSE(test_obj.empty()) << "FR3 public OBJ test asset is missing";
    m_method = SphereTreeMethod::SphereTreeMethodGrid::create(configPath +
                                                              "/sphereTree/sphereTreeConfig.yml");
    SphereTreeMethod::MySphereTree tree;
    auto ret = m_method->constructTree(test_obj, tree);
    IRMV_INFO("{}", ret.message());
    ASSERT_TRUE(ret.isOk());
}

TEST_F(SphereTreeTest, SpawnTest) {
    const std::string test_obj = publicFr3Obj();
    ASSERT_FALSE(test_obj.empty()) << "FR3 public OBJ test asset is missing";
    m_method = SphereTreeMethod::SphereTreeMethodSpawn::create(configPath +
                                                               "/sphereTree/sphereTreeConfig.yml");
    SphereTreeMethod::MySphereTree tree;
    auto ret = m_method->constructTree(test_obj, tree);
    IRMV_INFO("{}", ret.message());
    ASSERT_TRUE(ret.isOk());
}

TEST_F(SphereTreeTest, HubbardTest) {
    const std::string test_obj = publicFr3Obj();
    ASSERT_FALSE(test_obj.empty()) << "FR3 public OBJ test asset is missing";
    m_method = SphereTreeMethod::SphereTreeMethodHubbard::create(
        configPath + "/sphereTree/sphereTreeConfig.yml");
    SphereTreeMethod::MySphereTree tree;
    auto ret = m_method->constructTree(test_obj, tree);
    IRMV_INFO("{}", ret.message());
    ASSERT_TRUE(ret.isOk());
}

TEST_F(SphereTreeTest, OctreeTest) {
    const std::string test_obj = publicFr3Obj();
    ASSERT_FALSE(test_obj.empty()) << "FR3 public OBJ test asset is missing";
    m_method = SphereTreeMethod::SphereTreeMethodOctree::create(configPath +
                                                                "/sphereTree/sphereTreeConfig.yml");
    SphereTreeMethod::MySphereTree tree;
    auto ret = m_method->constructTree(test_obj, tree);
    IRMV_INFO("{}", ret.message());
    ASSERT_TRUE(ret.isOk());
}

#include <yaml-cpp/yaml.h>

TEST(SphereTreeConfig, SingleSpherePresetHasExplicitFlag) {
    YAML::Node config = YAML::LoadFile(std::string(URDFApproxGeom_CONFIG_PATH) + "/sphereTree/single.yml");
    ASSERT_TRUE(config["SingleSphere"]);
    EXPECT_TRUE(config["SingleSphere"].as<bool>());
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
