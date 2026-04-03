#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <string_view>

namespace crynet::core {

/**
 * @cn
 * 日志级别枚举，按严重程度从低到高排列。
 *
 * @en
 * Log-level enumeration ordered from lowest to highest severity.
 */
enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
};

/**
 * @cn
 * 结构化日志记录，保存级别、组件和消息文本。
 *
 * @en
 * Structured log record storing severity, component, and message text.
 */
struct LogRecord final {
    /**
     * @cn
     * 当前日志级别。
     *
     * @en
     * Current log severity.
     */
    LogLevel level{LogLevel::Info};

    /**
     * @cn
     * 产生日志的组件名称。
     *
     * @en
     * Name of the component producing the log.
     */
    std::string component;

    /**
     * @cn
     * 日志消息文本。
     *
     * @en
     * Log message text.
     */
    std::string message;
};

/**
 * @cn
 * 日志 sink 回调类型，接收结构化记录与格式化文本。
 *
 * @en
 * Log-sink callback type receiving the structured record and formatted text.
 */
using LogSink = std::function<void(const LogRecord&, std::string_view)>;

/**
 * @cn
 * 返回日志级别的小写名称。
 *
 * @en
 * Return the lowercase name of a log level.
 */
[[nodiscard]] std::string_view log_level_name(LogLevel level) noexcept;

/**
 * @cn
 * 最小线程安全 logger，支持级别过滤、自定义 sink 和默认控制台输出。
 *
 * @en
 * Minimal thread-safe logger supporting level filtering, custom sinks, and default console output.
 */
class Logger final {
public:
    /**
     * @cn
     * 获取全局默认 logger 实例。
     *
     * @en
     * Get the global default logger instance.
     */
    [[nodiscard]] static Logger& default_logger() noexcept;

    /**
     * @cn
     * 设置最小输出级别。
     *
     * @en
     * Set the minimum severity level to emit.
     */
    void set_level(LogLevel level) noexcept;

    /**
     * @cn
     * 返回当前最小输出级别。
     *
     * @en
     * Return the current minimum severity level.
     */
    [[nodiscard]] LogLevel level() const noexcept;

    /**
     * @cn
     * 设置自定义 sink，用于拦截日志输出。
     *
     * @en
     * Set a custom sink used to intercept log output.
     */
    void set_sink(LogSink sink);

    /**
     * @cn
     * 恢复默认控制台 sink。
     *
     * @en
     * Restore the default console sink.
     */
    void reset_sink();

    /**
     * @cn
     * 将当前 logger 恢复为默认级别与默认 sink。
     *
     * @en
     * Restore the logger to the default level and default sink.
     */
    void reset() noexcept;

    /**
     * @cn
     * 按级别输出一条日志记录；若低于当前阈值则忽略。
     *
     * @en
     * Emit one log record at the specified severity; ignored when below the current threshold.
     */
    void log(LogLevel level, std::string_view component, std::string_view message);

private:
    /**
     * @cn
     * 构造格式化日志文本。
     *
     * @en
     * Build the formatted log line.
     */
    [[nodiscard]] static std::string format_line(const LogRecord& record);

    /**
     * @cn
     * 默认控制台 sink。
     *
     * @en
     * Default console sink.
     */
    static void default_sink(const LogRecord& record, std::string_view formatted_line);

    /**
     * @cn
     * 保护 logger 配置状态的互斥锁。
     *
     * @en
     * Mutex protecting logger configuration state.
     */
    mutable std::mutex m_mutex;

    /**
     * @cn
     * 当前最小输出级别。
     *
     * @en
     * Current minimum emission level.
     */
    LogLevel m_level{LogLevel::Info};

    /**
     * @cn
     * 当前使用的日志 sink。
     *
     * @en
     * Active log sink.
     */
    LogSink m_sink{default_sink};
};

}  // namespace crynet::core