/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#ifndef URDFAPPROXGEOM_SPHERETREEOCTREE_H
#define URDFAPPROXGEOM_SPHERETREEOCTREE_H

#include "irmv/bot_common/alg_factory/algorithm_factory.h"
#include "sphereTreeBase.h"

namespace SphereTreeMethod {
constexpr char SphereTreeMethodOctreeName[] = "SphereTreeOctreeName";

class SphereTreeMethodOctree : public SphereTreeMethodBase {
  public:
    SphereTreeMethodOctree(const std::string& config_path);

    ~SphereTreeMethodOctree() override = default;

    static SphereTreeUniquePtr create(const std::string& config_path);

    irmv_core::bot_common::ErrorInfo constructTree(Surface& sur, MySphereTree& tree) override;

  protected:
    int depth = 3;         ///<  depth of the sphere-tree
    bool verify = false;   ///<  verify model before construction
    bool nopause = false;  ///<  will we pause before starting
    bool eval = false;     ///<  do we evaluate the sphere-tree after construction
};

inline irmv_core::bot_common::REGISTER_ALGORITHM(SphereTreeMethodBase, SphereTreeMethodOctreeName,
                                                 SphereTreeMethodOctree, const std::string&);
}  // namespace SphereTreeMethod

#endif  // URDFAPPROXGEOM_SPHERETREEOCTREE_H
