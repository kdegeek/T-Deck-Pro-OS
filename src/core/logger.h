/**
 * @file      logger.h
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Unified Logging System for T-Deck-Pro OS
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>

// Forward declarations
class HardwareManager;

/**
 * @brief Log levels in order of severity
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

/**
 * @brief Log output destinations
 */
enum class LogOutput {
    LOG_SERIAL = 0,
    SD_CARD = 1,
    MQTT = 2,
    LOG_DISPLAY = 3
};

/**
 * @brief Log message structure
 */
struct LogMessage {
    uint32_t timestamp;
    LogLevel level;
    String component;
    String message;
    String formatted_message;
};

/**
 * @brief Log output interface
 */
class LogOutputHandler {
public:
    virtual ~LogOutputHandler() = default;
    virtual bool init() = 0;
    virtual bool write(const LogMessage& message) = 0;
    virtual void flush() = 0;
    virtual bool isAvailable() = 0;
};

/**
 * @brief Serial output handler
 */
class SerialLogHandler : public LogOutputHandler {
public:
    bool init() override;
    bool write(const LogMessage& message) override;
    void flush() override;
    bool isAvailable() override;

private:
    bool initialized = false;
};

/**
 * @brief SD card output handler
 */
class SDLogHandler : public LogOutputHandler {
public:
    SDLogHandler(const char* log_file_path = "/logs/system.log");
    ~SDLogHandler();
    
    bool init() override;
    bool write(const LogMessage& message) override;
    void flush() override;
    bool isAvailable() override;
    
    void setMaxFileSize(size_t max_size);
    void setMaxFiles(uint8_t max_files);

private:
    String log_file_path;
    size_t max_file_size = 1024 * 1024; // 1MB default
    uint8_t max_files = 5; // Keep 5 log files
    bool initialized = false;
    
    void rotateLogFiles();
    String getCurrentLogFileName();
};

/**
 * @brief MQTT output handler
 */
class MQTTLogHandler : public LogOutputHandler {
public:
    MQTTLogHandler(const char* topic_prefix = "tdeckpro/logs");
    
    bool init() override;
    bool write(const LogMessage& message) override;
    void flush() override;
    bool isAvailable() override;
    
    void setMQTTClient(void* mqtt_client);

private:
    String topic_prefix;
    void* mqtt_client = nullptr;
    bool initialized = false;
};

/**
 * @brief Display output handler (for critical messages)
 */
class DisplayLogHandler : public LogOutputHandler {
public:
    DisplayLogHandler();
    
    bool init() override;
    bool write(const LogMessage& message) override;
    void flush() override;
    bool isAvailable() override;
    
    void setHardwareManager(HardwareManager* hw);

private:
    HardwareManager* hardware = nullptr;
    bool initialized = false;
    std::vector<String> display_buffer;
    uint8_t max_lines = 10;
};

/**
 * @brief Main Logger Class
 * 
 * Thread-safe logging system with multiple output destinations
 */
class Logger {
public:
    /**
     * @brief Get singleton instance
     */
    static Logger& getInstance();

    /**
     * @brief Initialize logging system
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Shutdown logging system
     */
    void shutdown();

    /**
     * @brief Log a message
     * @param level Log level
     * @param component Component name
     * @param format Printf-style format string
     * @param ... Format arguments
     */
    void log(LogLevel level, const char* component, const char* format, ...);

    /**
     * @brief Log a message with va_list
     * @param level Log level
     * @param component Component name
     * @param format Printf-style format string
     * @param args Variable argument list
     */
    void logv(LogLevel level, const char* component, const char* format, va_list args);

    /**
     * @brief Convenience logging methods
     */
    void debug(const char* component, const char* format, ...);
    void info(const char* component, const char* format, ...);
    void warn(const char* component, const char* format, ...);
    void error(const char* component, const char* format, ...);
    void fatal(const char* component, const char* format, ...);

    /**
     * @brief Configuration methods
     */
    void setLevel(LogLevel min_level);
    void enableOutput(LogOutput output, bool enabled);
    void setOutputs(bool serial, bool sd, bool mqtt, bool display = false);

    /**
     * @brief Add custom output handler
     * @param handler Custom output handler
     */
    void addOutputHandler(std::unique_ptr<LogOutputHandler> handler);

    /**
     * @brief Set hardware manager reference
     * @param hw Hardware manager instance
     */
    void setHardwareManager(HardwareManager* hw);

    /**
     * @brief Set MQTT client reference
     * @param mqtt_client MQTT client instance
     */
    void setMQTTClient(void* mqtt_client);

    /**
     * @brief Flush all output handlers
     */
    void flush();

    /**
     * @brief Get recent log messages
     * @param count Number of recent messages to return
     * @return Vector of recent log messages
     */
    std::vector<LogMessage> getRecentMessages(size_t count = 50);

    /**
     * @brief Clear message history
     */
    void clearHistory();

    /**
     * @brief Get log statistics
     */
    struct LogStats {
        uint32_t total_messages = 0;
        uint32_t debug_count = 0;
        uint32_t info_count = 0;
        uint32_t warn_count = 0;
        uint32_t error_count = 0;
        uint32_t fatal_count = 0;
        uint32_t dropped_messages = 0;
    };
    
    LogStats getStatistics();

private:
    // Singleton pattern
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Configuration
    LogLevel min_level = LogLevel::INFO;
    bool initialized = false;

    // Output handlers
    std::vector<std::unique_ptr<LogOutputHandler>> output_handlers;
    bool output_enabled[4] = {true, false, false, false}; // Serial, SD, MQTT, Display

    // Message history
    std::vector<LogMessage> message_history;
    size_t max_history_size = 100;

    // Statistics
    LogStats stats;

    // Thread safety
    std::mutex log_mutex;

    // Hardware references
    HardwareManager* hardware_manager = nullptr;
    void* mqtt_client = nullptr;

    // Internal methods
    void writeToOutputs(const LogMessage& message);
    String formatMessage(const LogMessage& message);
    String levelToString(LogLevel level);
    void addToHistory(const LogMessage& message);
    void initializeDefaultHandlers();
};

// Global logger instance
extern Logger& Log;

// Convenience macros
#define LOG_DEBUG(component, ...) Log.debug(component, __VA_ARGS__)
#define LOG_INFO(component, ...)  Log.info(component, __VA_ARGS__)
#define LOG_WARN(component, ...)  Log.warn(component, __VA_ARGS__)
#define LOG_ERROR(component, ...) Log.error(component, __VA_ARGS__)
#define LOG_FATAL(component, ...) Log.fatal(component, __VA_ARGS__)

// Conditional logging macros (only compile in debug builds)
#ifdef DEBUG
#define LOG_DEBUG_IF(condition, component, ...) \
    do { if (condition) Log.debug(component, __VA_ARGS__); } while(0)
#else
#define LOG_DEBUG_IF(condition, component, ...) do {} while(0)
#endif
