#pragma once

#include <string>
#include <mutex>
#include <atomic>

enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Error = 3, Off = 4 };

class Logger {
public:
    static Logger& instance();

    void setLevel(LogLevel level) { level_.store(level, std::memory_order_relaxed); }
    LogLevel level() const { return level_.load(std::memory_order_relaxed); }

    void debug(const std::string& message);
    void info(const std::string& message);
    void error(const std::string& message);

private:
    Logger() = default;
    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void write(const char* tag, const std::string& message);

    std::mutex mutex_;
    std::atomic<LogLevel> level_{ LogLevel::Info };
};