/*
 ************************************************************************\

                              C O P Y R I G H T

   Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
                         All Rights Reserved.

   Licensed under the Creative Commons Attribution-NonCommercial 4.0
   International License (CC BY-NC 4.0).

   For commercial use or licensing inquiries, please contact:
   IRMV lab, Shanghai Jiao Tong University at: https://irmv.sjtu.edu.cn/

 \*************************************************************************

 */

#include "CapsuleURDFGenerator.h"
#include "irmv/bot_common/log/singleton_logger.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    // Initialize logger
    irmv_core::logging::SingletonLogger::getInstance().initialize("URDFApproxGeom");
    if (argc < 3) {
        IRMV_ERROR("Usage: {} -i <input_urdf_path> -o <output_urdf_path> [-r <key> <value> ...] [-c <capsule_config.yml>]",
                   argv[0]);
        return 1;
    }

    std::string configPath = URDFApproxGeom_CONFIG_PATH;
    std::string inputPath;
    std::string outputPath;
    std::string capsuleConfig;
    std::vector<std::pair<std::string, std::string>> replacements;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            inputPath = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "-r" && i + 2 < argc) {
            std::string key = argv[++i];
            std::string value = argv[++i];
            replacements.emplace_back(key, value);
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            capsuleConfig = argv[++i];
        } else {
            IRMV_ERROR("Unknown argument: {}", arg);
            return 1;
        }
    }

    if (inputPath.empty() || outputPath.empty()) {
        IRMV_ERROR("Error: Missing input or output path.");
        return 1;
    }

    if (capsuleConfig.empty()) {
        capsuleConfig = configPath + "/capsule/capsuleConfig.yml";
    }

    // Create the CapsuleURDFGenerator instance
    auto capsule_generator = std::make_shared<CapsuleURDFGenerator>(capsuleConfig);

    // Run the generator with the input, output, and replacement pairs
    auto ret = capsule_generator->run(inputPath, outputPath, replacements);
    IRMV_INFO("Processing {} -> {}: {}", inputPath, outputPath, ret.message());

    return 0;
}
