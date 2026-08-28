/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#ifndef URDFAPPROXGEOM_SPHERETREEHUBBARD_H
#define URDFAPPROXGEOM_SPHERETREEHUBBARD_H

#include "irmv/bot_common/alg_factory/algorithm_factory.h"
#include "sphereTreeBase.h"

namespace SphereTreeMethod {
constexpr char SphereTreeMethodHubbardName[] = "SphereTreeHubbardName";

class SphereTreeMethodHubbard : public SphereTreeMethodBase {
  public:
    SphereTreeMethodHubbard(const std::string& config_path);

    ~SphereTreeMethodHubbard() override = default;

    static SphereTreeUniquePtr create(const std::string& config_path);

    irmv_core::bot_common::ErrorInfo constructTree(Surface& sur, MySphereTree& tree) override;

  protected:
    int depth = 3;         ///<  depth of the sphere-tree
    int numSamples = 500;  //  number of samples to put on surface for static medial
    int minSamples = 1;    //  minimum number of points per triangle for static medial
    bool verify = false;   //  verify model before construction
    bool nopause = false;  //  will we pause before starting
};

inline irmv_core::bot_common::REGISTER_ALGORITHM(SphereTreeMethodBase, SphereTreeMethodHubbardName,
                                                 SphereTreeMethodHubbard, const std::string&);
}  // namespace SphereTreeMethod

#endif  // URDFAPPROXGEOM_SPHERETREEHUBBARD_H
