#pragma once

#include "Logger.hpp"
#include <iostream>
#include <mutex>

class ThreadSafeLogger : public Logger {
public:
    explicit ThreadSafeLogger(LogLevel min_level = LogLevel::Info) : min_level_(min_level) {}

    void set_level(LogLevel level) override { min_level_ = level; }
    LogLevel level() const override { return min_level_; }

    void info(const std::string& message) override {
        if (log_level_enabled(min_level_, LogLevel::Info)) {
            log_impl("INFO", std::cout, message);
        }
    }

    void warn(const std::string& message) override {
        if (log_level_enabled(min_level_, LogLevel::Warn)) {
            log_impl("WARN", std::cout, message);
        }
    }

    void error(const std::string& message) override {
        if (log_level_enabled(min_level_, LogLevel::Error)) {
            log_impl("ERROR", std::cerr, message);
        }
    }

    void debug(const std::string& message) override {
        if (log_level_enabled(min_level_, LogLevel::Debug)) {
            log_impl("DEBUG", std::cout, message);
        }
    }

private:
    mutable std::mutex log_mutex_;
    LogLevel min_level_;

    void log_impl(const std::string& level, std::ostream& stream, const std::string& message) const {
        std::lock_guard<std::mutex> lock(log_mutex_);
        stream << "[" << level << "] " << message << "\n";
        stream.flush();
    }
};

using StdoutLogger = ThreadSafeLogger;
