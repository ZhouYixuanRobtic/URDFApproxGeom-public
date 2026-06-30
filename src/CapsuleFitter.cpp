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

// Optimal covering capsule for a FIXED axis u over spheres {centers, radii}:
// lateral center = MEC of projected centers (axis offset), then
// R = max_i( |proj_i - c_perp| + r_i ); segment spans the axial extent.
static Capsule capsule_for_axis(const std::vector<Eigen::Vector3d>& centers,
                                const std::vector<double>& radii,
                                const Eigen::Vector3d& c, Eigen::Vector3d u) {
    u.normalize();
    Eigen::Vector3d ref = std::abs(u.x()) < 0.9 ? Eigen::Vector3d::UnitX() : Eigen::Vector3d::UnitY();
    Eigen::Vector3d e1 = u.cross(ref).normalized();
    Eigen::Vector3d e2 = u.cross(e1).normalized();

    const size_t n = centers.size();
    std::vector<V2> P(n);
    Eigen::VectorXd t(n);
    double tmin = std::numeric_limits<double>::max();
    double tmax = std::numeric_limits<double>::lowest();
    for (size_t i = 0; i < n; ++i) {
        Eigen::Vector3d d = centers[i] - c;
        P[i] = V2(d.dot(e1), d.dot(e2));
        t(i) = d.dot(u);
        tmin = std::min(tmin, t(i));
        tmax = std::max(tmax, t(i));
    }
    // ponytail: MEC of projected *centers* (not disks) -> lateral center is
    // near-optimal; r_i spread is small. R inflates by each disk radius so the
    // fit always covers (collision-safe). True MEC-of-disks only if too loose.
    Circle2 circ = min_enclosing_circle(P);
    Eigen::Vector3d seg_center = c + circ.c.x() * e1 + circ.c.y() * e2;
    double R = 0.0;
    for (size_t i = 0; i < n; ++i)
        R = std::max(R, (P[i] - circ.c).norm() + radii[i]);

    Capsule cap;
    cap.p0 = seg_center + tmin * u;
    cap.p1 = seg_center + tmax * u;
    cap.radius = R;
    return cap;
}

static Eigen::Vector3d rotate_around(const Eigen::Vector3d& u,
                                     const Eigen::Vector3d& axis, double ang) {
    double c = std::cos(ang), s = std::sin(ang);
    return (u * c + axis.cross(u) * s + axis * (axis.dot(u)) * (1.0 - c)).normalized();
}

// Candidate axis directions + local pattern-search refinement, shared by the
// point and disk fitters. Returns the min-radius covering Capsule.
static Capsule fit_axis_search(const std::vector<Eigen::Vector3d>& centers,
                               const std::vector<double>& radii) {
    Eigen::Vector3d c = Eigen::Vector3d::Zero();
    for (const auto& p : centers) c += p;
    c /= double(centers.size());

    // candidate directions: PCA eigenvectors, cardinals, diameter, fibonacci sphere.
    Eigen::MatrixXd Cn(centers.size(), 3);
    for (size_t i = 0; i < centers.size(); ++i) Cn.row(i) = centers[i].transpose() - c.transpose();
    Eigen::Matrix3d cov = (Cn.transpose() * Cn) / double(centers.size());
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es(cov);
    std::vector<Eigen::Vector3d> dirs;
    for (int k = 0; k < 3; ++k) dirs.push_back(es.eigenvectors().col(k));
    dirs.emplace_back(Eigen::Vector3d::UnitX());
    dirs.emplace_back(Eigen::Vector3d::UnitY());
    dirs.emplace_back(Eigen::Vector3d::UnitZ());
    {
        const int step = std::max(1, static_cast<int>(centers.size()) / 1500);
        double best = -1.0;
        Eigen::Vector3d diam = Eigen::Vector3d::UnitX();
        for (size_t i = 0; i < centers.size(); i += step)
            for (size_t j = i + 1; j < centers.size(); j += step) {
                double d = (centers[i] - centers[j]).squaredNorm();
                if (d > best) {
                    best = d;
                    diam = (centers[j] - centers[i]).normalized();
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
        Capsule cap = capsule_for_axis(centers, radii, c, u);
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
            Capsule cand = capsule_for_axis(centers, radii, c, rotate_around(u, ax, step));
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

Capsule fitCapsuleCoveringDisks(const std::vector<Eigen::Vector3d>& centers,
                                const std::vector<double>& radii) {
    if (centers.empty()) return {Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(), 0.0};
    std::vector<double> r = radii;
    if (r.size() != centers.size()) r.assign(centers.size(), 0.0);
    if (centers.size() == 1) return {centers[0], centers[0], r[0]};
    return fit_axis_search(centers, r);
}

Capsule fitCoveringCapsule(const Eigen::MatrixXd& V) {
    std::vector<Eigen::Vector3d> centers(V.rows());
    for (int i = 0; i < V.rows(); ++i) centers[i] = V.row(i).transpose();
    std::vector<double> zeros(V.rows(), 0.0);
    return fitCapsuleCoveringDisks(centers, zeros);
}

std::vector<Capsule> fitCoveringCapsules(const Eigen::MatrixXd& V,
                                         double split_volume_ratio,
                                         int max_capsules) {
    // v1 single-capsule path retained until the generator switches to the
    // sphere-based fitter (fitCapsulesFromSpheres). split hook documented.
    (void)split_volume_ratio;
    (void)max_capsules;
    return {fitCoveringCapsule(V)};
}

}  // namespace urdf_approx_geom
