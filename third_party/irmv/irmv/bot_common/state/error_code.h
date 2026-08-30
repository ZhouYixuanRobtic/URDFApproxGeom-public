/*
 * Minimal in-repo replacement for the external irmv_core package.
 *
 * Provides the subset of irmv_core::bot_common used by URDFApproxGeom:
 * ErrorCode + ErrorInfo. Kept API-compatible so call sites are unchanged.
 */

#ifndef URDFAPPROXGEOM_IRMV_ERROR_CODE_H
#define URDFAPPROXGEOM_IRMV_ERROR_CODE_H

#include <string>
#include <string_view>

namespace irmv_core {
namespace bot_common {

enum class ErrorCode {
    OK = 0,
    GENERAL_ERROR = 1,
};

inline std::string getErrorMessage(ErrorCode error_code) {
    switch (error_code) {
    case ErrorCode::OK:
        return "Operation successful";
    case ErrorCode::GENERAL_ERROR:
        return "General error";
    }
    return "Unknown error code";
}

class ErrorInfo {
public:
    ErrorInfo() noexcept : error_code_(ErrorCode::OK), error_msg_("Operation successful") {}
    static ErrorInfo ok() noexcept { return ErrorInfo{}; }
    ErrorInfo(ErrorCode error_code, std::string_view error_msg)
        : error_code_(error_code), error_msg_(error_msg) {}
    explicit ErrorInfo(ErrorCode error_code)
        : error_code_(error_code), error_msg_(getErrorMessage(error_code)) {}

    ErrorInfo(const ErrorInfo&) = default;
    ErrorInfo(ErrorInfo&&) = default;
    ErrorInfo& operator=(const ErrorInfo&) = default;
    ErrorInfo& operator=(ErrorInfo&&) = default;
    ~ErrorInfo() = default;

    constexpr ErrorCode code() const noexcept { return error_code_; }
    const std::string& message() const noexcept { return error_msg_; }
    constexpr bool isOk() const noexcept { return error_code_ == ErrorCode::OK; }
    constexpr bool isError() const noexcept { return error_code_ != ErrorCode::OK; }

    constexpr bool operator==(const ErrorInfo& other) const noexcept {
        return error_code_ == other.error_code_;
    }
    constexpr bool operator!=(const ErrorInfo& other) const noexcept {
        return !(*this == other);
    }

private:
    ErrorCode error_code_;
    std::string error_msg_;
};

} // namespace bot_common
} // namespace irmv_core

#endif // URDFAPPROXGEOM_IRMV_ERROR_CODE_H
