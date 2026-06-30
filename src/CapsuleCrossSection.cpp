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

namespace {
// Ray-cast point-in-polygon test (polygon given as ordered closed loop).
bool pointInPolygon(const Eigen::Vector2d& o, const std::vector<Eigen::Vector2d>& pts) {
    bool inside = false;
    int n = static_cast<int>(pts.size());
    for (int i = 0, j = n - 1; i < n; j = i++) {
        const Eigen::Vector2d& a = pts[i];
        const Eigen::Vector2d& b = pts[j];
        if ((a.y() > o.y()) != (b.y() > o.y())) {
            double xint = a.x() + (o.y() - a.y()) * (b.x() - a.x()) / (b.y() - a.y());
            if (o.x() < xint) inside = !inside;
        }
    }
    return inside;
}

// Clip segment [p0,p1] to the disk {x : |x-o|<=r}. Returns false if no part is
// inside. q0,q1 hold the inside portion on success.
bool clipSegToCircle(const Eigen::Vector2d& p0, const Eigen::Vector2d& p1,
                     const Eigen::Vector2d& o, double r,
                     Eigen::Vector2d& q0, Eigen::Vector2d& q1) {
    Eigen::Vector2d dir = p1 - p0;
    double A = dir.squaredNorm();
    if (A < 1e-18) {  // degenerate point
        if ((p0 - o).norm() <= r) { q0 = q1 = p0; return true; }
        return false;
    }
    Eigen::Vector2d e = p0 - o;
    double B = 2.0 * e.dot(dir);
    double C = e.dot(e) - r * r;
    double disc = B * B - 4.0 * A * C;
    if (disc <= 0.0) return C <= 0.0;  // no crossing: all-in or all-out
    double sd = std::sqrt(disc);
    double t0 = (-B - sd) / (2.0 * A);
    double t1 = (-B + sd) / (2.0 * A);
    double lo = std::max(0.0, t0), hi = std::min(1.0, t1);
    if (lo >= hi) return false;
    q0 = p0 + lo * dir;
    q1 = p0 + hi * dir;
    return true;
}
}  // namespace

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

// Wu2018 §III-B (Eq 1-5): area inside the circle but outside the polygon. The
// over-cover error. Accumulate signed circular-segment areas (sector-triangle)
// per edge; sign by outward-normal . (endpoint-center). Center-outside case
// adds pi*r^2 + (signed complementary) per Eq 5.
double circleOutsideArea(const Circle2D& C, const Contour2D& P) {
    if (C.radius <= 0.0 || P.points.size() < 3) return 0.0;
    const Eigen::Vector2d& o = C.center;
    double r = C.radius;
    int n = static_cast<int>(P.points.size());

    Eigen::Vector2d cen = Eigen::Vector2d::Zero();
    for (const auto& p : P.points) cen += p;
    cen /= double(n);
    bool oInside = pointInPolygon(o, P.points);

    double Ain = 0.0;
    for (int i = 0; i < n; ++i) {
        const Eigen::Vector2d& p0 = P.points[i];
        const Eigen::Vector2d& p1 = P.points[(i + 1) % n];
        Eigen::Vector2d q0, q1;
        if (!clipSegToCircle(p0, p1, o, r, q0, q1)) continue;  // edge outside disk

        Eigen::Vector2d a = q0 - o, b = q1 - o;
        double na = a.norm(), nb = b.norm();
        if (na < 1e-15 || nb < 1e-15) continue;
        double cos_t = std::clamp(a.dot(b) / (na * nb), -1.0, 1.0);
        double theta = std::acos(cos_t);          // minor sector angle [0,pi]
        double Asec = 0.5 * r * r * theta;
        double Atri = 0.5 * std::abs(a.x() * b.y() - a.y() * b.x());
        double COEA = Asec - Atri;                // circular-segment area

        // outward normal (away from polygon centroid)
        Eigen::Vector2d edge = p1 - p0;
        Eigen::Vector2d nrm(-edge.y(), edge.x());
        Eigen::Vector2d mid = 0.5 * (p0 + p1);
        if (nrm.dot(mid - cen) < 0.0) nrm = -nrm;
        double sign = (nrm.dot(p0 - o) >= 0.0) ? 1.0 : -1.0;
        Ain += sign * COEA;
    }

    if (oInside) return Ain;            // Eq 4
    return M_PI * r * r + Ain;          // Eq 5 (Ain is the negative complementary)
}

std::vector<Circle2D> fitCirclesLloyd(const Contour2D&, double, int) { return {}; }  // P3

std::vector<Capsule> fitCapsulesByCrossSection(const Eigen::MatrixXd&, const Eigen::MatrixXi&,
                                               int, double, int, int) {
    return {};  // P4/P5
}

}  // namespace urdf_approx_geom
