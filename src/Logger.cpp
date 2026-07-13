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

// The level check happens BEFORE the lock. A filtered-out log costs one
// relaxed atomic load - no mutex, no formatting, no console I/O.
void Logger::write(const char* tag, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << tag << " " << timestamp() << " - " << message << "\n";
}

void Logger::debug(const std::string& message) {
    if (level() > LogLevel::Debug) return;
    write("[DEBUG]", message);
}

void Logger::info(const std::string& message) {
    if (level() > LogLevel::Info) return;
    write("[INFO]", message);
}

void Logger::error(const std::string& message) {
    if (level() > LogLevel::Error) return;
    std::lock_guard<std::mutex> lock(mutex_);
    std::cerr << "[ERROR] " << timestamp() << " - " << message << std::endl;
}