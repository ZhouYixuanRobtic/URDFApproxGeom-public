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
#include <utility>
#include <vector>

namespace urdf_approx_geom {

/// A covering capsule: segment [p0, p1] in link frame + radius.
struct Capsule {
    Eigen::Vector3d p0;
    Eigen::Vector3d p1;
    double radius = 0.0;
};

/// Fit a single covering capsule to a point cloud (mesh surface). Thin wrapper
/// over fitCapsuleCoveringDisks with zero radii (points = degenerate disks).
/// Tight: radius = max point-to-segment distance over the optimal axis.
Capsule fitCoveringCapsule(const Eigen::MatrixXd& V);

/// Covering capsule for a set of spheres {centers[i], radii[i]}: for the
/// optimal axis, radius = max_i( dist(center_i, axis_segment) + r_i ).
/// (Core axis search; fitCoveringCapsule calls it with zero radii.)
Capsule fitCapsuleCoveringDisks(const std::vector<Eigen::Vector3d>& centers,
                                const std::vector<double>& radii);

/// Point-to-segment distance, exposed for coverage assertions.
double pointToSegmentDistance(const Eigen::Vector3d& p,
                              const Eigen::Vector3d& a,
                              const Eigen::Vector3d& b);

/// Sphere-decomposed, mesh-tight capsule fit. The sphere-tree spheres are a
/// COVERING approximation -- their radii over-cover the real surface (measured
/// ~1.4x), so a capsule fit to the spheres inherits that fatness. Instead:
///   1. cluster the spheres by proximity (Union-Find) -> link decomposition,
///   2. assign each mesh vertex to its nearest sphere's cluster,
///   3. fit a TIGHT covering capsule to each cluster's mesh vertices (radius
///      = true surface distance, not the sphere radius),
///   4. recursively k-means-split clusters whose capsule is "fat"
///      (radius > fat_split_ratio*segment_length), up to max_capsules.
/// Clusters with fewer than min_cluster_size vertices fold into the nearest
/// cluster so every vertex stays covered.
std::vector<Capsule> fitCapsulesFromMesh(const Eigen::MatrixXd& V,
                                         const std::vector<Eigen::Vector3d>& centers,
                                         const std::vector<double>& radii,
                                         double cluster_gap = 0.02,
                                         int min_cluster_size = 8,
                                         double fat_split_ratio = 0.4,
                                         int max_capsules = 3);

}  // namespace urdf_approx_geom

#endif  // URDFAPPROXGEOM_CAPSULEFITTER_H
