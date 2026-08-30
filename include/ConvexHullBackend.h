/*
 * ConvexHullBackend.h
 *
 * Thin wrapper around the vendored header-only QuickHull replacement.
 * Both research and commercial editions use this backend; it does not depend
 * on CGAL, GMP, or sphere_tree.
 */

#ifndef URDFAPPROXGEOM_CONVEXHULLBACKEND_H
#define URDFAPPROXGEOM_CONVEXHULLBACKEND_H

#include <Eigen/Dense>

namespace urdf_approx_geom {

/// Compute the 3D convex hull of the input point set.
///
/// @param V  #V by 3 input vertices
/// @param HV #HV by 3 output hull vertices (subset of unique input vertices)
/// @param HF #HF by 3 output triangle faces (manifold, outward orientation)
void computeConvexHull3D(const Eigen::MatrixXd& V, Eigen::MatrixXd& HV,
                         Eigen::MatrixXi& HF);

}  // namespace urdf_approx_geom

#endif  // URDFAPPROXGEOM_CONVEXHULLBACKEND_H
