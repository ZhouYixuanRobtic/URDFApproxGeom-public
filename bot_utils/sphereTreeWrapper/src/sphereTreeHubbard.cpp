/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#include "sphereTreeWrapper/sphereTreeHubbard.h"
#include "API/MSGrid.h"
#include "API/SSIsohedron.h"
#include "API/STGHubbard.h"
#include "EvalTree.h"
#include "MedialAxis/Voronoi3D.h"
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

SphereTreeMethodHubbard::SphereTreeMethodHubbard(const std::string& config_path) {
    YAML::Node doc_full = YAML::LoadFile(config_path);
    m_method_name = "Hubbard";
    auto doc = doc_full[m_method_name];
    branch = getParam<int>(doc, "Branch", 8);
    depth = getParam<int>(doc, "Depth", 3);
    numSamples = getParam<int>(doc, "NumSamples", 500);
    minSamples = getParam<int>(doc, "MinSamples", 1);
    verify = getParam<bool>(doc, "Verify", false);
    nopause = getParam<bool>(doc, "Nopause", false);
}

SphereTreeUniquePtr SphereTreeMethodHubbard::create(const std::string& config_path) {
    return irmv_core::bot_common::AlgorithmFactory<
        SphereTreeMethodBase, const std::string&>::CreateAlgorithm(SphereTreeMethodHubbardName,
                                                                   config_path);
}

irmv_core::bot_common::ErrorInfo SphereTreeMethodHubbard::constructTree(Surface& sur,
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
        verify model
    */
    if (verify && !verifyModel(sur)) {
        return {irmv_core::bot_common::ErrorCode::GENERAL_ERROR, "model is not usable"};
    }

    /*
        generate the set of sample points
    */
    Array<Surface::Point> samplePts;
    MSGrid::generateSamples(&samplePts, numSamples, sur, TRUE, minSamples);
    IRMV_DEBUG("sample points: {}", samplePts.getSize());

    //  SurfaceRep coverRep;
    //  coverRep.setup(coverPts);

    /*
       Setup voronoi diagram
    */
    Point3D pC{};
    pC.x = (sur.pMax.x + sur.pMin.x) / 2.0;
    pC.y = (sur.pMax.y + sur.pMin.y) / 2.0;
    pC.z = (sur.pMax.z + sur.pMin.z) / 2.0;

    Voronoi3D vor;
    vor.initialise(pC, 1.5 * sur.pMin.distance(pC));
    vor.randomInserts(samplePts);

    /*
        setup HUBBARD's algorithm
    */
    STGHubbard hubbard;
    hubbard.setup(&vor, &mt);

    /*
        make sphere-tree
    */
    SphereTree m_tree;
    m_tree.setupTree(branch, depth + 1);

    hubbard.constructTree(&m_tree);
    m_tree.setupTree(branch, depth + 1);
    tree.setBySphereTree(m_tree, 1.0 / boxScale);

    return irmv_core::bot_common::ErrorInfo::ok();
}
}  // namespace SphereTreeMethod
