#include "Logger.h"
#include <iostream>
#include <chrono>
#include <ctime>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

static std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    char buffer[26];
    ctime_s(buffer, sizeof(buffer), &time);
    buffer[24] = '\0';

    return std::string(buffer);
}

void Logger::info(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "[INFO] " << timestamp() << " - " << message << std::endl;
}

void Logger::error(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cerr << "[ERROR] " << timestamp() << " - " << message << std::endl;
}
