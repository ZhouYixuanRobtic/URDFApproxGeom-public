/*
 * Mesh validation tests for the ManifoldPlus-free pipeline.
 *
 * Covers the contract in docs/dual-edition-plan.md section 5.2:
 *   - watertight cube passes closed modes;
 *   - missing-face cube fails capsule/multi-sphere;
 *   - duplicate vertices are merged;
 *   - non-manifold edges fail;
 *   - convex/single-sphere accept open meshes.
 */

#include <gtest/gtest.h>
#include <Eigen/Dense>
#include "ConvexHullBackend.h"
#include "URDFGenerator.h"

namespace {

class ExposedURDFGenerator : public URDFGenerator {
  public:
    using URDFGenerator::validateMeshForMode;

    irmv_core::bot_common::ErrorInfo run(
        const std::string&, const std::string&,
        const std::vector<std::pair<std::string, std::string>>&) override {
        return irmv_core::bot_common::ErrorInfo::ok();
    }
};

Eigen::MatrixXd cubeV() {
    Eigen::MatrixXd V(8, 3);
    V << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1;
    return V;
}

Eigen::MatrixXi cubeF() {
    Eigen::MatrixXi F(12, 3);
    F << 0, 1, 4, 0, 4, 2, 3, 5, 7, 3, 7, 6, 0, 1, 5, 0, 5, 3, 2, 4, 7, 2, 7, 6, 0, 3, 6, 0, 6, 2, 1, 4, 7, 1, 7, 5;
    return F;
}

}  // namespace

TEST(MeshValidation, WatertightCubePassesClosedModes) {
    ExposedURDFGenerator gen;
    Eigen::MatrixXd outV;
    Eigen::MatrixXi outF;
    auto ret = gen.validateMeshForMode(cubeV(), cubeF(), MeshMode::Capsule, outV, outF, "link", "cube.obj");
    EXPECT_TRUE(ret.isOk());
    EXPECT_GT(outF.rows(), 0);
}

TEST(MeshValidation, MissingFaceCubeFailsClosedModes) {
    ExposedURDFGenerator gen;
    Eigen::MatrixXd V = cubeV();
    Eigen::MatrixXi F = cubeF();
    // Remove the last face, leaving a hole.
    F.conservativeResize(F.rows() - 1, Eigen::NoChange);
    Eigen::MatrixXd outV;
    Eigen::MatrixXi outF;
    auto ret = gen.validateMeshForMode(V, F, MeshMode::Capsule, outV, outF, "link", "open.obj");
    EXPECT_FALSE(ret.isOk());
    EXPECT_NE(ret.message().find("not watertight"), std::string::npos);
}

TEST(MeshValidation, DuplicateVerticesAreMerged) {
    ExposedURDFGenerator gen;
    Eigen::MatrixXd V(10, 3);
    V.topRows(8) = cubeV();
    V.row(8) = V.row(0);
    V.row(9) = V.row(1);
    Eigen::MatrixXi F = cubeF();
    // Faces referencing the duplicated vertices should still merge cleanly.
    for (int i = 0; i < F.rows(); ++i) {
        for (int j = 0; j < 3; ++j) {
            if (F(i, j) == 0) F(i, j) = 8;
            else if (F(i, j) == 1) F(i, j) = 9;
        }
    }
    Eigen::MatrixXd outV;
    Eigen::MatrixXi outF;
    auto ret = gen.validateMeshForMode(V, F, MeshMode::Capsule, outV, outF, "link", "dup.obj");
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(outV.rows(), 8);
}

TEST(MeshValidation, NonManifoldEdgeFailsClosedModes) {
    ExposedURDFGenerator gen;
    Eigen::MatrixXd V = cubeV();
    Eigen::MatrixXi F(13, 3);
    F.topRows(12) = cubeF();
    // Add a triangle that shares edge (0,1) a third time.
    F.row(12) << 0, 1, 2;
    Eigen::MatrixXd outV;
    Eigen::MatrixXi outF;
    auto ret = gen.validateMeshForMode(V, F, MeshMode::SphereTree, outV, outF, "link", "nonmanifold.obj");
    EXPECT_FALSE(ret.isOk());
    EXPECT_NE(ret.message().find("non-manifold"), std::string::npos);
}

TEST(MeshValidation, OpenMeshAllowedForConvexAndSingleSphere) {
    ExposedURDFGenerator gen;
    Eigen::MatrixXd V = cubeV();
    Eigen::MatrixXi F = cubeF();
    F.conservativeResize(F.rows() - 1, Eigen::NoChange);
    Eigen::MatrixXd outV;
    Eigen::MatrixXi outF;
    EXPECT_TRUE(gen.validateMeshForMode(V, F, MeshMode::Convex, outV, outF, "link", "open.obj").isOk());
    EXPECT_TRUE(gen.validateMeshForMode(V, F, MeshMode::SphereSingle, outV, outF, "link", "open.obj").isOk());
}

TEST(ConvexHullBackend, UnitCubeHullHasExpectedVolumeAndContainsInputs) {
    Eigen::MatrixXd V = cubeV();
    Eigen::MatrixXd HV;
    Eigen::MatrixXi HF;
    urdf_approx_geom::computeConvexHull3D(V, HV, HF);
    ASSERT_GE(HF.rows(), 4);
    ASSERT_GE(HV.rows(), 4);

    // Signed volume of an outward-oriented closed triangle mesh.
    double volume = 0.0;
    Eigen::Vector3d center = HV.colwise().mean().transpose();
    for (int i = 0; i < HF.rows(); ++i) {
        Eigen::Vector3d a = HV.row(HF(i, 0)).transpose() - center;
        Eigen::Vector3d b = HV.row(HF(i, 1)).transpose() - center;
        Eigen::Vector3d c = HV.row(HF(i, 2)).transpose() - center;
        volume += a.cross(b).dot(c) / 6.0;
    }
    EXPECT_NEAR(volume, 1.0, 1e-6);

    // Every input vertex must lie inside or on the hull.
    for (int i = 0; i < V.rows(); ++i) {
        Eigen::Vector3d p = V.row(i).transpose();
        for (int f = 0; f < HF.rows(); ++f) {
            Eigen::Vector3d a = HV.row(HF(f, 0)).transpose();
            Eigen::Vector3d b = HV.row(HF(f, 1)).transpose();
            Eigen::Vector3d c = HV.row(HF(f, 2)).transpose();
            EXPECT_LE((b - a).cross(c - a).dot(p - a), 1e-8);
        }
    }
}
