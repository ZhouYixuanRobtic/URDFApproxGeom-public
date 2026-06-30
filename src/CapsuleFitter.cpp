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
#include <limits>

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
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es(cov);

    // Candidate axes: 3 PCA eigenvectors, 3 cardinal axes, and the point-cloud
    // diameter (direction of the two farthest points). Raw PCA picks the
    // largest-variance direction, which for a wide/flat link (e.g. a base) is a
    // tilted diagonal across its footprint -> a long, tilted capsule. Choosing
    // the axis that minimizes the covering radius avoids that.
    // ponytail: heuristic min-radius over candidates; the true minimum-enclosing
    // capsule needs numerical optimization (roboptim/Ipopt) which we skip.
    std::vector<Eigen::Vector3d> axes;
    for (int k = 0; k < 3; ++k) axes.push_back(es.eigenvectors().col(k));
    axes.emplace_back(Eigen::Vector3d::UnitX());
    axes.emplace_back(Eigen::Vector3d::UnitY());
    axes.emplace_back(Eigen::Vector3d::UnitZ());
    {
        const int step = std::max(1, static_cast<int>(V.rows()) / 1500);
        double best = -1.0;
        Eigen::Vector3d diam = Eigen::Vector3d::UnitX();
        for (int i = 0; i < V.rows(); i += step)
            for (int j = i + 1; j < V.rows(); j += step) {
                double d = (V.row(i) - V.row(j)).squaredNorm();
                if (d > best) {
                    best = d;
                    diam = (V.row(j) - V.row(i)).transpose().normalized();
                }
            }
        axes.push_back(diam);
    }

    Capsule best;
    double best_r = std::numeric_limits<double>::max();
    for (Eigen::Vector3d u : axes) {
        u.normalize();
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
        if (r < best_r) {
            best_r = r;
            best = cap;
        }
    }
    return best;
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
