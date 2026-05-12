/**
 * @file logger.h
 * @brief Logging utility for ESP32 Simulator
 */

#ifndef UTILS_LOGGER_H
#define UTILS_LOGGER_H

#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdarg>

// Undefine any conflicting macros from system headers
#ifdef DEBUG
#undef DEBUG
#endif
#ifdef ERROR
#undef ERROR
#endif

namespace esp32sim {

enum class LogLevel {
    VERBOSE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    NONE = 5
};

class Logger {
public:
    Logger();
    ~Logger();

    static Logger& instance();

    void setLevel(LogLevel level);
    void addConsoleSink();
    void addFileSink(const std::string& filename);

    void log(LogLevel level, const std::string& message);

    void verbose(const std::string& msg);
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

private:
    LogLevel min_level_;
    std::mutex mutex_;
    using Sink = std::function<void(LogLevel, const std::string&)>;
    std::vector<Sink> sinks_;
};

// Format helper
std::string format(const char* fmt, ...);

// Convenience macros (variadic formatting support)
#define LOG_VERBOSE(...) esp32sim::Logger::instance().log(esp32sim::LogLevel::VERBOSE, format(__VA_ARGS__))
#define LOG_DEBUG(...)   esp32sim::Logger::instance().log(esp32sim::LogLevel::DEBUG,   format(__VA_ARGS__))
#define LOG_INFO(...)    esp32sim::Logger::instance().log(esp32sim::LogLevel::INFO,    format(__VA_ARGS__))
#define LOG_WARN(...)    esp32sim::Logger::instance().log(esp32sim::LogLevel::WARN,    format(__VA_ARGS__))
#define LOG_ERROR(...)   esp32sim::Logger::instance().log(esp32sim::LogLevel::ERROR,   format(__VA_ARGS__))

} // namespace esp32sim

#endif // UTILS_LOGGER_H
