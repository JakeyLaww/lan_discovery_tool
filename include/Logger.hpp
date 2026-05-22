#pragma once

#include <memory>
#include <string>

enum class LogLevel {
    Error = 0,
    Warn,
    Info,
    Debug
};

/**
 * @brief Abstract logging interface.
 */
class Logger {
public:
    virtual ~Logger() noexcept = default;

    virtual void set_level(LogLevel level) { (void)level; }
    virtual LogLevel level() const { return LogLevel::Info; }

    virtual void info(const std::string& message) = 0;
    virtual void warn(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
    virtual void debug(const std::string& message) = 0;
};

using LoggerPtr = std::shared_ptr<Logger>;

inline bool log_level_enabled(LogLevel current, LogLevel message_level) {
    return static_cast<int>(message_level) <= static_cast<int>(current);
}
