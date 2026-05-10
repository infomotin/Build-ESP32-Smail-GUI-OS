/**
 * @file logger.cpp
 * @brief Logging utility implementation
 */

#include "utils/logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <memory>
#include <fstream>

namespace esp32sim {

// Singleton instance
static std::unique_ptr<Logger> g_logger_instance = nullptr;

Logger::Logger() : min_level_(LogLevel::INFO) {
}

Logger::~Logger() = default;

Logger& Logger::instance() {
    if (!g_logger_instance) {
        g_logger_instance = std::make_unique<Logger>();
    }
    return *g_logger_instance;
}

void Logger::setLevel(LogLevel level) {
    min_level_ = level;
}

void Logger::addConsoleSink() {
    sinks_.push_back([](LogLevel lvl, const std::string& msg) {
        std::string prefix;
        switch (lvl) {
            case LogLevel::VERBOSE: prefix = "[V]"; break;
            case LogLevel::DEBUG:   prefix = "[D]"; break;
            case LogLevel::INFO:    prefix = "[I]"; break;
            case LogLevel::WARN:    prefix = "[W]"; break;
            case LogLevel::ERROR:   prefix = "[E]"; break;
            case LogLevel::NONE:    prefix = ""; break;
        }
        std::cout << prefix << " " << msg << std::endl;
    });
}

void Logger::addFileSink(const std::string& filename) {
    std::ofstream* file = new std::ofstream(filename, std::ios::app);
    sinks_.push_back([file](LogLevel, const std::string& msg) {
        (*file) << msg << std::endl;
    });
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < min_level_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    char time_buf[20];
    std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", std::localtime(&time_t));

    std::stringstream formatted;
    formatted << "[" << time_buf << "." << std::setfill('0') << std::setw(3) << ms.count()
             << "] " << message;

    // Send to all sinks
    for (const auto& sink : sinks_) {
        sink(level, formatted.str());
    }
}

// Convenience functions
void Logger::verbose(const std::string& msg) { log(LogLevel::VERBOSE, msg); }
void Logger::debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
void Logger::info(const std::string& msg) { log(LogLevel::INFO, msg); }
void Logger::warn(const std::string& msg) { log(LogLevel::WARN, msg); }
void Logger::error(const std::string& msg) { log(LogLevel::ERROR, msg); }

// Format helpers
std::string format(const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return std::string(buffer);
}

} // namespace esp32sim
