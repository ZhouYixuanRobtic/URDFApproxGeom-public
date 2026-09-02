/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#include <gtest/gtest.h>
#include <cstdio>
#include <ctime>  // clock_t, clock, CLOCKS_PER_SEC
#include "ConvexHullCollisionURDFGenerator.h"
#include "SphereTreeURDFGenerator.h"
#include "irmv/bot_common/log/singleton_logger.h"

class URDFGeneratorTest : public testing::Test {
  protected:
    void SetUp() override {
        convex_generator = std::make_shared<ConvexHullCollisionURDFGenerator>();
        // use_visual=false: the FR3 visuals are .dae, which the C++ loader
        // rejects (the Python CLI converts them via mesh_prep); the collision
        // meshes are OBJ/STL.
        spherized_generator = std::make_shared<SphereTreeURDFGenerator>(
            configPath + "/sphereTree/sphereTreeConfig.yml", true, false);
    }

    void TearDown() override {}

  protected:
    std::string resourcePath = URDFApproxGeom_RESOURCE_PATH;
    std::string configPath = URDFApproxGeom_CONFIG_PATH;
    std::shared_ptr<ConvexHullCollisionURDFGenerator> convex_generator;
    std::shared_ptr<SphereTreeURDFGenerator> spherized_generator;
};

TEST_F(URDFGeneratorTest, CVXTest) {
    // Outputs go to the temp dir: writing into resources/ would overwrite
    // tracked files and fail on read-only checkouts.
    auto out = ::testing::TempDir() + "fr3_convex_test.urdf";
    auto ret = convex_generator->run(resourcePath + "/fr3/urdf/fr3.urdf", out, {});

    ASSERT_TRUE(ret.isOk()) << ret.message();
}

TEST_F(URDFGeneratorTest, STTest) {
    auto out = ::testing::TempDir() + "fr3_spherized_test.urdf";
    auto ret = spherized_generator->run(resourcePath + "/fr3/urdf/fr3.urdf", out, {});

    ASSERT_TRUE(ret.isOk()) << ret.message();
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
