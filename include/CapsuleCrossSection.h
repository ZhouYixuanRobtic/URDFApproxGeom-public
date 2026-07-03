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

#ifndef URDFAPPROXGEOM_CAPSULECROSSSECTION_H
#define URDFAPPROXGEOM_CAPSULECROSSSECTION_H

#include <Eigen/Dense>
#include <vector>

#include "CapsuleFitter.h"  // Capsule

namespace urdf_approx_geom {

/// 2D circle (cross-section primitive).
struct Circle2D {
    Eigen::Vector2d center{0, 0};
    double radius = 0.0;
};

/// Ordered closed contour in a cross-section plane (2D).
struct Contour2D {
    std::vector<Eigen::Vector2d> points;  // ordered, implicitly closed (last->first)
};

/// A cross-section: its 2D contour + axial position t along the axis
/// (3D plane = { p : u.(p-origin) = t }).
struct Section2D {
    Contour2D contour;
    double t = 0.0;
};

/// Cross-section extraction (Wu2018 §III-A): slice the mesh with N planes
/// perpendicular to axis `u` (through `origin`), chain triangle-plane
/// intersections into closed 2D contours. Sections span the full axial extent
/// [tmin,tmax] (endpoint-inclusive) so capsules cover the whole link. Returns
/// one Section2D per plane that cuts the mesh (ascending t).
std::vector<Section2D> extractSections(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                       const Eigen::Vector3d& u, const Eigen::Vector3d& origin,
                                       int N);

/// Wu2018 Circle-Outside-Area (§III-B, Eq 1-5): area inside `circle` but outside
/// the polygon `contour`. The over-cover error minimized during fitting.
double circleOutsideArea(const Circle2D& circle, const Contour2D& contour);

/// Wu2018 Lloyd circle clustering (§III-B): approximate `contour` with circles
/// whose total COA < `coa_threshold` (teleportation splits the worst circle
/// until met or `max_circles` reached). Welzl MEC seeds the single-circle case.
std::vector<Circle2D> fitCirclesLloyd(const Contour2D& contour, double coa_threshold,
                                      int max_circles);

/// Fit a fixed number of covering circles to all contours in one section plane.
/// k==1 is the MEC fallback. k>1 uses deterministic k-means seeds followed by
/// MEC refits per cluster so every sampled contour point remains covered.
std::vector<Circle2D> fitFixedCountCirclesForPlane(const std::vector<Contour2D>& contours,
                                                   int k);

struct CapsuleFitOptions {
    int n_sections = 10;
    double coa_threshold = 1e-4;
    int max_circles_per_section = 4;
    int max_capsules = 6;
    double max_radius_bin_ratio = 1.45;
    double max_capv_aabb_ratio = -1.0;
    double min_split_volume_improvement = 0.005;
    bool adaptive_circle_count = true;
};

struct CapsuleTightnessMetrics {
    bool covered = false;
    double worst_signed_distance = 0.0;
    double capsule_volume = 0.0;
    double aabb_volume = 0.0;
    double capV_aabb = 0.0;
    double max_radius_bin_ratio = 0.0;
};

struct CapsuleVertexAssignment {
    std::vector<int> capsule_index;
    std::vector<double> raw_distance;
    std::vector<double> signed_distance;
};

CapsuleVertexAssignment assignVerticesToCapsules(const Eigen::MatrixXd& V,
                                                 const std::vector<Capsule>& caps);

CapsuleTightnessMetrics evaluateCapsuleTightness(const Eigen::MatrixXd& V,
                                                 const std::vector<Capsule>& caps);

double assignedPlaneCircleScore(const std::vector<Contour2D>& contours,
                                const std::vector<Circle2D>& circles);

std::vector<Circle2D> fitAdaptiveCirclesForPlane(const std::vector<Contour2D>& contours,
                                                 double coa_threshold,
                                                 int max_circles);

/// Wu2018 sphere+capsule fit (§III-C..E): slice `V,F` along its PCA axis,
/// fit circles per section, link adjacent-section circles into capsules, drop
/// capsules covered by others, grow to cover every vertex. Returns capsules in
/// the same frame as V (constant-radius, ready for <cylinder>+<sphere>).
std::vector<Capsule> fitCapsulesByCrossSection(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                               int n_sections = 10, double coa_threshold = 1e-4,
                                               int max_circles_per_section = 4,
                                               int max_capsules = 6);

std::vector<Capsule> fitCapsulesByCrossSection(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                               const CapsuleFitOptions& options);

/// Grow capsule radii so every vertex is within its nearest capsule
/// (collision-safe outer fit). Call against the ORIGINAL mesh vertices when the
/// fit was run on a processed (e.g. watertight) mesh that may differ.
void growCapsulesToCover(std::vector<Capsule>& caps, const Eigen::MatrixXd& V);

}  // namespace urdf_approx_geom

#endif  // URDFAPPROXGEOM_CAPSULECROSSSECTION_H
