#include "core/base/includes/build_config.h"

namespace crynet::base {
namespace {
constexpr std::string_view k_project_name{"crynet-server"};
constexpr std::string_view k_project_version{"0.1.0"};

#if defined(__APPLE__)
constexpr std::string_view k_platform_name{"macOS"};
#elif defined(__linux__)
constexpr std::string_view k_platform_name{"Linux"};
#else
constexpr std::string_view k_platform_name{"Unknown"};
#endif
}  // namespace

std::string_view BuildConfig::project_name() noexcept {
    return k_project_name;
}

std::string_view BuildConfig::version() noexcept {
    return k_project_version;
}

std::string_view BuildConfig::platform() noexcept {
    return k_platform_name;
}

}  // namespace crynet::base
