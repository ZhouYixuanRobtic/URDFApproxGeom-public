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

#include "CapsuleFitter.h"

#include <algorithm>

namespace urdf_approx_geom {

double pointToSegmentDistance(const Eigen::Vector3d& p,
                              const Eigen::Vector3d& a,
                              const Eigen::Vector3d& b) {
    Eigen::Vector3d d = b - a;
    double denom = d.squaredNorm();
    double t = denom > 0.0 ? std::clamp((p - a).dot(d) / denom, 0.0, 1.0) : 0.0;
    return (p - (a + t * d)).norm();
}

Capsule fitCoveringCapsule(const Eigen::MatrixXd& V) {
    Eigen::Vector3d c = V.colwise().mean();                 // centroid
    Eigen::MatrixXd C = V.rowwise() - c.transpose();        // centered
    Eigen::Matrix3d cov = (C.transpose() * C) / double(V.rows());

    // Principal axis = eigenvector of the largest eigenvalue.
    // SelfAdjointEigenSolver sorts eigenvalues ascending, so col(2) is largest.
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es(cov);
    Eigen::Vector3d u = es.eigenvectors().col(2);

    Eigen::VectorXd t = C * u;
    double tmin = t.minCoeff();
    double tmax = t.maxCoeff();

    Capsule cap;
    cap.p0 = c + tmin * u;
    cap.p1 = c + tmax * u;

    double r = 0.0;
    for (int i = 0; i < V.rows(); ++i)
        r = std::max(r, pointToSegmentDistance(V.row(i).transpose(), cap.p0, cap.p1));
    cap.radius = r;
    return cap;
}

std::vector<Capsule> fitCoveringCapsules(const Eigen::MatrixXd& V,
                                         double split_volume_ratio,
                                         int max_capsules) {
    // ponytail: single covering capsule per link in v1. Adaptive splitting along
    // the principal axis (when capsule-volume / convex-hull-volume >
    // split_volume_ratio) is the documented upgrade path; knobs are wired through
    // so config can turn it on without an API change.
    (void)split_volume_ratio;
    (void)max_capsules;
    return {fitCoveringCapsule(V)};
}

}  // namespace urdf_approx_geom
