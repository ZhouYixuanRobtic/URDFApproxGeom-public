/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#ifndef URDFAPPROXGEOM_SPHERETREESPAWN_H
#define URDFAPPROXGEOM_SPHERETREESPAWN_H

#include "irmv/bot_common/alg_factory/algorithm_factory.h"
#include "sphereTreeBase.h"

namespace SphereTreeMethod {
constexpr char SphereTreeMethodSpawnName[] = "SphereTreeSpawnName";

class SphereTreeMethodSpawn : public SphereTreeMethodBase {
  public:
    SphereTreeMethodSpawn(const std::string& config_path);

    ~SphereTreeMethodSpawn() override = default;

    static SphereTreeUniquePtr create(const std::string& config_path);

    irmv_core::bot_common::ErrorInfo constructTree(Surface& sur, MySphereTree& tree) override;

  protected:
    int testerLevels = -1;   ///<  number of levels for NON-CONVEX, -1 uses CONVEX tester
    int depth = 3;           ///<  depth of the sphere-tree
    int numCoverPts = 5000;  ///<  number of test points to put on surface for coverage
    int minCoverPts = 5;     ///<  minimum number of points per triangle for coverage
    bool verify = false;     ///<  verify model before construction
    bool nopause = false;    ///<  will we pause before starting
    bool eval = false;       ///<  do we evaluate the sphere-tree after construction
};

inline irmv_core::bot_common::REGISTER_ALGORITHM(SphereTreeMethodBase, SphereTreeMethodSpawnName,
                                                 SphereTreeMethodSpawn, const std::string&);
}  // namespace SphereTreeMethod

#endif  // URDFAPPROXGEOM_SPHERETREESPAWN_H
