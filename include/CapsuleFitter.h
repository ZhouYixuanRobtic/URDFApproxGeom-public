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

#ifndef URDFAPPROXGEOM_CAPSULEFITTER_H
#define URDFAPPROXGEOM_CAPSULEFITTER_H

#include <Eigen/Dense>
#include <vector>

namespace urdf_approx_geom {

/// A covering capsule: segment [p0, p1] in link frame + radius.
struct Capsule {
    Eigen::Vector3d p0;
    Eigen::Vector3d p1;
    double radius = 0.0;
};

/// Fit a single covering capsule to a point cloud. Thin wrapper over
/// fitCapsuleCoveringDisks with zero radii (points = degenerate disks).
Capsule fitCoveringCapsule(const Eigen::MatrixXd& V);

/// Covering capsule for a set of spheres {centers[i], radii[i]}: for the
/// optimal axis, radius = max_i( dist(center_i, axis_segment) + r_i ).
/// Conservative outer fit -- every sphere lies inside the capsule.
/// (Reuses the v1 candidate-axis + sphere pattern-search machinery.)
Capsule fitCapsuleCoveringDisks(const std::vector<Eigen::Vector3d>& centers,
                                const std::vector<double>& radii);

/// Fit 1..max_capsules covering capsules (v1 single-capsule path; superseded
/// by fitCapsulesFromSpheres once the generator switches to the sphere path).
std::vector<Capsule> fitCoveringCapsules(const Eigen::MatrixXd& V,
                                         double split_volume_ratio = 5.0,
                                         int max_capsules = 2);

/// Point-to-segment distance, exposed for coverage assertions.
double pointToSegmentDistance(const Eigen::Vector3d& p,
                              const Eigen::Vector3d& a,
                              const Eigen::Vector3d& b);

}  // namespace urdf_approx_geom

#endif  // URDFAPPROXGEOM_CAPSULEFITTER_H
