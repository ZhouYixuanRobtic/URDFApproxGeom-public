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
#include <gtest/gtest.h>
#include "CapsuleFitter.h"

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

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
