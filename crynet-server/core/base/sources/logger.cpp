#include "core/logger/includes/logger.h"

#include <iostream>
#include <utility>

namespace crynet::core {

std::string_view log_level_name(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace:
            return "trace";
        case LogLevel::Debug:
            return "debug";
        case LogLevel::Info:
            return "info";
        case LogLevel::Warn:
            return "warn";
        case LogLevel::Error:
            return "error";
    }

    return "unknown";
}

Logger& Logger::default_logger() noexcept {
    static Logger logger;
    return logger;
}

void Logger::set_level(LogLevel level) noexcept {
    std::scoped_lock lock{m_mutex};
    m_level = level;
}

LogLevel Logger::level() const noexcept {
    std::scoped_lock lock{m_mutex};
    return m_level;
}

void Logger::set_sink(LogSink sink) {
    std::scoped_lock lock{m_mutex};
    m_sink = std::move(sink);
}

void Logger::reset_sink() {
    std::scoped_lock lock{m_mutex};
    m_sink = default_sink;
}

void Logger::reset() noexcept {
    std::scoped_lock lock{m_mutex};
    m_level = LogLevel::Info;
    m_sink = default_sink;
}

void Logger::log(LogLevel level, std::string_view component, std::string_view message) {
    LogSink sink;
    LogRecord record;
    {
        std::scoped_lock lock{m_mutex};
        if (static_cast<int>(level) < static_cast<int>(m_level)) {
            return;
        }

        sink = m_sink;
        record = LogRecord{
            .level = level,
            .component = std::string{component},
            .message = std::string{message},
        };
    }

    const auto formatted_line = format_line(record);
    sink(record, formatted_line);
}

std::string Logger::format_line(const LogRecord& record) {
    std::string line;
    line.reserve(record.component.size() + record.message.size() + 16);
    line.append("[");
    line.append(log_level_name(record.level));
    line.append("] ");
    if (!record.component.empty()) {
        line.append("[");
        line.append(record.component);
        line.append("] ");
    }
    line.append(record.message);
    return line;
}

void Logger::default_sink(const LogRecord& record, std::string_view formatted_line) {
    auto& stream = record.level == LogLevel::Error ? std::cerr : std::cout;
    stream << formatted_line << std::endl;
}

}  // namespace crynet::core