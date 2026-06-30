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

#include "CapsuleCrossSection.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace urdf_approx_geom {

// Wu2018 §III-A: slice the mesh with planes perpendicular to axis u, chain the
// triangle-plane intersection segments into closed 2D contours.
std::vector<Contour2D> extractSections(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                       const Eigen::Vector3d& u, const Eigen::Vector3d& origin,
                                       int N) {
    std::vector<Contour2D> out;
    if (V.rows() == 0 || F.rows() == 0 || N <= 0) return out;
    Eigen::Vector3d un = u.normalized();
    Eigen::Vector3d ref = std::abs(un.x()) < 0.9 ? Eigen::Vector3d::UnitX() : Eigen::Vector3d::UnitY();
    Eigen::Vector3d e1 = un.cross(ref).normalized();
    Eigen::Vector3d e2 = un.cross(e1).normalized();
    auto proj = [&](const Eigen::Vector3d& p) {
        Eigen::Vector3d d = p - origin;
        return Eigen::Vector2d(d.dot(e1), d.dot(e2));
    };

    double tmin = std::numeric_limits<double>::max();
    double tmax = std::numeric_limits<double>::lowest();
    for (int i = 0; i < V.rows(); ++i) {
        double t = (V.row(i).transpose() - origin).dot(un);
        tmin = std::min(tmin, t);
        tmax = std::max(tmax, t);
    }
    if (tmax - tmin < 1e-12) return out;

    const double eps = 1e-9;
    auto chain = [&](std::vector<std::pair<Eigen::Vector2d, Eigen::Vector2d>>& segs) {
        std::vector<char> used(segs.size(), 0);
        for (size_t s = 0; s < segs.size(); ++s) {
            if (used[s]) continue;
            used[s] = 1;
            Contour2D c;
            c.points.push_back(segs[s].first);
            c.points.push_back(segs[s].second);
            Eigen::Vector2d cur = segs[s].second;
            Eigen::Vector2d start = segs[s].first;
            bool extended = true;
            while (extended) {
                extended = false;
                for (size_t j = 0; j < segs.size(); ++j) {
                    if (used[j]) continue;
                    const Eigen::Vector2d& A = segs[j].first;
                    const Eigen::Vector2d& B = segs[j].second;
                    bool fa = (A - cur).squaredNorm() <= eps;
                    bool fb = (B - cur).squaredNorm() <= eps;
                    if (!fa && !fb) continue;
                    Eigen::Vector2d next = fa ? B : A;
                    used[j] = 1;
                    if ((next - start).squaredNorm() <= eps) break;  // loop closed
                    c.points.push_back(next);
                    cur = next;
                    extended = true;
                    break;
                }
            }
            out.push_back(std::move(c));
        }
    };

    for (int k = 0; k < N; ++k) {
        double tk = tmin + (k + 0.5) * (tmax - tmin) / N;
        std::vector<std::pair<Eigen::Vector2d, Eigen::Vector2d>> segs;
        for (int f = 0; f < F.rows(); ++f) {
            int va[3] = {F(f, 0), F(f, 1), F(f, 2)};
            double vt[3];
            for (int e = 0; e < 3; ++e) vt[e] = (V.row(va[e]).transpose() - origin).dot(un);
            double lo = std::min({vt[0], vt[1], vt[2]});
            double hi = std::max({vt[0], vt[1], vt[2]});
            if (tk < lo - eps || tk > hi + eps) continue;
            std::vector<Eigen::Vector3d> pts;
            for (int e = 0; e < 3; ++e) {
                int p = va[e], q = va[(e + 1) % 3];
                double tp = vt[e], tq = vt[(e + 1) % 3];
                if (std::abs(tq - tp) < 1e-18) continue;
                bool cross = (tp <= tk && tq >= tk) || (tp >= tk && tq <= tk);
                if (!cross) continue;
                double fr = (tk - tp) / (tq - tp);
                pts.push_back(V.row(p).transpose() +
                              fr * (V.row(q).transpose() - V.row(p).transpose()));
            }
            if (pts.size() == 2) segs.push_back({proj(pts[0]), proj(pts[1])});
        }
        chain(segs);
    }
    return out;
}

double circleOutsideArea(const Circle2D&, const Contour2D&) { return 0.0; }  // P2

std::vector<Circle2D> fitCirclesLloyd(const Contour2D&, double, int) { return {}; }  // P3

std::vector<Capsule> fitCapsulesByCrossSection(const Eigen::MatrixXd&, const Eigen::MatrixXi&,
                                               int, double, int, int) {
    return {};  // P4/P5
}

}  // namespace urdf_approx_geom
