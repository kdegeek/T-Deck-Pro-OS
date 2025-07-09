#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

/**
 * @brief Simple logging utility for T-Deck-Pro OS
 */
class Logger {
public:
    enum LogLevel {
        LOG_DEBUG = 0,
        LOG_INFO = 1,
        LOG_WARNING = 2,
        LOG_ERROR = 3
    };

    static void setLogLevel(LogLevel level) {
        currentLogLevel = level;
    }

    static void debug(const String& tag, const String& message) {
        if (currentLogLevel <= LOG_DEBUG) {
            Serial.printf("[DEBUG][%s] %s\n", tag.c_str(), message.c_str());
        }
    }

    static void info(const String& tag, const String& message) {
        if (currentLogLevel <= LOG_INFO) {
            Serial.printf("[INFO][%s] %s\n", tag.c_str(), message.c_str());
        }
    }

    static void warning(const String& tag, const String& message) {
        if (currentLogLevel <= LOG_WARNING) {
            Serial.printf("[WARNING][%s] %s\n", tag.c_str(), message.c_str());
        }
    }

    static void error(const String& tag, const String& message) {
        if (currentLogLevel <= LOG_ERROR) {
            Serial.printf("[ERROR][%s] %s\n", tag.c_str(), message.c_str());
        }
    }

    // Template versions for formatted strings
    template<typename... Args>
    static void debug(const String& tag, const char* format, Args... args) {
        if (currentLogLevel <= LOG_DEBUG) {
            Serial.printf("[DEBUG][%s] ", tag.c_str());
            Serial.printf(format, args...);
            Serial.println();
        }
    }

    template<typename... Args>
    static void info(const String& tag, const char* format, Args... args) {
        if (currentLogLevel <= LOG_INFO) {
            Serial.printf("[INFO][%s] ", tag.c_str());
            Serial.printf(format, args...);
            Serial.println();
        }
    }

    template<typename... Args>
    static void warning(const String& tag, const char* format, Args... args) {
        if (currentLogLevel <= LOG_WARNING) {
            Serial.printf("[WARNING][%s] ", tag.c_str());
            Serial.printf(format, args...);
            Serial.println();
        }
    }

    template<typename... Args>
    static void error(const String& tag, const char* format, Args... args) {
        if (currentLogLevel <= LOG_ERROR) {
            Serial.printf("[ERROR][%s] ", tag.c_str());
            Serial.printf(format, args...);
            Serial.println();
        }
    }

private:
    static LogLevel currentLogLevel;
};

#endif // LOGGER_H