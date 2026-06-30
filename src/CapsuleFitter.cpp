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
#include <cmath>
#include <limits>
#include <random>
#include <vector>

namespace urdf_approx_geom {

double pointToSegmentDistance(const Eigen::Vector3d& p,
                              const Eigen::Vector3d& a,
                              const Eigen::Vector3d& b) {
    Eigen::Vector3d d = b - a;
    double denom = d.squaredNorm();
    double t = denom > 0.0 ? std::clamp((p - a).dot(d) / denom, 0.0, 1.0) : 0.0;
    return (p - (a + t * d)).norm();
}

// ---- minimum enclosing circle (2D), Welzl move-to-front -----------------
using V2 = Eigen::Vector2d;
struct Circle2 { V2 c; double r; };

static Circle2 circle_from(const V2& a, const V2& b) {
    return {0.5 * (a + b), 0.5 * (a - b).norm()};
}
static Circle2 circle_from(const V2& a, const V2& b, const V2& c) {
    double ax = a.x(), ay = a.y(), bx = b.x(), by = b.y(), cx = c.x(), cy = c.y();
    double d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    if (std::abs(d) < 1e-18) {
        // degenerate: pick the farthest pair
        double dab = (a - b).squaredNorm(), dac = (a - c).squaredNorm(), dbc = (b - c).squaredNorm();
        if (dab >= dac && dab >= dbc) return circle_from(a, b);
        if (dac >= dbc) return circle_from(a, c);
        return circle_from(b, c);
    }
    double ux = ((ax * ax + ay * ay) * (by - cy) + (bx * bx + by * by) * (cy - ay) +
                 (cx * cx + cy * cy) * (ay - by)) / d;
    double uy = ((ax * ax + ay * ay) * (cx - bx) + (bx * bx + by * by) * (ax - cx) +
                 (cx * cx + cy * cy) * (bx - ax)) / d;
    V2 ctr(ux, uy);
    return {ctr, (a - ctr).norm()};
}

static Circle2 min_enclosing_circle(std::vector<V2> P) {
    std::mt19937 rng(0xC0FFEE);  // fixed seed: deterministic
    std::shuffle(P.begin(), P.end(), rng);
    Circle2 C{V2::Zero(), -1.0};
    for (size_t i = 0; i < P.size(); ++i) {
        if (C.r < 0.0 || (P[i] - C.c).norm() > C.r + 1e-12) {
            C = {P[i], 0.0};
            for (size_t j = 0; j < i; ++j) {
                if ((P[j] - C.c).norm() > C.r + 1e-12) {
                    C = circle_from(P[i], P[j]);
                    for (size_t k = 0; k < j; ++k) {
                        if ((P[k] - C.c).norm() > C.r + 1e-12) {
                            C = circle_from(P[i], P[j], P[k]);
                        }
                    }
                }
            }
        }
    }
    if (C.r < 0.0) C = {V2::Zero(), 0.0};
    return C;
}

// Optimal covering capsule for a FIXED axis u: radius = MEC of points
// projected to the plane perpendicular to u; segment spans the axial extent.
static Capsule capsule_for_axis(const Eigen::MatrixXd& V, const Eigen::Vector3d& c,
                                Eigen::Vector3d u) {
    u.normalize();
    Eigen::Vector3d ref = std::abs(u.x()) < 0.9 ? Eigen::Vector3d::UnitX() : Eigen::Vector3d::UnitY();
    Eigen::Vector3d e1 = u.cross(ref).normalized();
    Eigen::Vector3d e2 = u.cross(e1).normalized();

    std::vector<V2> P(V.rows());
    Eigen::VectorXd t(V.rows());
    for (int i = 0; i < V.rows(); ++i) {
        Eigen::Vector3d d = V.row(i).transpose() - c;
        P[i] = V2(d.dot(e1), d.dot(e2));
        t(i) = d.dot(u);
    }
    Circle2 circ = min_enclosing_circle(P);
    Eigen::Vector3d seg_center = c + circ.c.x() * e1 + circ.c.y() * e2;
    Capsule cap;
    cap.p0 = seg_center + t.minCoeff() * u;
    cap.p1 = seg_center + t.maxCoeff() * u;
    cap.radius = circ.r;
    return cap;
}

static Eigen::Vector3d rotate_around(const Eigen::Vector3d& u,
                                     const Eigen::Vector3d& axis, double ang) {
    // Rodrigues rotation of u about unit `axis` by ang.
    double c = std::cos(ang), s = std::sin(ang);
    return (u * c + axis.cross(u) * s + axis * (axis.dot(u)) * (1.0 - c)).normalized();
}

Capsule fitCoveringCapsule(const Eigen::MatrixXd& V) {
    Eigen::Vector3d c = V.colwise().mean();

    // candidate directions: PCA eigenvectors, cardinals, diameter, fibonacci sphere.
    Eigen::MatrixXd Cn = V.rowwise() - c.transpose();
    Eigen::Matrix3d cov = (Cn.transpose() * Cn) / double(V.rows());
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es(cov);
    std::vector<Eigen::Vector3d> dirs;
    for (int k = 0; k < 3; ++k) dirs.push_back(es.eigenvectors().col(k));
    dirs.emplace_back(Eigen::Vector3d::UnitX());
    dirs.emplace_back(Eigen::Vector3d::UnitY());
    dirs.emplace_back(Eigen::Vector3d::UnitZ());
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
        dirs.push_back(diam);
    }
    const int NF = 96;
    for (int k = 0; k < NF; ++k) {
        double phi = std::acos(1.0 - 2.0 * (k + 0.5) / NF);
        double th = M_PI * (1.0 + std::sqrt(5.0)) * k;
        dirs.emplace_back(std::sin(phi) * std::cos(th), std::sin(phi) * std::sin(th),
                          std::cos(phi));
    }

    Capsule best;
    double best_r = std::numeric_limits<double>::max();
    for (const auto& u : dirs) {
        Capsule cap = capsule_for_axis(V, c, u);
        if (cap.radius < best_r) {
            best_r = cap.radius;
            best = cap;
        }
    }

    // local refinement (pattern search on the sphere) for the true minimum.
    Eigen::Vector3d u = (best.p1 - best.p0).normalized();
    if (u.hasNaN() || (best.p1 - best.p0).norm() < 1e-12) u = Eigen::Vector3d::UnitX();
    double step = 0.25;
    while (step > 1e-3) {
        bool improved = false;
        Eigen::Vector3d ref = std::abs(u.x()) < 0.9 ? Eigen::Vector3d::UnitX() : Eigen::Vector3d::UnitY();
        Eigen::Vector3d e1 = u.cross(ref).normalized();
        Eigen::Vector3d e2 = u.cross(e1).normalized();
        std::vector<Eigen::Vector3d> rot_axes = {e1, e2, Eigen::Vector3d(-e1), Eigen::Vector3d(-e2)};
        for (const Eigen::Vector3d& ax : rot_axes) {
            Capsule cand = capsule_for_axis(V, c, rotate_around(u, ax, step));
            if (cand.radius < best_r - 1e-9) {
                best_r = cand.radius;
                best = cand;
                u = (cand.p1 - cand.p0).normalized();
                improved = true;
            }
        }
        if (!improved) step *= 0.5;
    }
    return best;
}

std::vector<Capsule> fitCoveringCapsules(const Eigen::MatrixXd& V,
                                         double split_volume_ratio,
                                         int max_capsules) {
    // ponytail: single optimal capsule per link. Adaptive splitting along the
    // axis (when capsule/hull volume > split_volume_ratio) is the documented
    // escalation for wide/non-capsule-shaped links (e.g. a robot base).
    (void)split_volume_ratio;
    (void)max_capsules;
    return {fitCoveringCapsule(V)};
}

}  // namespace urdf_approx_geom
