/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#include "sphereTreeWrapper/sphereTreeGrid.h"
#include "API/MSGrid.h"
#include "API/SEConvex.h"
#include "API/SESphPt.h"
#include "API/SRGrid.h"
#include "API/SSIsohedron.h"
#include "API/STGGeneric.h"
#include "EvalTree.h"
#include "Surface/OBJLoader.h"
#include "Surface/Surface.h"
#include "VerifyModel.h"
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

SphereTreeMethodGrid::SphereTreeMethodGrid(const std::string& config_path) {
    YAML::Node doc_full = YAML::LoadFile(config_path);
    m_method_name = "Grid";
    auto doc = doc_full[m_method_name];
    testerLevels = getParam<int>(doc, "TesterLevers", -1);
    branch = getParam<int>(doc, "Branch", 8);
    depth = getParam<int>(doc, "Depth", 3);
    numCoverPts = getParam<int>(doc, "NumCoverPts", 5000);
    minCoverPts = getParam<int>(doc, "MinCoverPts", 5);
    verify = getParam<bool>(doc, "Verify", false);
    nopause = getParam<bool>(doc, "Nopause", false);
    eval = getParam<bool>(doc, "Eval", false);
}

SphereTreeUniquePtr SphereTreeMethodGrid::create(const std::string& config_path) {
    return irmv_core::bot_common::AlgorithmFactory<
        SphereTreeMethodBase, const std::string&>::CreateAlgorithm(SphereTreeMethodGridName,
                                                                   config_path);
}

irmv_core::bot_common::ErrorInfo SphereTreeMethodGrid::constructTree(Surface& sur,
                                                                     MySphereTree& tree) {
    /*
            scale box
        */
    float boxScale = sur.fitIntoBox(1000);

    /*
        make medial tester
    */
    MedialTester mt;
    mt.setSurface(sur);
    mt.useLargeCover = true;

    /*
        setup evaluator
    */
    SEConvex convEval;
    convEval.setTester(mt);
    SEBase* eval_ = &convEval;

    Array<Point3D> sphPts;
    SESphPt sphEval;
    if (testerLevels > 0) {  //  <= 0 will use convex tester
        SSIsohedron::generateSamples(&sphPts, testerLevels - 1);
        sphEval.setup(mt, sphPts);
        eval_ = &sphEval;
        IRMV_INFO("Using concave tester {}", sphPts.getSize());
    }

    /*
        verify model
    */
    if (verify && !verifyModel(sur)) {
        return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR, "model is not usable"};
    }

    /*
        setup for the set of cover points
    */
    Array<Surface::Point> coverPts;
    MSGrid::generateSamples(&coverPts, numCoverPts, sur, TRUE, minCoverPts);
    IRMV_DEBUG("{} cover points", coverPts.getSize());

    /*
        setup GRID algorithm
    */
    SRGrid grid;
    grid.optimise = FALSE;
    grid.sphereEval = eval_;
    grid.useQuickTest = true;

    /*
        setup SphereTree constructor - using dynamic construction
    */
    STGGeneric treegen;
    treegen.eval = eval_;
    treegen.useRefit = true;
    treegen.setSamples(coverPts);
    treegen.reducer = &grid;

    /*
        make sphere-tree
    */
    SphereTree m_tree;
    m_tree.setupTree(branch, depth + 1);

    treegen.constructTree(&m_tree);
    tree.setBySphereTree(m_tree, 1.0 / boxScale);
    return irmv_core::bot_common::ErrorInfo::ok();
}

}  // namespace SphereTreeMethod
