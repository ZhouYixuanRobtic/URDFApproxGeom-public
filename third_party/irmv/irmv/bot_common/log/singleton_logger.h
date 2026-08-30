/*
 * Minimal in-repo replacement for the external irmv_core package.
 *
 * Logs to stderr with fmt-style formatting, mirroring the IRMV_* macro
 * surface the codebase uses. Thread-safe via a shared mutex; level prefix
 * keeps the output greppable like the original spdlog-backed logger.
 */

#ifndef URDFAPPROXGEOM_IRMV_SINGLETON_LOGGER_H
#define URDFAPPROXGEOM_IRMV_SINGLETON_LOGGER_H

#include <fmt/format.h>

#include <cstdio>
#include <mutex>
#include <string>

namespace irmv_core {
namespace logging {

class SingletonLogger {
public:
    static SingletonLogger& getInstance() {
        static SingletonLogger instance;
        return instance;
    }
    void initialize(const std::string&) { /* stderr logging needs no setup */ }
    SingletonLogger(const SingletonLogger&) = delete;
    SingletonLogger& operator=(const SingletonLogger&) = delete;

private:
    SingletonLogger() = default;
};

inline void logLine(const char* level, const char* file, int line, const std::string& msg) {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    std::fprintf(stderr, "[%s] %s:%d: %s\n", level, file, line, msg.c_str());
}

} // namespace logging
} // namespace irmv_core

#define IRMV_TRACE(...) \
    ::irmv_core::logging::logLine("trace", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define IRMV_DEBUG(...) \
    ::irmv_core::logging::logLine("debug", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define IRMV_INFO(...) \
    ::irmv_core::logging::logLine("info", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define IRMV_WARN(...) \
    ::irmv_core::logging::logLine("warn", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define IRMV_ERROR(...) \
    ::irmv_core::logging::logLine("error", __FILE__, __LINE__, fmt::format(__VA_ARGS__))
#define IRMV_CRITICAL(...) \
    ::irmv_core::logging::logLine("critical", __FILE__, __LINE__, fmt::format(__VA_ARGS__))

#endif // URDFAPPROXGEOM_IRMV_SINGLETON_LOGGER_H
