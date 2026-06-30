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

/// Cross-section extraction (Wu2018 §III-A): slice the mesh with N planes
/// perpendicular to axis `u` (through `origin`), chain triangle-plane
/// intersections into closed 2D contours. `origin` is any point on the axis.
/// Returns up to N contours (one per plane that cuts the mesh).
std::vector<Contour2D> extractSections(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
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

/// Wu2018 sphere+capsule fit (§III-C..E): slice `V,F` along its PCA axis,
/// fit circles per section, link adjacent-section circles into capsules, drop
/// capsules covered by others, grow to cover every vertex. Returns capsules in
/// the same frame as V (constant-radius, ready for <cylinder>+<sphere>).
std::vector<Capsule> fitCapsulesByCrossSection(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                               int n_sections = 10, double coa_threshold = 1e-4,
                                               int max_circles_per_section = 4,
                                               int max_capsules = 6);

}  // namespace urdf_approx_geom

#endif  // URDFAPPROXGEOM_CAPSULECROSSSECTION_H
