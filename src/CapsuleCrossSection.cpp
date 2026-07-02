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
#include <map>
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

// Merge capsules that are collinear, end-to-end, and similar radius (a chain of
// section-pair capsules along a tube -> one long capsule). Coverage-safe: the
// merged capsule (max radius, full span) contains both.
std::vector<Capsule> mergeCollinearCapsules(std::vector<Capsule> caps) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < caps.size() && !changed; ++i) {
            for (size_t j = 0; j < caps.size(); ++j) {
                if (i == j) continue;
                Capsule& A = caps[i];
                Capsule& B = caps[j];
                double la = (A.p1 - A.p0).norm();
                double lb = (B.p1 - B.p0).norm();
                if (la < 1e-9 || lb < 1e-9) continue;            // skip degenerate
                if (std::abs(A.radius - B.radius) > 0.15 * std::max(A.radius, B.radius)) continue;
                double gap = (A.p1 - B.p0).norm();               // end-to-end adjacency
                if (gap > 0.3 * std::max(la, lb)) continue;
                Eigen::Vector3d axa = (A.p1 - A.p0) / la;
                Eigen::Vector3d axb = (B.p1 - B.p0) / lb;
                if (axa.dot(axb) < 0.3) continue;                // grossly non-collinear (L-joint)
                // orient B to follow A
                double merged_radius = std::max(A.radius, B.radius);
                Capsule merged{A.p0, B.p1, merged_radius};
                caps[i] = merged;
                caps.erase(caps.begin() + j);
                changed = true;
                break;
            }
        }
    }
    return caps;
}

struct CapsuleBinProfile {
    std::vector<double> bin_max;
    double median_nonzero = 0.0;
    double ratio = 0.0;
};

CapsuleBinProfile profileCapsuleBins(const Capsule& cap, const Eigen::MatrixXd& V, int bins = 10) {
    CapsuleBinProfile out;
    out.bin_max.assign(bins, 0.0);
    Eigen::Vector3d axis = cap.p1 - cap.p0;
    double denom = axis.squaredNorm();
    for (int i = 0; i < V.rows(); ++i) {
        Eigen::Vector3d p = V.row(i).transpose();
        double t = denom < 1e-12 ? 0.0 : std::clamp((p - cap.p0).dot(axis) / denom, 0.0, 1.0);
        int slot = std::min(bins - 1, std::max(0, static_cast<int>(t * bins)));
        out.bin_max[slot] = std::max(out.bin_max[slot], pointToSegmentDistance(p, cap.p0, cap.p1));
    }
    std::vector<double> nonzero;
    for (double v : out.bin_max)
        if (v > 1e-12) nonzero.push_back(v);
    if (!nonzero.empty()) {
        std::sort(nonzero.begin(), nonzero.end());
        out.median_nonzero = nonzero[nonzero.size() / 2];
        out.ratio = cap.radius / out.median_nonzero;
    }
    return out;
}

static Eigen::MatrixXd assignedVerticesForCapsule(const Eigen::MatrixXd& V,
                                                  const CapsuleVertexAssignment& assignment,
                                                  int capsule_index) {
    int count = 0;
    for (int idx : assignment.capsule_index)
        if (idx == capsule_index) ++count;
    Eigen::MatrixXd out(count, 3);
    int row = 0;
    for (int i = 0; i < V.rows(); ++i) {
        if (assignment.capsule_index[i] == capsule_index) out.row(row++) = V.row(i);
    }
    return out;
}

static void resizeCapsulesFromAssignedVertices(std::vector<Capsule>& caps,
                                               const Eigen::MatrixXd& V) {
    if (caps.empty() || V.rows() == 0) return;
    auto assignment = assignVerticesToCapsules(V, caps);
    std::vector<double> new_radius(caps.size(), 0.0);
    for (int i = 0; i < V.rows(); ++i) {
        int c = assignment.capsule_index[i];
        if (c < 0) continue;
        new_radius[c] = std::max(new_radius[c], assignment.raw_distance[i]);
    }
    for (int c = 0; c < static_cast<int>(caps.size()); ++c) {
        caps[c].radius = std::max(caps[c].radius, new_radius[c]);
    }
}

bool splitMostInflatedCapsule(std::vector<Capsule>& caps,
                              const Eigen::MatrixXd& V,
                              double max_ratio,
                              int max_capsules) {
    if (static_cast<int>(caps.size()) >= max_capsules) return false;

    auto before_metrics = evaluateCapsuleTightness(V, caps);
    auto assignment = assignVerticesToCapsules(V, caps);

    int worst_index = -1;
    CapsuleBinProfile worst_profile;
    double worst_ratio = max_ratio;
    for (int i = 0; i < static_cast<int>(caps.size()); ++i) {
        if ((caps[i].p1 - caps[i].p0).norm() < 1e-9) continue;
        Eigen::MatrixXd local = assignedVerticesForCapsule(V, assignment, i);
        if (local.rows() == 0) continue;
        auto profile = profileCapsuleBins(caps[i], local);
        if (profile.ratio > worst_ratio) {
            worst_ratio = profile.ratio;
            worst_profile = profile;
            worst_index = i;
        }
    }
    if (worst_index < 0) return false;

    Capsule original = caps[worst_index];
    int split_bin = 5;
    double best_drop = -1.0;
    for (int i = 1; i < static_cast<int>(worst_profile.bin_max.size()); ++i) {
        double left = worst_profile.bin_max[i - 1];
        double right = worst_profile.bin_max[i];
        double drop = original.radius - std::max(left, right);
        if (drop > best_drop) {
            best_drop = drop;
            split_bin = i;
        }
    }

    double t = std::clamp(split_bin / double(worst_profile.bin_max.size()), 0.15, 0.85);
    Eigen::Vector3d mid = original.p0 + t * (original.p1 - original.p0);
    Capsule left{original.p0, mid, 0.0};
    Capsule right{mid, original.p1, 0.0};

    std::vector<Capsule> candidate = caps;
    candidate[worst_index] = left;
    candidate.push_back(right);
    resizeCapsulesFromAssignedVertices(candidate, V);
    growCapsulesToCover(candidate, V);
    candidate = dedupeNestedCapsules(candidate);

    auto after_metrics = evaluateCapsuleTightness(V, candidate);
    if (!after_metrics.covered) return false;
    bool improves_volume = after_metrics.capsule_volume < before_metrics.capsule_volume * 0.995;
    bool improves_ratio = after_metrics.max_radius_bin_ratio < before_metrics.max_radius_bin_ratio;
    if (!improves_volume && !improves_ratio) return false;

    caps = std::move(candidate);
    return true;
}

bool allCovered(const std::vector<Capsule>& caps, const Eigen::MatrixXd& V, double eps = 1e-9) {
    if (caps.empty()) return V.rows() == 0;
    for (int i = 0; i < V.rows(); ++i) {
        Eigen::Vector3d p = V.row(i).transpose();
        bool covered = false;
        for (const auto& cap : caps) {
            if (pointToSegmentDistance(p, cap.p0, cap.p1) <= cap.radius + eps) {
                covered = true;
                break;
            }
        }
        if (!covered) return false;
    }
    return true;
}

double capsuleVolume(const Capsule& cap) {
    double L = (cap.p1 - cap.p0).norm();
    return M_PI * cap.radius * cap.radius * L + 4.0 * M_PI * cap.radius * cap.radius * cap.radius / 3.0;
}

double capsuleSetVolume(const std::vector<Capsule>& caps) {
    double total = 0.0;
    for (const auto& cap : caps) total += capsuleVolume(cap);
    return total;
}

}  // namespace

CapsuleVertexAssignment assignVerticesToCapsules(const Eigen::MatrixXd& V,
                                                 const std::vector<Capsule>& caps) {
    CapsuleVertexAssignment out;
    out.capsule_index.assign(V.rows(), -1);
    out.raw_distance.assign(V.rows(), std::numeric_limits<double>::max());
    out.signed_distance.assign(V.rows(), std::numeric_limits<double>::max());
    for (int i = 0; i < V.rows(); ++i) {
        Eigen::Vector3d p = V.row(i).transpose();
        for (int c = 0; c < static_cast<int>(caps.size()); ++c) {
            double raw = pointToSegmentDistance(p, caps[c].p0, caps[c].p1);
            double signed_dist = raw - caps[c].radius;
            if (signed_dist < out.signed_distance[i]) {
                out.signed_distance[i] = signed_dist;
                out.raw_distance[i] = raw;
                out.capsule_index[i] = c;
            }
        }
    }
    return out;
}

static double aabbVolume(const Eigen::MatrixXd& V) {
    if (V.rows() == 0) return 0.0;
    Eigen::Vector3d lo = V.row(0).transpose();
    Eigen::Vector3d hi = lo;
    for (int i = 1; i < V.rows(); ++i) {
        Eigen::Vector3d p = V.row(i).transpose();
        lo = lo.cwiseMin(p);
        hi = hi.cwiseMax(p);
    }
    Eigen::Vector3d ext = (hi - lo).cwiseMax(Eigen::Vector3d::Constant(1e-12));
    return ext.x() * ext.y() * ext.z();
}

static double assignedRadiusBinRatio(const Eigen::MatrixXd& V,
                                     const std::vector<Capsule>& caps,
                                     const CapsuleVertexAssignment& assignment,
                                     int bins = 10) {
    double worst = 0.0;
    for (int c = 0; c < static_cast<int>(caps.size()); ++c) {
        std::vector<double> bin_max(bins, 0.0);
        Eigen::Vector3d axis = caps[c].p1 - caps[c].p0;
        double denom = axis.squaredNorm();
        for (int i = 0; i < V.rows(); ++i) {
            if (assignment.capsule_index[i] != c) continue;
            Eigen::Vector3d p = V.row(i).transpose();
            double t = denom < 1e-12 ? 0.0 : std::clamp((p - caps[c].p0).dot(axis) / denom, 0.0, 1.0);
            int slot = std::min(bins - 1, std::max(0, static_cast<int>(t * bins)));
            bin_max[slot] = std::max(bin_max[slot], pointToSegmentDistance(p, caps[c].p0, caps[c].p1));
        }
        std::vector<double> nonzero;
        for (double v : bin_max)
            if (v > 1e-12) nonzero.push_back(v);
        if (nonzero.empty()) continue;
        std::sort(nonzero.begin(), nonzero.end());
        double median = nonzero[nonzero.size() / 2];
        worst = std::max(worst, caps[c].radius / median);
    }
    return worst;
}

CapsuleTightnessMetrics evaluateCapsuleTightness(const Eigen::MatrixXd& V,
                                                 const std::vector<Capsule>& caps) {
    CapsuleTightnessMetrics out;
    if (V.rows() == 0 || caps.empty()) {
        out.covered = V.rows() == 0;
        return out;
    }
    auto assignment = assignVerticesToCapsules(V, caps);
    out.worst_signed_distance = *std::max_element(assignment.signed_distance.begin(),
                                                  assignment.signed_distance.end());
    out.covered = out.worst_signed_distance <= 1e-9;
    out.capsule_volume = capsuleSetVolume(caps);
    out.aabb_volume = aabbVolume(V);
    out.capV_aabb = out.aabb_volume > 1e-12 ? out.capsule_volume / out.aabb_volume : 0.0;
    out.max_radius_bin_ratio = assignedRadiusBinRatio(V, caps, assignment);
    return out;
}

namespace {
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

double contourAreaAbs(const Contour2D& c) {
    if (c.points.size() < 3) return 0.0;
    double area = 0.0;
    int n = static_cast<int>(c.points.size());
    for (int i = 0; i < n; ++i) {
        const auto& a = c.points[i];
        const auto& b = c.points[(i + 1) % n];
        area += a.x() * b.y() - b.x() * a.y();
    }
    return 0.5 * std::abs(area);
}
}  // namespace

// Wu2018 §III-A: slice the mesh with planes perpendicular to axis u, chain the
// triangle-plane intersection segments into closed 2D contours.
std::vector<Section2D> extractSections(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                       const Eigen::Vector3d& u, const Eigen::Vector3d& origin,
                                       int N) {
    std::vector<Section2D> out;
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
    auto chain = [&](std::vector<std::pair<Eigen::Vector2d, Eigen::Vector2d>>& segs,
                     std::vector<Contour2D>& dest) {
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
            dest.push_back(std::move(c));
        }
    };

    auto tk_at = [&](int k) {
        // midpoint spacing (robust: endpoint planes degenerate on flat caps).
        return tmin + (k + 0.5) * (tmax - tmin) / N;
    };
    for (int k = 0; k < N; ++k) {
        double tk = tk_at(k);
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
        std::vector<Contour2D> cs;
        chain(segs, cs);
        for (auto& c : cs) out.push_back({std::move(c), tk});
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

std::vector<Circle2D> fitFixedCountCirclesForPlane(const std::vector<Contour2D>& contours,
                                                   int k) {
    std::vector<Eigen::Vector2d> pts;
    for (const auto& contour : contours) {
        auto sampled = sampleContour(contour);
        pts.insert(pts.end(), sampled.begin(), sampled.end());
    }
    std::vector<Circle2D> circles;
    if (pts.empty() || k <= 0) return circles;
    if (k == 1 || static_cast<int>(pts.size()) < k) {
        circles.push_back(mec2d(pts));
        return circles;
    }

    std::vector<Eigen::Vector2d> seeds;
    seeds.reserve(k);
    seeds.push_back(pts.front());
    // ponytail: deterministic farthest-point seeding (not probabilistic k-means++); reproducibility over statistical optimality.
    while (static_cast<int>(seeds.size()) < k) {
        int best = 0;
        double best_dist = -1.0;
        for (int i = 0; i < static_cast<int>(pts.size()); ++i) {
            double nearest = std::numeric_limits<double>::max();
            for (const auto& seed : seeds) {
                nearest = std::min(nearest, (pts[i] - seed).squaredNorm());
            }
            if (nearest > best_dist) {
                best_dist = nearest;
                best = i;
            }
        }
        seeds.push_back(pts[best]);
    }

    std::vector<std::vector<Eigen::Vector2d>> groups(k);
    constexpr int max_iters = 100;
    constexpr double convergence_threshold = 1e-9;
    for (int iter = 0; iter < max_iters; ++iter) {
        for (auto& group : groups) group.clear();
        for (const auto& p : pts) {
            int best = 0;
            double best_dist = std::numeric_limits<double>::max();
            for (int i = 0; i < k; ++i) {
                double d = (p - seeds[i]).squaredNorm();
                if (d < best_dist) {
                    best_dist = d;
                    best = i;
                }
            }
            groups[best].push_back(p);
        }
        double max_movement = 0.0;
        for (int i = 0; i < k; ++i) {
            if (groups[i].empty()) continue;
            Eigen::Vector2d mean = Eigen::Vector2d::Zero();
            for (const auto& p : groups[i]) mean += p;
            Eigen::Vector2d new_seed = mean / double(groups[i].size());
            double movement = (new_seed - seeds[i]).norm();
            max_movement = std::max(max_movement, movement);
            seeds[i] = new_seed;
        }
        if (max_movement < convergence_threshold) break;
    }

    for (int i = 0; i < k; ++i) {
        if (!groups[i].empty()) circles.push_back(mec2d(groups[i]));
    }
    // ponytail: pad empty clusters with last circle; grow step corrects overlap.
    while (static_cast<int>(circles.size()) < k && !circles.empty()) {
        circles.push_back(circles.back());
    }
    return circles;
}

double normalizedPlaneOutsideArea(const std::vector<Circle2D>& circles,
                                  const std::vector<Contour2D>& contours) {
    double outside = 0.0;
    double area = 0.0;
    for (const auto& contour : contours) {
        area += contourAreaAbs(contour);
        for (const auto& circle : circles) {
            outside += circleOutsideArea(circle, contour);
        }
    }
    if (area < 1e-12) return outside;
    return outside / area;
}

double assignedPlaneCircleScore(const std::vector<Contour2D>& contours,
                                const std::vector<Circle2D>& circles) {
    std::vector<Eigen::Vector2d> pts;
    for (const auto& contour : contours) {
        auto sampled = sampleContour(contour);
        pts.insert(pts.end(), sampled.begin(), sampled.end());
    }
    if (pts.empty() || circles.empty()) return std::numeric_limits<double>::infinity();

    double max_assigned_radius = 0.0;
    double mean_positive_slack = 0.0;
    for (const auto& p : pts) {
        double best_signed = std::numeric_limits<double>::max();
        double best_raw = std::numeric_limits<double>::max();
        for (const auto& circle : circles) {
            double raw = (p - circle.center).norm();
            double signed_dist = raw - circle.radius;
            if (signed_dist < best_signed) {
                best_signed = signed_dist;
                best_raw = raw;
            }
        }
        max_assigned_radius = std::max(max_assigned_radius, best_raw);
        mean_positive_slack += std::max(0.0, -best_signed);
    }
    mean_positive_slack /= double(pts.size());
    return max_assigned_radius + 0.05 * mean_positive_slack;
}

std::vector<Circle2D> fitAdaptiveCirclesForPlane(const std::vector<Contour2D>& contours,
                                                 double coa_threshold,
                                                 int max_circles) {
    int cap = std::max(1, max_circles);
    if (coa_threshold <= 0.0) return fitFixedCountCirclesForPlane(contours, 1);

    std::vector<Circle2D> best = fitFixedCountCirclesForPlane(contours, 1);
    double best_score = assignedPlaneCircleScore(contours, best);
    if (best_score <= coa_threshold) return best;

    for (int k = 2; k <= cap; ++k) {
        auto candidate = fitFixedCountCirclesForPlane(contours, k);
        double score = assignedPlaneCircleScore(contours, candidate);
        double relative_improvement = (best_score - score) / std::max(best_score, 1e-12);
        if (score < best_score) {
            best = candidate;
            best_score = score;
        }
        if (relative_improvement < coa_threshold) break;
    }
    return best;
}

// Wu2018 §III-C..E: section circles -> capsules -> dedupe -> grow to cover.
// Capsule endpoints sit at circle centers (axial t of consecutive sections),
// radius = max of the two end radii (constant-radius for <cylinder>+<sphere>).
std::vector<Capsule> fitCapsulesByCrossSection(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                               int n_sections, double coa_threshold,
                                               int max_circles_per_section, int max_capsules) {
    CapsuleFitOptions options;
    options.n_sections = n_sections;
    options.coa_threshold = coa_threshold;
    options.max_circles_per_section = max_circles_per_section;
    options.max_capsules = max_capsules;
    // Preserve existing fixed-count behavior for backward compatibility.
    // When max_circles_per_section > 1 the adaptive path is needed so that
    // per-section circles can actually capture multiple cross-section lobes;
    // with max_circles_per_section == 1 there is nothing to adapt.
    options.adaptive_circle_count = max_circles_per_section > 1;
    options.max_radius_bin_ratio = -1;  // disable split for backward compatibility
    return fitCapsulesByCrossSection(V, F, options);
}

std::vector<Capsule> fitCapsulesByCrossSection(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                               const CapsuleFitOptions& options) {
    std::vector<Capsule> caps;
    if (V.rows() < 4 || F.rows() == 0 || options.n_sections < 1) return caps;

    // PCA axis (longest dimension = "bone direction").
    Eigen::Vector3d origin = V.colwise().mean();
    Eigen::MatrixXd Cn = V.rowwise() - origin.transpose();
    Eigen::Matrix3d cov = (Cn.transpose() * Cn) / double(V.rows());
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es(cov);
    Eigen::Vector3d u = es.eigenvectors().col(2);
    Eigen::Vector3d ref = std::abs(u.x()) < 0.9 ? Eigen::Vector3d::UnitX() : Eigen::Vector3d::UnitY();
    Eigen::Vector3d e1 = u.cross(ref).normalized();
    Eigen::Vector3d e2 = u.cross(e1).normalized();

    auto rawSections = extractSections(V, F, u, origin, options.n_sections);
    if (rawSections.empty()) return caps;

    std::map<double, std::vector<Contour2D>> planes;
    for (auto& s : rawSections) planes[s.t].push_back(std::move(s.contour));
    std::vector<double> planeT;
    std::vector<std::vector<Circle2D>> planeCircles;

    for (auto& kv : planes) {
        auto circles = options.adaptive_circle_count
            ? fitAdaptiveCirclesForPlane(kv.second, options.coa_threshold, options.max_circles_per_section)
            : fitFixedCountCirclesForPlane(kv.second, std::max(1, options.max_circles_per_section));
        if (circles.empty()) continue;
        planeT.push_back(kv.first);
        planeCircles.push_back(std::move(circles));
    }
    int N = static_cast<int>(planeT.size());
    if (N == 0) return caps;

    auto to3d = [&](const Circle2D& c, double t) {
        return origin + t * u + c.center.x() * e1 + c.center.y() * e2;
    };

    double amin = std::numeric_limits<double>::max();
    double amax = std::numeric_limits<double>::lowest();
    for (int i = 0; i < V.rows(); ++i) {
        double t = (V.row(i).transpose() - origin).dot(u);
        amin = std::min(amin, t);
        amax = std::max(amax, t);
    }

    auto emit_pair = [&](const Circle2D& a, double ta, const Circle2D& b, double tb) {
        Capsule cap;
        cap.p0 = to3d(a, ta);
        cap.p1 = to3d(b, tb);
        cap.radius = std::max(a.radius, b.radius);
        caps.push_back(cap);
    };

    auto emit_degenerate = [&](const Circle2D& c, double t) {
        Capsule cap;
        cap.p0 = to3d(c, t);
        cap.p1 = cap.p0;
        cap.radius = c.radius;
        caps.push_back(cap);
    };

    struct ActiveChain {
        Circle2D circle;
        double t = 0.0;
        bool matched = false;
    };

    if (N == 1) {
        for (const auto& circle : planeCircles[0]) emit_degenerate(circle, planeT[0]);
    } else {
        std::vector<ActiveChain> active;
        for (const auto& c : planeCircles.front()) active.push_back({c, planeT.front(), false});

        for (int section = 1; section < N; ++section) {
            for (auto& chain : active) chain.matched = false;
            std::vector<char> used(planeCircles[section].size(), 0);

            for (auto& chain : active) {
                int best = -1;
                double best_dist = std::numeric_limits<double>::max();
                for (int j = 0; j < static_cast<int>(planeCircles[section].size()); ++j) {
                    if (used[j]) continue;
                    double d = (chain.circle.center - planeCircles[section][j].center).squaredNorm();
                    if (d < best_dist) {
                        best_dist = d;
                        best = j;
                    }
                }
                if (best >= 0) {
                    emit_pair(chain.circle, chain.t, planeCircles[section][best], planeT[section]);
                    chain.circle = planeCircles[section][best];
                    chain.t = planeT[section];
                    chain.matched = true;
                    used[best] = 1;
                }
            }

            std::vector<ActiveChain> next_active;
            for (const auto& chain : active) {
                if (chain.matched) next_active.push_back(chain);
            }
            for (int j = 0; j < static_cast<int>(planeCircles[section].size()); ++j) {
                if (!used[j]) next_active.push_back({planeCircles[section][j], planeT[section], true});
            }
            active = std::move(next_active);
        }
    }

    double t0 = planeT.front(), tN = planeT.back();
    for (auto& cap : caps) {
        double a0 = (cap.p0 - origin).dot(u);
        double a1 = (cap.p1 - origin).dot(u);
        if (std::abs(a0 - t0) < 1e-9) cap.p0 += (amin - t0) * u;
        if (std::abs(a1 - t0) < 1e-9) cap.p1 += (amin - t0) * u;
        if (std::abs(a0 - tN) < 1e-9) cap.p0 += (amax - tN) * u;
        if (std::abs(a1 - tN) < 1e-9) cap.p1 += (amax - tN) * u;
    }

    caps = mergeCollinearCapsules(caps);
    growCapsulesToCover(caps, V);
    if (options.max_radius_bin_ratio > 0) {
        while (splitMostInflatedCapsule(caps, V, options.max_radius_bin_ratio, options.max_capsules)) {
        }
        growCapsulesToCover(caps, V);
    }
    caps = dedupeNestedCapsules(caps);

    while (static_cast<int>(caps.size()) > options.max_capsules && caps.size() > 1) {
        int best_remove = -1;
        double best_score = std::numeric_limits<double>::max();
        std::vector<Capsule> best_candidate;

        for (int i = 0; i < static_cast<int>(caps.size()); ++i) {
            std::vector<Capsule> candidate = caps;
            candidate.erase(candidate.begin() + i);
            growCapsulesToCover(candidate, V);
            candidate = dedupeNestedCapsules(candidate);
            if (!allCovered(candidate, V)) continue;
            double score = capsuleSetVolume(candidate);
            if (score < best_score) {
                best_score = score;
                best_remove = i;
                best_candidate = std::move(candidate);
            }
        }

        if (best_remove < 0) break;
        caps = std::move(best_candidate);
    }
    growCapsulesToCover(caps, V);
    caps = dedupeNestedCapsules(caps);
    return caps;
}

void growCapsulesToCover(std::vector<Capsule>& caps, const Eigen::MatrixXd& V) {
    if (caps.empty() || V.rows() == 0) return;
    for (int i = 0; i < V.rows(); ++i) {
        Eigen::Vector3d p = V.row(i).transpose();
        int best = -1;
        double bd = std::numeric_limits<double>::max();
        for (int c = 0; c < static_cast<int>(caps.size()); ++c) {
            double d = pointToSegmentDistance(p, caps[c].p0, caps[c].p1);
            if (d < bd) { bd = d; best = c; }
        }
        if (best >= 0 && bd > caps[best].radius) caps[best].radius = bd;
    }
}

}  // namespace urdf_approx_geom
