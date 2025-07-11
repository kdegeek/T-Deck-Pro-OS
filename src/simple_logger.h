/**
 * @file      simple_logger.h
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Simplified Arduino-compatible logging system for Phase 1
 */

#ifndef SIMPLE_LOGGER_H
#define SIMPLE_LOGGER_H

#include <Arduino.h>
#include <SD.h>
#include <FS.h>

enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3
};

class SimpleLogger {
private:
    static SimpleLogger* instance;
    LogLevel current_level;
    bool serial_enabled;
    bool sd_enabled;
    String log_filename;
    
    // Private constructor for singleton
    SimpleLogger();
    
    void writeToSerial(const char* level_str, const char* component, const char* message);
    void writeToSD(const char* level_str, const char* component, const char* message);
    const char* getLevelString(LogLevel level);

public:
    // Singleton access
    static SimpleLogger* getInstance();
    
    // Configuration
    bool init(LogLevel level = LOG_INFO);
    void enableSerial(bool enabled = true);
    void enableSD(bool enabled = true, const char* filename = "/logs/system.log");
    void setLogLevel(LogLevel level);
    
    // Logging methods
    void debug(const char* component, const char* message);
    void info(const char* component, const char* message);
    void warn(const char* component, const char* message);
    void error(const char* component, const char* message);
    
    // Printf-style logging methods
    void debugf(const char* component, const char* format, ...);
    void infof(const char* component, const char* format, ...);
    void warnf(const char* component, const char* format, ...);
    void errorf(const char* component, const char* format, ...);
    
    // Utility
    void flush();
    uint32_t getLogCount() { return log_count; }
    
private:
    uint32_t log_count;
    char format_buffer[512];
};

// Global logger instance
extern SimpleLogger* Logger;

// Convenience macros
#define LOG_DEBUG(component, message) Logger->debug(component, message)
#define LOG_INFO(component, message) Logger->info(component, message)
#define LOG_WARN(component, message) Logger->warn(component, message)
#define LOG_ERROR(component, message) Logger->error(component, message)

#define LOG_DEBUGF(component, format, ...) Logger->debugf(component, format, ##__VA_ARGS__)
#define LOG_INFOF(component, format, ...) Logger->infof(component, format, ##__VA_ARGS__)
#define LOG_WARNF(component, format, ...) Logger->warnf(component, format, ##__VA_ARGS__)
#define LOG_ERRORF(component, format, ...) Logger->errorf(component, format, ##__VA_ARGS__)

#endif // SIMPLE_LOGGER_H
