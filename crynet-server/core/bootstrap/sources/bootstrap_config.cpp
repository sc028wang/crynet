#include "core/bootstrap/includes/bootstrap_config.h"

#include <charconv>
#include <cctype>
#include <string>

namespace crynet::bootstrap {
namespace {

std::string trim_copy(std::string_view text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }

    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }

    return std::string{text.substr(begin, end - begin)};
}

}  // namespace

std::optional<BootstrapConfig> BootstrapConfig::from_text(std::string_view config_text) {
    BootstrapConfig config;

    while (!config_text.empty()) {
        const auto line_break = config_text.find('\n');
        const auto raw_line = config_text.substr(0, line_break);
        const auto line = trim_copy(raw_line);

        if (!line.empty() && !line.starts_with('#')) {
            if (line.starts_with("service ")) {
                const auto name_begin = line.find_first_not_of(' ', 8);
                if (name_begin == std::string::npos) {
                    return std::nullopt;
                }

                const auto payload_begin = line.find(' ', name_begin);
                BootstrapServiceConfig service;
                if (payload_begin == std::string::npos) {
                    service.name = line.substr(name_begin);
                } else {
                    service.name = line.substr(name_begin, payload_begin - name_begin);
                    service.boot_payload = trim_copy(line.substr(payload_begin + 1));
                }

                if (service.name.empty()) {
                    return std::nullopt;
                }

                config.m_services.push_back(std::move(service));
            } else if (line.starts_with("timeout_ms ")) {
                const auto value_view = std::string_view{line}.substr(11);
                std::uint64_t timeout_value = 0;
                const auto parse_result = std::from_chars(
                    value_view.data(),
                    value_view.data() + value_view.size(),
                    timeout_value
                );
                if (parse_result.ec != std::errc{}) {
                    return std::nullopt;
                }

                config.m_timeout = std::chrono::milliseconds{timeout_value};
            } else {
                return std::nullopt;
            }
        }

        if (line_break == std::string_view::npos) {
            break;
        }

        config_text.remove_prefix(line_break + 1);
    }

    if (config.m_services.empty()) {
        return std::nullopt;
    }

    return config;
}

const std::vector<BootstrapServiceConfig>& BootstrapConfig::services() const noexcept {
    return m_services;
}

std::chrono::milliseconds BootstrapConfig::timeout() const noexcept {
    return m_timeout;
}

}  // namespace crynet::bootstrap