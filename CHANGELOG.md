# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.5.0] - 2026-07-03

### Added
- **Capsule collision geometry approximation** (`capsuleized` binary)
  - Wu2018 cross-section decomposition: mesh-plane slicing, COA circle fitting, capsule chaining
  - Config-driven multi-circle section fitting with k-means++ and adaptive circle count
  - Volume-driven axial splitting with assignment-based tightness metrics
  - Endpoint-span optimization: `p0`/`p1` are sphere centers, not mesh axial extrema
  - Coverage-aware budget pruning and variable-count chain matching
  - Local radius-inflation guard in capsule merging
- **Diagnostic toolchain** (`scripts/`)
  - `check_capsule_coverage.py`: per-link coverage, tightness, and axial overhang metrics (JSON + human-readable)
  - `check_capsule_tightness.py`: global + per-link gate enforcement (capV/aabb, r/binMed, capsule count, overhang)
  - `compare_capsule_presets.py`: sparse-vs-tight regression detection with absolute ceilings
- **Config presets**: `capsuleConfig.yml` (sparse), `capsuleConfig_tight.yml` (tight)
- **25 C++ unit tests** covering cross-section fitting, COA math, splitting, merging, endpoint semantics, and FR3 integration
- **FR3 capsule output** (`resources/fr3/urdf/fr3_capsuleized.{urdf,json}`)

### Changed
- Capsule fitter now uses assignment-based metrics aligned with Python gate thresholds
- Test integration outputs redirected to `/tmp/` to avoid polluting tracked assets
- Merge radius-difference guard tightened from 0.30 to 0.15

### Fixed
- Coverage diagnostics now evaluate union-of-capsules per link (was single-capsule false negatives)
- Config values `CoaThreshold` and `MaxCirclesPerSection` now actually drive the fitter
- Local capsule radii preserved during merge (no more bulge-radius inheritance)

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

