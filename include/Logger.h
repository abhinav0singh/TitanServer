#pragma once

#include <string>
#include <mutex>

class Logger {
public:
    static Logger& instance();

    void info(const std::string& message);
    void error(const std::string& message);

private:
    Logger() = default;
    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::mutex mutex_;
};
