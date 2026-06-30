# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.0] - 2024-XX-XX

### Changed
- **BREAKING**: Updated `irmv_core` dependency from version 0.4 to 1.0
- **BREAKING**: Migrated from plog-based logging system to spdlog-based logging system from `irmv_core`
  - Replaced all `PLOGE`, `PLOGW`, `PLOGI`, `PLOGD` macros with `IRMV_ERROR`, `IRMV_WARN`, `IRMV_INFO`, `IRMV_DEBUG`
  - Replaced all `std::cout` and `std::cerr` with appropriate `IRMV_XXX` logging macros
  - Updated logging header includes from `irmv/bot_common/log/log.h` to `irmv/bot_common/log/singleton_logger.h`
- Updated error handling API to match `irmv_core` 1.0:
  - Changed namespace from `bot_common::` to `irmv_core::bot_common::`
  - Updated method names: `IsOK()` → `isOk()`, `error_msg()` → `message()`
  - Updated error codes: `ErrorCode::Error` → `ErrorCode::GENERAL_ERROR`
  - Updated factory methods: `ErrorInfo::OK()` → `ErrorInfo::ok()`
- Updated CMake configuration:
  - Removed `-DPLOG_CAPTURE_FILE` definition (no longer needed)
  - Added `${irmv_core_LIBRARIES}` linking to all targets that use logging
  - Updated minimum CMake version requirement from 3.26 to 3.22

### Added
- Logger initialization in main functions for proper logging setup
- Support for fmt-style formatting in all log messages
- Automatic Eigen type formatting support through `irmv_core` logging system

### Fixed
- Fixed compilation errors related to namespace changes in `irmv_core` 1.0
- Fixed linking errors by adding proper library dependencies
- Fixed formatting issues with `std::filesystem::path` in log messages

### Technical Details
- All source files updated to use new logging API
- All test files updated to use new logging API
- All header files updated with correct namespace declarations
- CMakeLists.txt updated for all subdirectories (app, test, bot_utils)

## [1.3.0] - Previous Version

### Features
- Initial release with spherized and convex URDF generation
- Support for multiple sphere tree algorithms (Grid, Hubbard, Medial, Octree, Spawn)
- Watertight mesh processing with ManifoldPlus
- Mesh simplification support
- URDF collision geometry generation

