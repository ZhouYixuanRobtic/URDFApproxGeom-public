/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "SphereTreeURDFGenerator.h"
#include "irmv/bot_common/log/singleton_logger.h"

int main(int argc, char* argv[]) {
    // Initialize logger
    irmv_core::logging::SingletonLogger::getInstance().initialize("URDFApproxGeom");
    if (argc < 4) {
        IRMV_ERROR("Usage: {} -i <input_urdf_path> -o <output_urdf_path> [-r <key> <value> ...] "
                   "[-c <sphere_config.yml>] [--simplify <0|1>]",
                   argv[0]);
        return 1;
    }

    std::string configPath = URDFApproxGeom_CONFIG_PATH;
    std::string inputPath;
    std::string outputPath;
    std::string sphereConfig;
    std::vector<std::pair<std::string, std::string>> replacements;
    bool simplify = true;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            inputPath = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "-r" && i + 2 < argc) {
            // Parse replacement pairs
            std::string key = argv[++i];
            std::string value = argv[++i];
            replacements.emplace_back(key, value);
        } else if (arg == "--simplify" && i + 1 < argc) {
            simplify = std::stoi(argv[++i]);
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            sphereConfig = argv[++i];
        } else {
            IRMV_ERROR("Unknown argument: {}", arg);
            return 1;
        }
    }

    // Check if input and output paths are provided
    if (inputPath.empty() || outputPath.empty()) {
        IRMV_ERROR("Error: Missing input or output path.");
        return 1;
    }

    if (sphereConfig.empty()) {
        sphereConfig = configPath + "/sphereTree/sphereTreeConfig.yml";
    }

    // Create the SphereTreeURDFGenerator instance
    auto spherized_generator = std::make_shared<SphereTreeURDFGenerator>(sphereConfig, simplify);

    // Run the generator with the input, output, and replacement pairs
    auto ret = spherized_generator->run(inputPath, outputPath, replacements);
    IRMV_INFO("Processing {} -> {}: {}", inputPath, outputPath, ret.message());

    return 0;
}
