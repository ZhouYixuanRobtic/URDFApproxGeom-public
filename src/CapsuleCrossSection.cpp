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
#include <random>
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

// Compact 2D minimum enclosing circle (Welzl move-to-front), fixed seed.
Circle2D mec2d(std::vector<Eigen::Vector2d> P) {
    std::mt19937 rng(0xC0FFEE);
    std::shuffle(P.begin(), P.end(), rng);
    Circle2D C{Eigen::Vector2d::Zero(), -1.0};
    auto from2 = [](const Eigen::Vector2d& a, const Eigen::Vector2d& b) {
        return Circle2D{0.5 * (a + b), 0.5 * (a - b).norm()};
    };
    auto from3 = [&](const Eigen::Vector2d& a, const Eigen::Vector2d& b, const Eigen::Vector2d& c) {
        double ax = a.x(), ay = a.y(), bx = b.x(), by = b.y(), cx = c.x(), cy = c.y();
        double d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
        if (std::abs(d) < 1e-18) {
            double ab = (a - b).squaredNorm(), ac = (a - c).squaredNorm(), bc = (b - c).squaredNorm();
            return (ab >= ac && ab >= bc) ? from2(a, b) : (ac >= bc ? from2(a, c) : from2(b, c));
        }
        double ux = ((ax * ax + ay * ay) * (by - cy) + (bx * bx + by * by) * (cy - ay) +
                     (cx * cx + cy * cy) * (ay - by)) / d;
        double uy = ((ax * ax + ay * ay) * (cx - bx) + (bx * bx + by * by) * (ax - cx) +
                     (cx * cx + cy * cy) * (bx - ax)) / d;
        Eigen::Vector2d ctr(ux, uy);
        return Circle2D{ctr, (a - ctr).norm()};
    };
    for (size_t i = 0; i < P.size(); ++i) {
        if (C.radius < 0.0 || (P[i] - C.center).norm() > C.radius + 1e-12) {
            C = {P[i], 0.0};
            for (size_t j = 0; j < i; ++j)
                if ((P[j] - C.center).norm() > C.radius + 1e-12) {
                    C = from2(P[i], P[j]);
                    for (size_t k = 0; k < j; ++k)
                        if ((P[k] - C.center).norm() > C.radius + 1e-12) C = from3(P[i], P[j], P[k]);
                }
        }
    }
    if (C.radius < 0.0) C = {Eigen::Vector2d::Zero(), 0.0};
    return C;
}

// Sample a polygon: subdivided boundary edges + interior grid points.
std::vector<Eigen::Vector2d> sampleContour(const Contour2D& c, int edgeN = 24, int grid = 24) {
    std::vector<Eigen::Vector2d> pts;
    int n = static_cast<int>(c.points.size());
    Eigen::Vector2d lo = c.points[0], hi = c.points[0];
    for (const auto& p : c.points) {
        lo = lo.cwiseMin(p);
        hi = hi.cwiseMax(p);
    }
    for (int i = 0; i < n; ++i) {
        const Eigen::Vector2d& p0 = c.points[i];
        const Eigen::Vector2d& p1 = c.points[(i + 1) % n];
        for (int s = 0; s < edgeN; ++s) pts.push_back(p0 + (double(s) / edgeN) * (p1 - p0));
    }
    for (int gx = 1; gx < grid; ++gx)
        for (int gy = 1; gy < grid; ++gy) {
            Eigen::Vector2d p(lo.x() + (hi.x() - lo.x()) * gx / grid,
                              lo.y() + (hi.y() - lo.y()) * gy / grid);
            if (pointInPolygon(p, c.points)) pts.push_back(p);
        }
    return pts;
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

// Wu2018 §III-B: adaptive Lloyd circle clustering. Start from one MEC of the
// sampled contour; while total COA exceeds the threshold (and budget remains),
// split the worst-COA circle into two via k-means(2) on its assigned points and
// refit each (teleportation). Returns circles covering the contour.
std::vector<Circle2D> fitCirclesLloyd(const Contour2D& contour, double coa_threshold,
                                      int max_circles) {
    std::vector<Circle2D> circles;
    if (contour.points.size() < 3 || max_circles < 1) return circles;
    auto pts = sampleContour(contour);
    if (pts.empty()) return circles;

    auto totalCOA = [&]() {
        double s = 0.0;
        for (const auto& c : circles) s += circleOutsideArea(c, contour);
        return s;
    };
    auto nearest = [&](const Eigen::Vector2d& p) {
        int b = 0;
        double bd = std::numeric_limits<double>::max();
        for (int i = 0; i < static_cast<int>(circles.size()); ++i) {
            double d = (circles[i].center - p).squaredNorm();
            if (d < bd) { bd = d; b = i; }
        }
        return b;
    };
    auto kmeans2 = [&](const std::vector<Eigen::Vector2d>& sub,
                       std::vector<Eigen::Vector2d>& g0, std::vector<Eigen::Vector2d>& g1) {
        Eigen::Vector2d s0 = sub.front();
        Eigen::Vector2d s1 = sub.front();
        double best = -1.0;
        for (const auto& p : sub) {
            double d = (p - s0).squaredNorm();
            if (d > best) { best = d; s1 = p; }
        }
        for (int it = 0; it < 6; ++it) {
            g0.clear(); g1.clear();
            for (const auto& p : sub)
                ((p - s0).squaredNorm() <= (p - s1).squaredNorm() ? g0 : g1).push_back(p);
            if (g0.empty() || g1.empty()) return;
            Eigen::Vector2d m0 = Eigen::Vector2d::Zero(), m1 = Eigen::Vector2d::Zero();
            for (const auto& p : g0) m0 += p;
            for (const auto& p : g1) m1 += p;
            s0 = m0 / double(g0.size());
            s1 = m1 / double(g1.size());
        }
    };

    circles.push_back(mec2d(pts));
    int guard = 0;
    while (totalCOA() > coa_threshold && static_cast<int>(circles.size()) < max_circles &&
           guard++ < 8 * max_circles) {
        int worst = 0;
        double wc = -1.0;
        for (int i = 0; i < static_cast<int>(circles.size()); ++i) {
            double a = circleOutsideArea(circles[i], contour);
            if (a > wc) { wc = a; worst = i; }
        }
        std::vector<Eigen::Vector2d> sub;
        for (const auto& p : pts)
            if (nearest(p) == worst) sub.push_back(p);
        if (sub.size() < 4) break;
        std::vector<Eigen::Vector2d> g0, g1;
        kmeans2(sub, g0, g1);
        if (g0.empty() || g1.empty()) break;
        circles.erase(circles.begin() + worst);
        circles.push_back(mec2d(g0));
        circles.push_back(mec2d(g1));
    }
    return circles;
}

std::vector<Capsule> fitCapsulesByCrossSection(const Eigen::MatrixXd&, const Eigen::MatrixXi&,
                                               int, double, int, int) {
    return {};  // P4/P5
}

}  // namespace urdf_approx_geom
