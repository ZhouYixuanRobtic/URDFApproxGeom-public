/*
 * ConvexHullBackend.cpp
 *
 * Thin public wrapper around the vendored quickhull.hpp implementation.
 */

#include "ConvexHullBackend.h"
#include <quickhull.hpp>

namespace urdf_approx_geom {

void computeConvexHull3D(const Eigen::MatrixXd& V, Eigen::MatrixXd& HV,
                         Eigen::MatrixXi& HF) {
    quickhull_detail::computeConvexHull3DImpl(V, HV, HF);
}

}  // namespace urdf_approx_geom
