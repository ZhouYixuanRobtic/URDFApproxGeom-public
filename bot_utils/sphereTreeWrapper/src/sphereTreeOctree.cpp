/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#include "sphereTreeWrapper/sphereTreeOctree.h"
#include "API/STGOctree.h"
#include "Surface/OBJLoader.h"
#include "Surface/Surface.h"
#include "irmv/bot_common/log/singleton_logger.h"
#include "yaml-cpp/yaml.h"

namespace SphereTreeMethod {
template <typename T>
static inline T getParam(const YAML::Node& node, const std::string& name, const T& defaultValue) {
    T v;
    try {
        v = node[name].as<T>();
    } catch (std::exception& e) {
        IRMV_WARN("Yaml exception {}", e.what());
        v = defaultValue;
    }
    return v;
}

SphereTreeMethodOctree::SphereTreeMethodOctree(const std::string& config_path) {
    YAML::Node doc_full = YAML::LoadFile(config_path);
    m_method_name = "Octree";
    auto doc = doc_full[m_method_name];
    depth = getParam<int>(doc, "Depth", 3);
    verify = getParam<bool>(doc, "Verify", false);
    nopause = getParam<bool>(doc, "Nopause", false);
    eval = getParam<bool>(doc, "Eval", false);
}

SphereTreeUniquePtr SphereTreeMethodOctree::create(const std::string& config_path) {
    return irmv_core::bot_common::AlgorithmFactory<
        SphereTreeMethodBase, const std::string&>::CreateAlgorithm(SphereTreeMethodOctreeName,
                                                                   config_path);
}

irmv_core::bot_common::ErrorInfo SphereTreeMethodOctree::constructTree(Surface& sur,
                                                                       MySphereTree& tree) {
    /*
            scale box
        */
    float boxScale = sur.fitIntoBox(1000);

    STGOctree treegen;
    treegen.setSurface(sur);
    SphereTree m_tree;
    m_tree.setupTree(8, depth + 1);

    treegen.constructTree(&m_tree);
    tree.setBySphereTree(m_tree, 1.0 / boxScale);
    return irmv_core::bot_common::ErrorInfo::ok();
}
}  // namespace SphereTreeMethod
