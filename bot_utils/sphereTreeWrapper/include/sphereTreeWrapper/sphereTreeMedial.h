/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#ifndef URDFAPPROXGEOM_SPHERETREEMEDIAL_HPP
#define URDFAPPROXGEOM_SPHERETREEMEDIAL_HPP

#include "irmv/bot_common/alg_factory/algorithm_factory.h"
#include "sphereTreeBase.h"

namespace SphereTreeMethod {
constexpr char SphereTreeMethodMedialName[] = "SphereTreeMedialName";

class SphereTreeMethodMedial : public SphereTreeMethodBase {
  public:
    SphereTreeMethodMedial(const std::string& config_path);

    ~SphereTreeMethodMedial() override = default;

    static SphereTreeUniquePtr create(const std::string& config_path);

    irmv_core::bot_common::ErrorInfo constructTree(Surface& sur, MySphereTree& tree) override;

  protected:
    int testerLevels = -1;     ///<  number of levels for NON-CONVEX, -1 uses CONVEX tester
    int depth = 3;             ///<  depth of the sphere-tree
    int numCoverPts = 5000;    ///<  number of test points to put on surface for coverage
    int minCoverPts = 5;       ///<  minimum number of points per triangle for coverage
    int initSpheres = 500;     ///<  initial spheres in Voronoi
    float erFact = 2;          ///<  error reduction factor for adaptive Voronoi
    int spheresPerNode = 100;  ///<  minimum number of spheres per node
    bool verify = false;       ///<  verify model before construction
    bool nopause = false;      ///<  will we pause before starting
    bool eval = false;         ///<  do we evaluate the sphere-tree after construction
    bool useMerge = false;     ///<  do we include the MERGE algorithm
    bool useBurst = false;     ///<  do we include the BURST algorithm
    bool useExpand = false;    ///<  do we include the EXPAND algorithm

    int optimise = Optimiser::NONE;  ///<  which optimiser should we use
    float balExcess = 0.0;           ///<  % increase error allowed for BALANCE opt algorithm
    int maxOptLevel =
        -1;  ///<  maximum sphere-tree level to apply optimiser (0 does first set only)
};

inline irmv_core::bot_common::REGISTER_ALGORITHM(SphereTreeMethodBase, SphereTreeMethodMedialName,
                                                 SphereTreeMethodMedial, const std::string&);
}  // namespace SphereTreeMethod

#endif  // URDFAPPROXGEOM_SPHERETREEMEDIAL_HPP
