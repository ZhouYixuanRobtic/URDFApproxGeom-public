/*
 * quickhull.hpp
 *
 * Minimal self-contained 3D QuickHull-style convex hull implementation.
 * Vendored for both research and commercial editions; no CGAL/GMP/sphere_tree
 * dependency. See src/ConvexHullBackend.cpp for the thin public wrapper TU.
 */

#ifndef URDFAPPROXGEOM_QUICKHULL_HPP
#define URDFAPPROXGEOM_QUICKHULL_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include <Eigen/Dense>

namespace urdf_approx_geom {
namespace quickhull_detail {

using Vec3 = Eigen::Vector3d;

struct Face {
    int v[3];
};

double orientDist(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& p) {
    return (b - a).cross(c - a).dot(p - a);
}

void orientFace(Face& f, const std::vector<Vec3>& P, const Vec3& inside) {
    const Vec3& a = P[f.v[0]];
    const Vec3& b = P[f.v[1]];
    const Vec3& c = P[f.v[2]];
    if (orientDist(a, b, c, inside) > 0.0) {
        std::swap(f.v[1], f.v[2]);
    }
}

std::pair<int, int> edgeKey(int a, int b) {
    if (a > b) std::swap(a, b);
    return {a, b};
}

inline void computeConvexHull3DImpl(const Eigen::MatrixXd& V, Eigen::MatrixXd& HV,
                         Eigen::MatrixXi& HF) {
    HV.resize(0, 3);
    HF.resize(0, 3);
    if (V.rows() < 4) return;
    using namespace quickhull_detail;

    // Deduplicate input vertices.
    std::vector<Vec3> P;
    P.reserve(V.rows());
    double scale = 0.0;
    for (int i = 0; i < V.rows(); ++i) {
        Vec3 p = V.row(i);
        scale = std::max(scale, p.cwiseAbs().maxCoeff());
        P.push_back(p);
    }
    std::map<std::tuple<long long, long long, long long>, int> seen;
    std::vector<Vec3> uniqueP;
    uniqueP.reserve(P.size());
    auto rounded = [](double x) -> long long {
        return static_cast<long long>(std::llround(x * 1e12));
    };
    for (const Vec3& p : P) {
        auto key = std::make_tuple(rounded(p.x()), rounded(p.y()), rounded(p.z()));
        if (seen.find(key) != seen.end()) continue;
        seen.emplace(key, static_cast<int>(uniqueP.size()));
        uniqueP.push_back(p);
    }
    P.swap(uniqueP);
    const int n = static_cast<int>(P.size());
    if (n < 4) return;

    const double eps = 1e-10 * std::max(1.0, scale);

    // Pick two extreme points along the largest spread axis.
    int a = 0, b = 0;
    for (int i = 1; i < n; ++i) {
        if (P[i].x() < P[a].x()) a = i;
        if (P[i].x() > P[b].x()) b = i;
    }
    if ((P[b] - P[a]).norm() <= eps) {
        for (int i = 1; i < n; ++i) {
            if (P[i].y() < P[a].y()) a = i;
            if (P[i].y() > P[b].y()) b = i;
        }
    }
    if ((P[b] - P[a]).norm() <= eps) {
        for (int i = 1; i < n; ++i) {
            if (P[i].z() < P[a].z()) a = i;
            if (P[i].z() > P[b].z()) b = i;
        }
    }
    if ((P[b] - P[a]).norm() <= eps) return;

    // Find the point farthest from the ab line.
    const Vec3 ab = P[b] - P[a];
    const double ab2 = ab.squaredNorm();
    int c = -1;
    double bestDist = eps;
    for (int i = 0; i < n; ++i) {
        if (i == a || i == b) continue;
        const Vec3 ap = P[i] - P[a];
        double dist = (ap - ab * (ap.dot(ab) / ab2)).norm();
        if (dist > bestDist) {
            bestDist = dist;
            c = i;
        }
    }
    if (c < 0) return;

    // Find the point farthest from the abc plane.
    int d = -1;
    bestDist = eps;
    for (int i = 0; i < n; ++i) {
        if (i == a || i == b || i == c) continue;
        double dist = std::abs(orientDist(P[a], P[b], P[c], P[i]));
        if (dist > bestDist) {
            bestDist = dist;
            d = i;
        }
    }
    if (d < 0) return;

    const Vec3 inside = (P[a] + P[b] + P[c] + P[d]) / 4.0;
    std::vector<Face> faces;
    auto addTetFace = [&](int i, int j, int k) {
        Face f{{i, j, k}};
        orientFace(f, P, inside);
        faces.push_back(f);
    };
    addTetFace(a, b, c);
    addTetFace(a, c, d);
    addTetFace(a, d, b);
    addTetFace(b, d, c);

    // Incrementally add every outside point.
    for (int p = 0; p < n; ++p) {
        if (p == a || p == b || p == c || p == d) continue;

        std::vector<int> visible;
        visible.reserve(faces.size());
        for (int fi = 0; fi < static_cast<int>(faces.size()); ++fi) {
            const Face& f = faces[fi];
            if (orientDist(P[f.v[0]], P[f.v[1]], P[f.v[2]], P[p]) > eps) {
                visible.push_back(fi);
            }
        }
        if (visible.empty()) continue;

        std::map<std::pair<int, int>, int> edgeCount;
        for (int fi : visible) {
            const Face& f = faces[fi];
            for (int e = 0; e < 3; ++e) {
                int u = f.v[e];
                int v = f.v[(e + 1) % 3];
                auto key = edgeKey(u, v);
                ++edgeCount[key];
            }
        }

        std::vector<Face> next;
        next.reserve(faces.size() - visible.size() + edgeCount.size());
        std::vector<char> erased(faces.size(), 0);
        for (int fi : visible) erased[fi] = 1;
        for (int fi = 0; fi < static_cast<int>(faces.size()); ++fi) {
            if (!erased[fi]) next.push_back(faces[fi]);
        }

        for (const auto& kv : edgeCount) {
            if (kv.second != 1) continue;
            Face f{{kv.first.first, kv.first.second, p}};
            orientFace(f, P, inside);
            next.push_back(f);
        }
        faces.swap(next);
    }

    if (faces.empty()) return;

    // Compact the referenced vertices.
    std::map<int, int> remap;
    for (const Face& f : faces) {
        for (int v : f.v) {
            if (remap.find(v) == remap.end()) {
                int idx = static_cast<int>(remap.size());
                remap[v] = idx;
            }
        }
    }
    HV.resize(static_cast<int>(remap.size()), 3);
    for (const auto& kv : remap) HV.row(kv.second) = P[kv.first];

    HF.resize(static_cast<int>(faces.size()), 3);
    for (int i = 0; i < static_cast<int>(faces.size()); ++i) {
        for (int e = 0; e < 3; ++e) HF(i, e) = remap[faces[i].v[e]];
    }
}

}  // namespace quickhull_detail
}  // namespace urdf_approx_geom

#endif  // URDFAPPROXGEOM_QUICKHULL_HPP
