/**
 * @file logger.h
 * @brief T-Deck-Pro Logger - Centralized Logging System
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Provides centralized logging with different levels and outputs
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

// ===== LOG LEVELS =====
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};

// ===== LOG OUTPUTS =====
enum class LogOutput {
    SERIAL,
    SD_CARD,
    MQTT,
    DISPLAY
};

/**
 * @brief Log message structure
 */
struct LogMessage {
    LogLevel level;
    String tag;
    String message;
    uint32_t timestamp;
    uint32_t uptime;
    String source;
};

/**
 * @brief Logger configuration
 */
struct LoggerConfig {
    LogLevel min_level;
    bool enable_timestamp;
    bool enable_uptime;
    bool enable_source;
    uint32_t max_messages;
    bool auto_flush;
    uint32_t flush_interval;
};

/**
 * @brief Logger for T-Deck-Pro OS
 * @note Provides centralized logging with multiple outputs
 */
class Logger {
public:
    Logger();
    ~Logger();
    
    /**
     * @brief Initialize logger
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Check if logger is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Set logger configuration
     * @param config Logger configuration
     */
    void setConfig(const LoggerConfig& config);
    
    /**
     * @brief Get logger configuration
     * @return Logger configuration
     */
    LoggerConfig getConfig() const { return config_; }
    
    /**
     * @brief Set minimum log level
     * @param level Minimum log level
     */
    void setMinLevel(LogLevel level);
    
    /**
     * @brief Get minimum log level
     * @return Minimum log level
     */
    LogLevel getMinLevel() const { return config_.min_level; }
    
    /**
     * @brief Enable/disable log output
     * @param output Log output
     * @param enabled true to enable
     */
    void setOutputEnabled(LogOutput output, bool enabled);
    
    /**
     * @brief Check if log output is enabled
     * @param output Log output
     * @return true if enabled
     */
    bool isOutputEnabled(LogOutput output) const;
    
    /**
     * @brief Log debug message
     * @param tag Message tag
     * @param message Message content
     */
    void debug(const String& tag, const String& message);
    
    /**
     * @brief Log info message
     * @param tag Message tag
     * @param message Message content
     */
    void info(const String& tag, const String& message);
    
    /**
     * @brief Log warning message
     * @param tag Message tag
     * @param message Message content
     */
    void warn(const String& tag, const String& message);
    
    /**
     * @brief Log error message
     * @param tag Message tag
     * @param message Message content
     */
    void error(const String& tag, const String& message);
    
    /**
     * @brief Log critical message
     * @param tag Message tag
     * @param message Message content
     */
    void critical(const String& tag, const String& message);
    
    /**
     * @brief Log message with level
     * @param level Log level
     * @param tag Message tag
     * @param message Message content
     */
    void log(LogLevel level, const String& tag, const String& message);
    
    /**
     * @brief Log formatted message
     * @param level Log level
     * @param tag Message tag
     * @param format Format string
     * @param ... Format arguments
     */
    void logf(LogLevel level, const String& tag, const char* format, ...);
    
    /**
     * @brief Log JSON message
     * @param level Log level
     * @param tag Message tag
     * @param json JSON document
     */
    void logJson(LogLevel level, const String& tag, const JsonDocument& json);
    
    /**
     * @brief Get log messages
     * @param level Minimum log level
     * @param max_count Maximum number of messages
     * @return Vector of log messages
     */
    std::vector<LogMessage> getMessages(LogLevel level = LogLevel::DEBUG, uint32_t max_count = 100);
    
    /**
     * @brief Clear log messages
     */
    void clearMessages();
    
    /**
     * @brief Save logs to file
     * @param file_path File path
     * @return true if successful
     */
    bool saveToFile(const String& file_path);
    
    /**
     * @brief Load logs from file
     * @param file_path File path
     * @return true if successful
     */
    bool loadFromFile(const String& file_path);
    
    /**
     * @brief Get log statistics
     * @return Statistics JSON string
     */
    String getStatistics();
    
    /**
     * @brief Reset log statistics
     */
    void resetStatistics();
    
    /**
     * @brief Process logger events
     */
    void process();
    
    /**
     * @brief Flush log buffers
     */
    void flush();

private:
    // ===== LOG OPERATIONS =====
    
    /**
     * @brief Initialize log outputs
     * @return true if successful
     */
    bool initOutputs();
    
    /**
     * @brief Write message to outputs
     * @param message Log message
     */
    void writeMessage(const LogMessage& message);
    
    /**
     * @brief Write to serial output
     * @param message Log message
     */
    void writeToSerial(const LogMessage& message);
    
    /**
     * @brief Write to SD card output
     * @param message Log message
     */
    void writeToSDCard(const LogMessage& message);
    
    /**
     * @brief Write to MQTT output
     * @param message Log message
     */
    void writeToMQTT(const LogMessage& message);
    
    /**
     * @brief Write to display output
     * @param message Log message
     */
    void writeToDisplay(const LogMessage& message);
    
    /**
     * @brief Format log message
     * @param message Log message
     * @return Formatted message string
     */
    String formatMessage(const LogMessage& message);
    
    /**
     * @brief Get log level string
     * @param level Log level
     * @return Level string
     */
    String getLevelString(LogLevel level);
    
    /**
     * @brief Get log level color
     * @param level Log level
     * @return Color string
     */
    String getLevelColor(LogLevel level);
    
    /**
     * @brief Add message to buffer
     * @param message Log message
     */
    void addMessage(const LogMessage& message);
    
    /**
     * @brief Update log statistics
     * @param level Log level
     */
    void updateStatistics(LogLevel level);

private:
    // ===== MEMBER VARIABLES =====
    bool initialized_;
    LoggerConfig config_;
    
    // Output settings
    bool serial_enabled_;
    bool sd_card_enabled_;
    bool mqtt_enabled_;
    bool display_enabled_;
    
    // Message buffer
    std::vector<LogMessage> messages_;
    uint32_t message_count_;
    
    // Statistics
    uint32_t debug_count_;
    uint32_t info_count_;
    uint32_t warn_count_;
    uint32_t error_count_;
    uint32_t critical_count_;
    uint32_t total_messages_;
    
    // Timing
    uint32_t last_flush_time_;
    uint32_t flush_interval_;
    
    // File handling
    String log_file_path_;
    bool file_output_enabled_;
};

// ===== GLOBAL LOGGER INSTANCE =====
extern Logger* g_logger;

// ===== LOGGER UTILITY FUNCTIONS =====

/**
 * @brief Initialize global logger
 * @return true if successful
 */
bool initializeLogger();

/**
 * @brief Get global logger instance
 * @return Logger pointer
 */
Logger* getLogger();

/**
 * @brief Log debug message (global)
 * @param tag Message tag
 * @param message Message content
 */
void logDebug(const String& tag, const String& message);

/**
 * @brief Log info message (global)
 * @param tag Message tag
 * @param message Message content
 */
void logInfo(const String& tag, const String& message);

/**
 * @brief Log warning message (global)
 * @param tag Message tag
 * @param message Message content
 */
void logWarn(const String& tag, const String& message);

/**
 * @brief Log error message (global)
 * @param tag Message tag
 * @param message Message content
 */
void logError(const String& tag, const String& message);

/**
 * @brief Log critical message (global)
 * @param tag Message tag
 * @param message Message content
 */
void logCritical(const String& tag, const String& message);

/**
 * @brief Get log level string
 * @param level Log level
 * @return Level string
 */
String getLogLevelString(LogLevel level);

/**
 * @brief Parse log level from string
 * @param level_string Level string
 * @return Log level
 */
LogLevel parseLogLevel(const String& level_string);

/**
 * @brief Format timestamp
 * @param timestamp Timestamp
 * @return Formatted timestamp string
 */
String formatTimestamp(uint32_t timestamp);

/**
 * @brief Format uptime
 * @param uptime_ms Uptime in milliseconds
 * @return Formatted uptime string
 */
String formatUptime(uint32_t uptime_ms);

// ===== LOG MACROS =====
#define LOG_DEBUG(tag, message) logDebug(tag, message)
#define LOG_INFO(tag, message) logInfo(tag, message)
#define LOG_WARN(tag, message) logWarn(tag, message)
#define LOG_ERROR(tag, message) logError(tag, message)
#define LOG_CRITICAL(tag, message) logCritical(tag, message)

#endif // LOGGER_H 