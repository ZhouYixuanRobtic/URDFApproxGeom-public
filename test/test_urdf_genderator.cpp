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
        spherized_generator = std::make_shared<SphereTreeURDFGenerator>(
            configPath + "/sphereTree/sphereTreeConfig.yml", true);
    }

    void TearDown() override {}

  public:
  protected:
    std::string resourcePath = URDFApproxGeom_RESOURCE_PATH;
    std::string configPath = URDFApproxGeom_CONFIG_PATH;
    std::shared_ptr<ConvexHullCollisionURDFGenerator> convex_generator;
    std::shared_ptr<SphereTreeURDFGenerator> spherized_generator;
};

TEST_F(URDFGeneratorTest, CVXTest) {
    auto ret = convex_generator->run(resourcePath + "/fr3/urdf/fr3.urdf",
                                     resourcePath + "/fr3/urdf/fr3_convex.urdf", {});

    IRMV_INFO("{}", ret.message());
}

TEST_F(URDFGeneratorTest, STTest) {
    auto ret = spherized_generator->run(resourcePath + "/fr3/urdf/fr3_convex.urdf",
                                        resourcePath + "/fr3/urdf/fr3_spherized.urdf", {});

    IRMV_INFO("{}", ret.message());
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
