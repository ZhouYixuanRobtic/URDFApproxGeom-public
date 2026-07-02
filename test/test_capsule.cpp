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
#include "CapsuleCrossSection.h"
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

// ---- mesh-tight capsule fit (fitCapsulesFromMesh) ----

// Cylinder surface vertices + one over-covering decomposition sphere -> ONE
// tight capsule whose radius ~= the true cylinder radius (NOT the sphere
// radius, which over-covers). Proves the fit hugs the mesh, not the spheres.
TEST(CapsuleMeshFit, CylinderSurfaceIsTight) {
    const double R_true = 0.05;
    Eigen::MatrixXd V(64, 3);
    for (int i = 0; i < 64; ++i) {
        double x = (i < 32) ? 0.0 : 1.0;
        double a = 2.0 * M_PI * (i % 32) / 32;
        V(i, 0) = x; V(i, 1) = R_true * std::cos(a); V(i, 2) = R_true * std::sin(a);
    }
    std::vector<Eigen::Vector3d> c{{0.5, 0, 0}};
    std::vector<double> r{0.1};  // over-covers by 2x on purpose
    auto caps = fitCapsulesFromMesh(V, c, r, 0.02, 8, 0.4, 3);
    ASSERT_EQ(caps.size(), 1u);
    EXPECT_NEAR(caps[0].radius, R_true, 1e-6);
    EXPECT_NEAR((caps[0].p1 - caps[0].p0).norm(), 1.0, 1e-6);
}

// A wide flat slab of vertices -> fat -> k-means split into >=2 capsules.
TEST(CapsuleMeshFit, SlabSplits) {
    std::vector<Eigen::Vector3d> verts;
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            verts.emplace_back(0, 0.05 * i, 0.05 * j);
    Eigen::MatrixXd V(verts.size(), 3);
    for (size_t k = 0; k < verts.size(); ++k) V.row(k) = verts[k].transpose();
    std::vector<Eigen::Vector3d> c{{0, 0.1, 0.1}};
    std::vector<double> r{0.3};
    auto caps = fitCapsulesFromMesh(V, c, r, 0.02, 8, 0.4, 3);
    EXPECT_GE(caps.size(), 2u);
}

// A small capsule fully inside a big one is redundant -> dropped.
TEST(CapsuleMeshFit, NestedCapsuleDeduped) {
    Capsule big{Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(1, 0, 0), 0.1};
    Capsule small{Eigen::Vector3d(0.4, 0, 0), Eigen::Vector3d(0.6, 0, 0), 0.03};  // inside big
    Capsule side{Eigen::Vector3d(0, 1, 0), Eigen::Vector3d(1, 1, 0), 0.05};        // disjoint
    auto out = dedupeNestedCapsules({big, small, side});
    EXPECT_EQ(out.size(), 2u);  // small dropped
}

// Every mesh vertex must be covered by some capsule (collision safety).
TEST(CapsuleMeshFit, AllVerticesCovered) {
    Eigen::MatrixXd V(40, 3);
    for (int i = 0; i < 40; ++i) { V(i, 0) = 0.05 * i; V(i, 1) = 0.03 * std::sin(i); V(i, 2) = 0.03 * std::cos(i); }
    std::vector<Eigen::Vector3d> c{{0.5, 0, 0}};
    std::vector<double> r{0.1};
    auto caps = fitCapsulesFromMesh(V, c, r, 0.02, 8, 0.4, 3);
    ASSERT_FALSE(caps.empty());
    double Rmax = 0.0;
    for (const auto& cap : caps) Rmax = std::max(Rmax, cap.radius);
    for (int i = 0; i < V.rows(); ++i) {
        double best = 1e9;
        for (const auto& cap : caps)
            best = std::min(best, pointToSegmentDistance(V.row(i).transpose(), cap.p0, cap.p1));
        EXPECT_LE(best, Rmax + 1e-9);
    }
}

// ---- Wu2018 cross-section extraction (P1) ----

// Build an open cylinder surface (2 rings, side triangles) of radius r, length L.
static void makeCylinder(double r, double L, int M, Eigen::MatrixXd& V, Eigen::MatrixXi& F) {
    V.resize(2 * M, 3);
    for (int ring = 0; ring < 2; ++ring)
        for (int i = 0; i < M; ++i) {
            double a = 2.0 * M_PI * i / M;
            V.row(ring * M + i) << ring * L, r * std::cos(a), r * std::sin(a);
        }
    std::vector<Eigen::Vector3i> faces;
    for (int i = 0; i < M; ++i) {
        int ni = (i + 1) % M;
        faces.push_back({i, ni, M + i});
        faces.push_back({ni, M + ni, M + i});
    }
    F.resize(faces.size(), 3);
    for (size_t i = 0; i < faces.size(); ++i)
        F.row(i) = faces[i];
}

// Axis-aligned box mesh centered at origin with dimensions (x, y, z).
static void makeBox(double x, double y, double z, Eigen::MatrixXd& V, Eigen::MatrixXi& F) {
    V.resize(8, 3);
    V << -x/2, -y/2, -z/2,
          x/2, -y/2, -z/2,
          x/2,  y/2, -z/2,
         -x/2,  y/2, -z/2,
         -x/2, -y/2,  z/2,
          x/2, -y/2,  z/2,
          x/2,  y/2,  z/2,
         -x/2,  y/2,  z/2;
    F.resize(12, 3);
    F << 0, 1, 2, 0, 2, 3,
         4, 6, 5, 4, 7, 6,
         0, 4, 5, 0, 5, 1,
         1, 5, 6, 1, 6, 2,
         2, 6, 7, 2, 7, 3,
         3, 7, 4, 3, 4, 0;
}

// Two small boxes separated by a narrow neck: the bulge should stay local.
static void makeTwoBoxLink(Eigen::MatrixXd& V, Eigen::MatrixXi& F) {
    Eigen::MatrixXd A, B;
    Eigen::MatrixXi FA, FB;
    makeBox(0.20, 0.20, 0.20, A, FA);
    makeBox(0.20, 0.60, 0.20, B, FB);
    for (int i = 0; i < A.rows(); ++i) A(i, 0) -= 0.20;
    for (int i = 0; i < B.rows(); ++i) B(i, 0) += 0.20;

    V.resize(A.rows() + B.rows(), 3);
    V << A, B;
    F.resize(FA.rows() + FB.rows(), 3);
    F.topRows(FA.rows()) = FA;
    F.bottomRows(FB.rows()) = FB.array() + A.rows();
}

// Slicing a cylinder perpendicular to its axis yields circular contours whose
// radius == the cylinder radius, centered on the axis (2D origin).
TEST(CapsuleXSection, CylinderSectionsAreCircles) {
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeCylinder(0.05, 1.0, 24, V, F);
    auto sections = extractSections(V, F, Eigen::Vector3d::UnitX(), Eigen::Vector3d(0.5, 0, 0), 5);
    EXPECT_GE(sections.size(), 5u);
    for (const auto& s : sections) {
        const auto& pts = s.contour.points;
        ASSERT_GE(pts.size(), 3u);
        Eigen::Vector2d ctr = Eigen::Vector2d::Zero();
        for (const auto& p : pts) ctr += p;
        ctr /= double(pts.size());
        double R = 0.0;
        for (const auto& p : pts) R = std::max(R, (p - ctr).norm());
        EXPECT_NEAR(R, 0.05, 2e-3);       // tight to cylinder radius
        EXPECT_LT(ctr.norm(), 1e-6);      // centered on axis
    }
}

static double capsuleVolume(const Capsule& c) {
    const double L = (c.p1 - c.p0).norm();
    return M_PI * c.radius * c.radius * L + 4.0 * M_PI * c.radius * c.radius * c.radius / 3.0;
}

static double capsuleSetVolume(const std::vector<Capsule>& caps) {
    double v = 0.0;
    for (const auto& c : caps) v += capsuleVolume(c);
    return v;
}

static bool allVerticesCoveredByAnyCapsule(const Eigen::MatrixXd& V,
                                           const std::vector<Capsule>& caps,
                                           double eps = 1e-9) {
    for (int i = 0; i < V.rows(); ++i) {
        bool covered = false;
        Eigen::Vector3d p = V.row(i).transpose();
        for (const auto& c : caps) {
            if (pointToSegmentDistance(p, c.p0, c.p1) <= c.radius + eps) {
                covered = true;
                break;
            }
        }
        if (!covered) return false;
    }
    return true;
}


// ---- Wu2018 COA metric (P2) ----

// Unit square (CCW), side 1 centered at origin.
static Contour2D unitSquare() {
    Contour2D c;
    c.points = {{-0.5, -0.5}, {0.5, -0.5}, {0.5, 0.5}, {-0.5, 0.5}};
    return c;
}

// Circle fully inside the polygon -> no outside area.
TEST(CapsuleCOA, CircleInsidePolygonIsZero) {
    auto sq = unitSquare();
    Circle2D c{{0, 0}, 0.3};
    EXPECT_NEAR(circleOutsideArea(c, sq), 0.0, 1e-6);
}

// Circle centered in polygon, radius larger than the polygon -> outside area =
// circle area - polygon area.
TEST(CapsuleCOA, CircleContainsPolygon) {
    auto sq = unitSquare();  // area = 1
    Circle2D c{{0, 0}, 1.0};
    EXPECT_NEAR(circleOutsideArea(c, sq), M_PI - 1.0, 1e-3);
}

// Circle disjoint from polygon (center outside) -> full circle area.
TEST(CapsuleCOA, CircleOutsidePolygon) {
    auto sq = unitSquare();
    Circle2D c{{3.0, 0.0}, 0.5};
    EXPECT_NEAR(circleOutsideArea(c, sq), M_PI * 0.25, 1e-3);
}

// ---- Wu2018 Lloyd circle clustering (P3) ----

// A narrow circular contour -> one circle ~= its MEC.
TEST(CapsuleLloyd, NarrowCircleIsOneCircle) {
    Contour2D c;
    for (int i = 0; i < 48; ++i) {
        double a = 2.0 * M_PI * i / 48;
        c.points.emplace_back(0.1 * std::cos(a), 0.1 * std::sin(a));
    }
    auto circles = fitCirclesLloyd(c, 0.01, 4);
    ASSERT_EQ(circles.size(), 1u);
    EXPECT_NEAR(circles[0].radius, 0.1, 2e-3);
}

// A wide rectangle -> split into >=2 circles whose union still covers the contour.
TEST(CapsuleLloyd, WideRectangleSplits) {
    Contour2D c;
    c.points = {{-0.5, -0.1}, {0.5, -0.1}, {0.5, 0.1}, {-0.5, 0.1}};  // 1.0 x 0.2
    auto circles = fitCirclesLloyd(c, 0.005, 4);
    EXPECT_GE(circles.size(), 2u);
    // coverage: every contour vertex within some circle
    for (const auto& v : c.points) {
        bool covered = false;
        for (const auto& cir : circles)
            if ((cir.center - v).norm() <= cir.radius + 1e-6) { covered = true; break; }
        EXPECT_TRUE(covered);
    }
}

TEST(CapsuleXSectionFit, CoaThresholdControlsCircleCount) {
    Contour2D c;
    c.points = {{-0.5, -0.1}, {0.5, -0.1}, {0.5, 0.1}, {-0.5, 0.1}};
    std::vector<Contour2D> contours{c};

    auto sparse = fitAdaptiveCirclesForPlane(contours, 10.0, 4);
    auto tight = fitAdaptiveCirclesForPlane(contours, 0.005, 4);

    ASSERT_EQ(sparse.size(), 1u);
    EXPECT_GT(tight.size(), sparse.size());
    EXPECT_LE(tight.size(), 4u);
}

// ---- Wu2018 capsule assembly (P4) ----

// A cylinder -> one capsule spanning its length, radius ~= cylinder radius,
// covering every vertex (collision-safe).
TEST(CapsuleXSectionFit, CylinderToOneCoveringCapsule) {
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeCylinder(0.05, 1.0, 24, V, F);
    auto caps = fitCapsulesByCrossSection(V, F, 2, 0.005, 1, 6);
    ASSERT_EQ(caps.size(), 1u);
    EXPECT_NEAR(caps[0].radius, 0.05, 1e-3);
    EXPECT_NEAR((caps[0].p1 - caps[0].p0).norm(), 1.0, 2e-2);
    for (int i = 0; i < V.rows(); ++i)
        EXPECT_LE(pointToSegmentDistance(V.row(i).transpose(), caps[0].p0, caps[0].p1),
                  caps[0].radius + 1e-9);
}

TEST(CapsuleXSectionFit, WideBoxUsesMultipleCapsulesWhenAllowed) {
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeBox(1.0, 0.20, 0.20, V, F);

    auto sparse = fitCapsulesByCrossSection(V, F, 4, 0.005, 1, 12);
    auto tight = fitCapsulesByCrossSection(V, F, 4, 0.005, 4, 12);

    ASSERT_GE(sparse.size(), 1u);
    EXPECT_GT(tight.size(), sparse.size())
        << "MaxCirclesPerSection must affect the fitter for wide sections";

    double sparse_volume = 0.0;
    for (const auto& c : sparse) {
        const double L = (c.p1 - c.p0).norm();
        sparse_volume += M_PI * c.radius * c.radius * L + 4.0 * M_PI * c.radius * c.radius * c.radius / 3.0;
    }

    double tight_volume = 0.0;
    for (const auto& c : tight) {
        const double L = (c.p1 - c.p0).norm();
        tight_volume += M_PI * c.radius * c.radius * L + 4.0 * M_PI * c.radius * c.radius * c.radius / 3.0;
    }

    EXPECT_LT(tight_volume, 0.85 * sparse_volume)
        << "More circles should reduce over-cover volume on a wide box";
}

TEST(CapsuleXSectionFit, LocalBulgeDoesNotInflateWholeLink) {
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeTwoBoxLink(V, F);

    auto caps = fitCapsulesByCrossSection(V, F, 6, 0.005, 2, 12);
    ASSERT_GE(caps.size(), 2u);

    double smallest_radius = std::numeric_limits<double>::max();
    double largest_radius = 0.0;
    for (const auto& cap : caps) {
        smallest_radius = std::min(smallest_radius, cap.radius);
        largest_radius = std::max(largest_radius, cap.radius);
    }
    EXPECT_LT(smallest_radius, 0.75 * largest_radius)
        << "A narrow section should keep a smaller capsule instead of inheriting the bulge radius";
}

TEST(CapsuleXSectionFit, LocalSplitReducesRadiusBinInflation) {
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeTwoBoxLink(V, F);

    CapsuleFitOptions opts;
    opts.n_sections = 6;
    opts.coa_threshold = 0.005;
    opts.max_circles_per_section = 3;
    opts.max_capsules = 12;
    opts.max_radius_bin_ratio = 1.45;
    opts.adaptive_circle_count = true;

    auto caps = fitCapsulesByCrossSection(V, F, opts);
    auto metrics = evaluateCapsuleTightness(V, caps);
    ASSERT_TRUE(metrics.covered);
    EXPECT_LE(metrics.max_radius_bin_ratio, 1.45);
}

TEST(CapsuleXSectionFit, LocalSplitReducesVolumeWhenAccepted) {
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeTwoBoxLink(V, F);

    CapsuleFitOptions no_split;
    no_split.n_sections = 6;
    no_split.coa_threshold = 0.005;
    no_split.max_circles_per_section = 4;
    no_split.max_capsules = 12;
    no_split.max_radius_bin_ratio = -1.0;
    no_split.adaptive_circle_count = true;
    auto before = fitCapsulesByCrossSection(V, F, no_split);
    auto before_metrics = evaluateCapsuleTightness(V, before);
    ASSERT_TRUE(before_metrics.covered);

    CapsuleFitOptions split = no_split;
    split.max_radius_bin_ratio = 1.45;
    auto after = fitCapsulesByCrossSection(V, F, split);
    auto after_metrics = evaluateCapsuleTightness(V, after);
    ASSERT_TRUE(after_metrics.covered);

    EXPECT_LT(after_metrics.capsule_volume, before_metrics.capsule_volume)
        << "Accepted split must reduce volume, not merely add same-radius segments";
    EXPECT_LE(after_metrics.max_radius_bin_ratio, before_metrics.max_radius_bin_ratio);
}

TEST(CapsuleXSectionFit, BudgetPruningPreservesCoverage) {
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    makeTwoBoxLink(V, F);

    CapsuleFitOptions opts;
    opts.n_sections = 6;
    opts.coa_threshold = 0.005;
    opts.max_circles_per_section = 4;
    opts.max_capsules = 2;
    opts.max_radius_bin_ratio = 1.45;
    opts.adaptive_circle_count = true;

    auto caps = fitCapsulesByCrossSection(V, F, opts);
    ASSERT_LE(caps.size(), 2u);
    EXPECT_TRUE(evaluateCapsuleTightness(V, caps).covered);
}

// End-to-end: run CapsuleURDFGenerator on FR3. Verify (a) the JSON sidecar
// carries valid per-link capsule params, and (b) the output URDF now contains
// NATIVE <cylinder> + <sphere> primitives (capsule = cylinder + 2 end spheres),
// not meshes.
TEST(CapsuleRun, EmitsNativeCylinderSphere) {
    const std::string out_urdf = "/workspace/resources/fr3/urdf/fr3_capsuleized.urdf";
    CapsuleURDFGenerator g(std::string(URDFApproxGeom_CONFIG_PATH) +
                           "/capsule/capsuleConfig.yml");
    auto ret = g.run("/workspace/resources/fr3/urdf/fr3.urdf", out_urdf, {});
    ASSERT_TRUE(ret.isOk()) << ret.message();

    // JSON sidecar: capsules carry p0, p1, radius > 0.
    std::ifstream f("/workspace/resources/fr3/urdf/fr3_capsuleized.json");
    ASSERT_TRUE(f.good()) << "JSON sidecar not written";
    nlohmann::json j;
    f >> j;
    ASSERT_FALSE(j.empty()) << "no links in JSON";
    int capsule_count = 0;
    for (auto& [link, body] : j.items()) {
        if (!body.contains("capsules")) continue;  // link without mesh collision: skip
        for (auto& cp : body["capsules"]) {
            EXPECT_EQ(cp["p0"].size(), 3u);
            EXPECT_EQ(cp["p1"].size(), 3u);
            EXPECT_GT(cp["radius"].get<double>(), 0.0);
            ++capsule_count;
        }
    }
    EXPECT_GT(capsule_count, 0) << "no capsules produced at all";

    // Output URDF: native cylinder + sphere primitives present, mesh gone.
    std::ifstream uf(out_urdf);
    ASSERT_TRUE(uf.good()) << "output URDF not written";
    std::string urdf_txt((std::istreambuf_iterator<char>(uf)), std::istreambuf_iterator<char>());
    EXPECT_NE(urdf_txt.find("<cylinder"), std::string::npos) << "no <cylinder> in URDF";
    EXPECT_NE(urdf_txt.find("<sphere"), std::string::npos) << "no <sphere> in URDF";
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
