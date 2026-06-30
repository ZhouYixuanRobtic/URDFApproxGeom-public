/*
 ************************************************************************\

                              C O P Y R I G H T

   Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
                         All Rights Reserved.

   Licensed under the Creative Commons Attribution-NonCommercial 4.0
   International License (CC BY-NC 4.0).

   For commercial use or licensing inquiries, please contact:
   IRMV lab, Shanghai Jiao Tong University at: https://irmv.sjtu.edu.cn/

 \*************************************************************************

 */

#include <cmath>
#include <fstream>
#include <gtest/gtest.h>
#include "CapsuleFitter.h"
#include "CapsuleURDFGenerator.h"
#include "irmv/third_party/json.hpp"

using namespace urdf_approx_geom;

// Vertices on a circle of radius r in the plane x = x0.
static Eigen::MatrixXd ring(double r, double x0, int n) {
    Eigen::MatrixXd V(n, 3);
    for (int i = 0; i < n; ++i) {
        double a = 2.0 * M_PI * i / n;
        V(i, 0) = x0;
        V(i, 1) = r * std::cos(a);
        V(i, 2) = r * std::sin(a);
    }
    return V;
}

// A capped cylinder (two end rings) must yield radius == r and a segment
// spanning the full length, with every vertex inside.
TEST(CapsuleFit, CylinderAxisIsPrincipalAxis) {
    Eigen::MatrixXd V(64, 3);
    V.topRows(32) = ring(0.05, 0.0, 32);   // x = 0 end
    V.bottomRows(32) = ring(0.05, 1.0, 32); // x = 1 end

    Capsule c = fitCoveringCapsule(V);

    EXPECT_NEAR(c.radius, 0.05, 1e-6);
    EXPECT_NEAR((c.p1 - c.p0).norm(), 1.0, 1e-6);
    for (int i = 0; i < V.rows(); ++i)
        EXPECT_LE(pointToSegmentDistance(V.row(i).transpose(), c.p0, c.p1),
                  c.radius + 1e-9);
}

// An isotropic spherical shell: PCA axis is (near) arbitrary, so the covering
// capsule is NOT degenerate -- its segment may span a diameter. The real
// invariant is that radius never exceeds the true enclosing sphere radius AND
// every sampled vertex lies within radius of the segment.
TEST(CapsuleFit, SphereShellIsCovered) {
    const int n = 200;
    Eigen::MatrixXd V(n, 3);
    for (int i = 0; i < n; ++i) {
        double t = 2.0 * M_PI * i / n;
        double ph = std::acos(1.0 - 2.0 * i / n);  // Fibonacci-ish sphere
        V(i, 0) = 0.1 * std::sin(ph) * std::cos(t);
        V(i, 1) = 0.1 * std::sin(ph) * std::sin(t);
        V(i, 2) = 0.1 * std::cos(ph);
    }

    Capsule c = fitCoveringCapsule(V);

    EXPECT_GT(c.radius, 0.0);
    EXPECT_LE(c.radius, 0.1 + 1e-6);  // never exceeds the true enclosing radius
    for (int i = 0; i < V.rows(); ++i)
        EXPECT_LE(pointToSegmentDistance(V.row(i).transpose(), c.p0, c.p1),
                  c.radius + 1e-9);
}

// Two rings of small spheres (caps shape of a cylinder): the disk-aware fit
// must give radius == ring_r + sphere_r and span the length, covering every
// sphere (dist(center, seg) + r_i <= R).
TEST(CapsuleFit, DiskAwareCylinderOfSpheres) {
    const double ring_r = 0.05, sph_r = 0.01;
    std::vector<Eigen::Vector3d> centers;
    std::vector<double> radii;
    for (double x : {0.0, 1.0}) {
        for (int i = 0; i < 32; ++i) {
            double a = 2.0 * M_PI * i / 32;
            centers.emplace_back(x, ring_r * std::cos(a), ring_r * std::sin(a));
            radii.push_back(sph_r);
        }
    }
    Capsule c = fitCapsuleCoveringDisks(centers, radii);
    EXPECT_NEAR(c.radius, ring_r + sph_r, 1e-6);
    EXPECT_NEAR((c.p1 - c.p0).norm(), 1.0, 1e-6);
    for (size_t i = 0; i < centers.size(); ++i)
        EXPECT_LE(pointToSegmentDistance(centers[i], c.p0, c.p1) + radii[i], c.radius + 1e-9);
}

// ---- sphere clustering (fitCapsulesFromSpheres) ----

// A thin rod of touching spheres -> one capsule, tight radius.
TEST(CapsuleCluster, RodIsOneCapsule) {
    std::vector<Eigen::Vector3d> c;
    std::vector<double> r;
    for (int i = 0; i < 10; ++i) { c.emplace_back(0.02 * i, 0, 0); r.push_back(0.015); }
    auto res = fitCapsulesFromSpheres(c, r, 0.02, 2, 0.6, 3);
    EXPECT_EQ(res.capsules.size(), 1u);
    EXPECT_TRUE(res.spheres.empty());
    EXPECT_NEAR(res.capsules[0].radius, 0.015, 1e-6);
    for (size_t i = 0; i < c.size(); ++i)
        EXPECT_LE(pointToSegmentDistance(c[i], res.capsules[0].p0, res.capsules[0].p1) + r[i],
                  res.capsules[0].radius + 1e-9);
}

// A wide flat blob (5x5 grid) -> split into >=2 capsules (too fat for one).
TEST(CapsuleCluster, FatBlobSplits) {
    std::vector<Eigen::Vector3d> c;
    std::vector<double> r;
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; ++j) { c.emplace_back(0, 0.05 * i, 0.05 * j); r.push_back(0.02); }
    auto res = fitCapsulesFromSpheres(c, r, 0.02, 2, 0.6, 3);
    EXPECT_GE(res.capsules.size(), 2u);
}

// A couple of isolated spheres -> no capsule, kept as spheres.
TEST(CapsuleCluster, IsolatedSpheresKept) {
    std::vector<Eigen::Vector3d> c{{0, 0, 0}, {1, 1, 1}};
    std::vector<double> r{0.01, 0.01};
    auto res = fitCapsulesFromSpheres(c, r, 0.02, 2, 0.6, 3);
    EXPECT_TRUE(res.capsules.empty());
    EXPECT_EQ(res.spheres.size(), 2u);
}

// End-to-end: run CapsuleURDFGenerator on FR3 and verify the JSON sidecar
// carries valid per-link capsule params (p0, p1, radius > 0) in link frame.
TEST(CapsuleRun, FR3EmitsJsonSidecar) {
    CapsuleURDFGenerator g(std::string(URDFApproxGeom_CONFIG_PATH) +
                           "/capsule/capsuleConfig.yml");
    auto ret = g.run("/workspace/resources/fr3/urdf/fr3.urdf",
                     "/workspace/resources/fr3/urdf/fr3_capsuleized.urdf", {});
    ASSERT_TRUE(ret.isOk()) << ret.message();

    std::ifstream f("/workspace/resources/fr3/urdf/fr3_capsuleized.json");
    ASSERT_TRUE(f.good()) << "JSON sidecar not written";
    nlohmann::json j;
    f >> j;
    ASSERT_FALSE(j.empty()) << "no links in JSON";
    for (auto& [link, body] : j.items()) {
        ASSERT_TRUE(body.contains("capsules")) << link << " missing capsules";
        ASSERT_FALSE(body["capsules"].empty()) << link << " has no capsules";
        for (auto& cp : body["capsules"]) {
            EXPECT_EQ(cp["p0"].size(), 3u);
            EXPECT_EQ(cp["p1"].size(), 3u);
            EXPECT_GT(cp["radius"].get<double>(), 0.0);
        }
    }
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
