/*
 * Copyright © 2024 IRMV lab, Shanghai Jiao Tong University, China.
 * All Rights Reserved.
 */


#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ConvexHullCollisionURDFGenerator.h"
#include "irmv/bot_common/log/singleton_logger.h"

int main(int argc, char* argv[]) {
    // Initialize logger
    irmv_core::logging::SingletonLogger::getInstance().initialize("URDFApproxGeom");
    if (argc < 3) {
        IRMV_ERROR("Usage: {} -i <input_urdf_path> -o <output_urdf_path> [-r <key> <value> ...]",
                   argv[0]);
        return 1;
    }

    std::string inputPath;
    std::string outputPath;
    std::vector<std::pair<std::string, std::string>> replacements;
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

    // Create the SphereTreeURDFGenerator instance
    auto convex_generator = std::make_shared<ConvexHullCollisionURDFGenerator>();

    // Run the generator with the input, output, and replacement pairs
    auto ret = convex_generator->run(inputPath, outputPath, replacements);
    if (!ret.isOk()) {
        IRMV_ERROR("Processing {} -> {} failed: {}", inputPath, outputPath, ret.message());
        return 1;
    }
    IRMV_INFO("Processing {} -> {}: {}", inputPath, outputPath, ret.message());

    return 0;
}
