/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#ifndef URDFAPPROXGEOM_SPHERETREEBASE_H
#define URDFAPPROXGEOM_SPHERETREEBASE_H

#include <SphereTree/SphereTree.h>
#include <Eigen/Dense>
#include <memory>
#include "Surface/Surface.h"
#include "irmv/bot_common/state/error_code.h"

namespace SphereTreeMethod {

enum Optimiser { NONE, SIMPLEX, BALANCE };

class Sphere {
  public:
    Sphere() = default;

    Sphere(double x, double y, double z, double r);

    ~Sphere() = default;

  protected:
    Eigen::Vector4d data;

  public:
    const Eigen::Vector4d& getData() const;

    const double& X() const;

    const double& Y() const;

    const double& Z() const;

    const double& R() const;

    void setByRaw(double x, double y, double z, double r);
};

class MySphereTree {
  public:
    MySphereTree() = default;

    MySphereTree(const SphereTree& tree, double scale);

    ~MySphereTree() = default;

  public:
    ulong levels, degree;
    Sphere biggest_sphere;
    std::vector<Sphere> sub_spheres;

  public:
    void setBySphereTree(const SphereTree& tree, double scale);
};

enum STMethodType { Grid, Hubbard, Medial, Octree, Spawn };

class SphereTreeMethodBase {
  public:
    SphereTreeMethodBase() = default;

    virtual ~SphereTreeMethodBase() = default;

  public:
    virtual irmv_core::bot_common::ErrorInfo constructTree(const std::string& file,
                                                           MySphereTree& tree);

    virtual irmv_core::bot_common::ErrorInfo constructTree(const Eigen::MatrixXd& V,
                                                           const Eigen::MatrixXi& F,
                                                           MySphereTree& tree);

    virtual irmv_core::bot_common::ErrorInfo constructTree(Surface& sur, MySphereTree& tree) = 0;

    const std::string& getMethodName();

    void setBranch(int branch);

  protected:
    std::string m_method_name;

    static void loadOBJFromEigen(Surface* sur, const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                                 float boxSize = -1);

    int branch = 8;  ///<  branching factor of the sphere-tree
};

typedef std::shared_ptr<SphereTreeMethodBase> SphereTreePtr;
typedef std::unique_ptr<SphereTreeMethodBase> SphereTreeUniquePtr;
}  // namespace SphereTreeMethod

#endif  // URDFAPPROXGEOM_SPHERETREEBASE_H
