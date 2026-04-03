#include "core/bootstrap/includes/bootstrap_config.h"

#include <charconv>
#include <cctype>
#include <fstream>
#include <iterator>
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

bool starts_with(std::string_view text, std::string_view prefix) noexcept {
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

std::optional<bool> parse_bool(std::string_view text) noexcept {
    if (text == "1" || text == "true" || text == "on" || text == "yes") {
        return true;
    }

    if (text == "0" || text == "false" || text == "off" || text == "no") {
        return false;
    }

    return std::nullopt;
}

}  // namespace

std::optional<BootstrapConfig> BootstrapConfig::from_text(std::string_view config_text) {
    BootstrapConfig config;

    while (!config_text.empty()) {
        const auto line_break = config_text.find('\n');
        const auto raw_line = config_text.substr(0, line_break);
        const auto line = trim_copy(raw_line);

        if (!line.empty() && line.front() != '#') {
            if (starts_with(line, "service ")) {
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
            } else if (starts_with(line, "timeout_ms ")) {
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
            } else if (starts_with(line, "monitor.enabled ")) {
                const auto enabled = parse_bool(trim_copy(std::string_view{line}.substr(16)));
                if (!enabled.has_value()) {
                    return std::nullopt;
                }

                config.m_monitor.enabled = enabled.value();
            } else if (starts_with(line, "monitor.logger_alert ")) {
                const auto enabled = parse_bool(trim_copy(std::string_view{line}.substr(21)));
                if (!enabled.has_value()) {
                    return std::nullopt;
                }

                config.m_monitor.logger_alert_enabled = enabled.value();
            } else if (starts_with(line, "monitor.check_cycles ")) {
                const auto value_view = std::string_view{line}.substr(21);
                std::uint64_t check_cycles = 0;
                const auto parse_result = std::from_chars(
                    value_view.data(),
                    value_view.data() + value_view.size(),
                    check_cycles
                );
                if (parse_result.ec != std::errc{}) {
                    return std::nullopt;
                }

                config.m_monitor.check_cycles = static_cast<std::size_t>(check_cycles);
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

std::optional<BootstrapConfig> BootstrapConfig::from_file(const std::filesystem::path& file_path) {
    std::ifstream input{file_path};
    if (!input.is_open()) {
        return std::nullopt;
    }

    const std::string config_text{
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{}
    };
    return from_text(config_text);
}

const std::vector<BootstrapServiceConfig>& BootstrapConfig::services() const noexcept {
    return m_services;
}

std::chrono::milliseconds BootstrapConfig::timeout() const noexcept {
    return m_timeout;
}

const BootstrapMonitorConfig& BootstrapConfig::monitor() const noexcept {
    return m_monitor;
}

}  // namespace crynet::bootstrap