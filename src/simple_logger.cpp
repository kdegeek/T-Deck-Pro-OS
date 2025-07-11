/**
 * @file      simple_logger.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Simplified Arduino-compatible logging system implementation
 */

#include "simple_logger.h"
#include <stdarg.h>

// Static instance
SimpleLogger* SimpleLogger::instance = nullptr;
SimpleLogger* Logger = nullptr;

SimpleLogger::SimpleLogger() {
    current_level = LOG_INFO;
    serial_enabled = true;
    sd_enabled = false;
    log_count = 0;
    log_filename = "/logs/system.log";
}

SimpleLogger* SimpleLogger::getInstance() {
    if (instance == nullptr) {
        instance = new SimpleLogger();
        Logger = instance; // Set global pointer
    }
    return instance;
}

bool SimpleLogger::init(LogLevel level) {
    current_level = level;
    serial_enabled = true;
    sd_enabled = false;
    log_count = 0;
    
    Serial.println("[LOGGER] Simple Logger initialized");
    return true;
}

void SimpleLogger::enableSerial(bool enabled) {
    serial_enabled = enabled;
}

void SimpleLogger::enableSD(bool enabled, const char* filename) {
    sd_enabled = enabled;
    if (filename) {
        log_filename = String(filename);
    }
    
    if (enabled) {
        // Create logs directory if it doesn't exist
        if (SD.exists("/logs") || SD.mkdir("/logs")) {
            Serial.println("[LOGGER] SD card logging enabled");
        } else {
            Serial.println("[LOGGER] Failed to create logs directory");
            sd_enabled = false;
        }
    }
}

void SimpleLogger::setLogLevel(LogLevel level) {
    current_level = level;
}

const char* SimpleLogger::getLevelString(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKNOWN";
    }
}

void SimpleLogger::writeToSerial(const char* level_str, const char* component, const char* message) {
    if (!serial_enabled) return;
    
    uint32_t timestamp = millis();
    Serial.printf("[%lu] [%s] %s: %s\n", timestamp, level_str, component, message);
}

void SimpleLogger::writeToSD(const char* level_str, const char* component, const char* message) {
    if (!sd_enabled) return;
    
    File logFile = SD.open(log_filename.c_str(), FILE_APPEND);
    if (logFile) {
        uint32_t timestamp = millis();
        logFile.printf("[%lu] [%s] %s: %s\n", timestamp, level_str, component, message);
        logFile.close();
    }
}

void SimpleLogger::debug(const char* component, const char* message) {
    if (current_level <= LOG_DEBUG) {
        const char* level_str = getLevelString(LOG_DEBUG);
        writeToSerial(level_str, component, message);
        writeToSD(level_str, component, message);
        log_count++;
    }
}

void SimpleLogger::info(const char* component, const char* message) {
    if (current_level <= LOG_INFO) {
        const char* level_str = getLevelString(LOG_INFO);
        writeToSerial(level_str, component, message);
        writeToSD(level_str, component, message);
        log_count++;
    }
}

void SimpleLogger::warn(const char* component, const char* message) {
    if (current_level <= LOG_WARN) {
        const char* level_str = getLevelString(LOG_WARN);
        writeToSerial(level_str, component, message);
        writeToSD(level_str, component, message);
        log_count++;
    }
}

void SimpleLogger::error(const char* component, const char* message) {
    if (current_level <= LOG_ERROR) {
        const char* level_str = getLevelString(LOG_ERROR);
        writeToSerial(level_str, component, message);
        writeToSD(level_str, component, message);
        log_count++;
    }
}

void SimpleLogger::debugf(const char* component, const char* format, ...) {
    if (current_level <= LOG_DEBUG) {
        va_list args;
        va_start(args, format);
        vsnprintf(format_buffer, sizeof(format_buffer), format, args);
        va_end(args);
        debug(component, format_buffer);
    }
}

void SimpleLogger::infof(const char* component, const char* format, ...) {
    if (current_level <= LOG_INFO) {
        va_list args;
        va_start(args, format);
        vsnprintf(format_buffer, sizeof(format_buffer), format, args);
        va_end(args);
        info(component, format_buffer);
    }
}

void SimpleLogger::warnf(const char* component, const char* format, ...) {
    if (current_level <= LOG_WARN) {
        va_list args;
        va_start(args, format);
        vsnprintf(format_buffer, sizeof(format_buffer), format, args);
        va_end(args);
        warn(component, format_buffer);
    }
}

void SimpleLogger::errorf(const char* component, const char* format, ...) {
    if (current_level <= LOG_ERROR) {
        va_list args;
        va_start(args, format);
        vsnprintf(format_buffer, sizeof(format_buffer), format, args);
        va_end(args);
        error(component, format_buffer);
    }
}

void SimpleLogger::flush() {
    if (serial_enabled) {
        Serial.flush();
    }
}
