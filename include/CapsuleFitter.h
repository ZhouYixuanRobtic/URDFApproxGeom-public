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

/// Fit a single covering capsule to a point cloud: PCA principal axis defines
/// the segment extent, radius = max point-to-segment distance (conservative /
/// outer fit, the collision-safe choice).
Capsule fitCoveringCapsule(const Eigen::MatrixXd& V);

/// Fit 1..max_capsules covering capsules. v1 returns a single capsule per call;
/// the split_volume_ratio knob is retained for adaptive splitting of wide /
/// non-convex links (capsule-volume / convex-hull-volume > ratio -> split).
std::vector<Capsule> fitCoveringCapsules(const Eigen::MatrixXd& V,
                                         double split_volume_ratio = 5.0,
                                         int max_capsules = 2);

/// Point-to-segment distance, exposed for coverage assertions.
double pointToSegmentDistance(const Eigen::Vector3d& p,
                              const Eigen::Vector3d& a,
                              const Eigen::Vector3d& b);

}  // namespace urdf_approx_geom

#endif  // URDFAPPROXGEOM_CAPSULEFITTER_H
