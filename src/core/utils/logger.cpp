/**
 * @file logger.cpp
 * @brief T-Deck-Pro Logger - Centralized Logging System Implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Provides centralized logging with different levels and outputs
 */

#include "logger.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <SD.h>

// ===== GLOBAL LOGGER INSTANCE =====
Logger* g_logger = nullptr;

// ===== LOGGER IMPLEMENTATION =====

Logger::Logger() : initialized_(false), message_count_(0), total_messages_(0),
                   debug_count_(0), info_count_(0), warn_count_(0), error_count_(0), critical_count_(0),
                   last_flush_time_(0), flush_interval_(5000), file_output_enabled_(false) {
    
    // Default configuration
    config_.min_level = LogLevel::INFO;
    config_.enable_timestamp = true;
    config_.enable_uptime = true;
    config_.enable_source = true;
    config_.max_messages = 1000;
    config_.auto_flush = true;
    config_.flush_interval = 5000;
    
    // Default output settings
    serial_enabled_ = true;
    sd_card_enabled_ = false;
    mqtt_enabled_ = false;
    display_enabled_ = false;
    
    // Initialize message buffer
    messages_.reserve(config_.max_messages);
}

Logger::~Logger() {
    if (initialized_) {
        flush();
    }
}

bool Logger::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Initialize outputs
    if (!initOutputs()) {
        return false;
    }
    
    // Set up log file path
    log_file_path_ = "/logs/system.log";
    
    initialized_ = true;
    
    // Log initialization
    info("LOGGER", "Logger initialized successfully");
    
    return true;
}

bool Logger::initOutputs() {
    // Serial output is always available
    serial_enabled_ = true;
    
    // SD card output (if available)
    if (SD.begin()) {
        sd_card_enabled_ = true;
        // Create logs directory if it doesn't exist
        if (!SD.exists("/logs")) {
            SD.mkdir("/logs");
        }
    }
    
    // MQTT output (will be enabled when MQTT is available)
    mqtt_enabled_ = false;
    
    // Display output (will be enabled when display is available)
    display_enabled_ = false;
    
    return true;
}

void Logger::setConfig(const LoggerConfig& config) {
    config_ = config;
    
    // Resize message buffer if needed
    if (messages_.capacity() != config_.max_messages) {
        messages_.reserve(config_.max_messages);
    }
}

void Logger::setMinLevel(LogLevel level) {
    config_.min_level = level;
}

void Logger::setOutputEnabled(LogOutput output, bool enabled) {
    switch (output) {
        case LogOutput::SERIAL:
            serial_enabled_ = enabled;
            break;
        case LogOutput::SD_CARD:
            sd_card_enabled_ = enabled;
            break;
        case LogOutput::MQTT:
            mqtt_enabled_ = enabled;
            break;
        case LogOutput::DISPLAY:
            display_enabled_ = enabled;
            break;
    }
}

bool Logger::isOutputEnabled(LogOutput output) const {
    switch (output) {
        case LogOutput::SERIAL:
            return serial_enabled_;
        case LogOutput::SD_CARD:
            return sd_card_enabled_;
        case LogOutput::MQTT:
            return mqtt_enabled_;
        case LogOutput::DISPLAY:
            return display_enabled_;
        default:
            return false;
    }
}

void Logger::debug(const String& tag, const String& message) {
    log(LogLevel::DEBUG, tag, message);
}

void Logger::info(const String& tag, const String& message) {
    log(LogLevel::INFO, tag, message);
}

void Logger::warn(const String& tag, const String& message) {
    log(LogLevel::WARN, tag, message);
}

void Logger::error(const String& tag, const String& message) {
    log(LogLevel::ERROR, tag, message);
}

void Logger::critical(const String& tag, const String& message) {
    log(LogLevel::CRITICAL, tag, message);
}

void Logger::log(LogLevel level, const String& tag, const String& message) {
    if (level < config_.min_level) {
        return;
    }
    
    LogMessage log_msg;
    log_msg.level = level;
    log_msg.tag = tag;
    log_msg.message = message;
    log_msg.timestamp = millis();
    log_msg.uptime = millis();
    log_msg.source = "SYSTEM";
    
    addMessage(log_msg);
    writeMessage(log_msg);
    
    // Auto-flush if enabled
    if (config_.auto_flush && (millis() - last_flush_time_) > config_.flush_interval) {
        flush();
    }
}

void Logger::logf(LogLevel level, const String& tag, const char* format, ...) {
    if (level < config_.min_level) {
        return;
    }
    
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    log(level, tag, String(buffer));
}

void Logger::logJson(LogLevel level, const String& tag, const JsonDocument& json) {
    if (level < config_.min_level) {
        return;
    }
    
    String json_string;
    serializeJson(json, json_string);
    log(level, tag, "JSON: " + json_string);
}

std::vector<LogMessage> Logger::getMessages(LogLevel level, uint32_t max_count) {
    std::vector<LogMessage> filtered_messages;
    
    for (const auto& msg : messages_) {
        if (msg.level >= level) {
            filtered_messages.push_back(msg);
            if (filtered_messages.size() >= max_count) {
                break;
            }
        }
    }
    
    return filtered_messages;
}

void Logger::clearMessages() {
    messages_.clear();
    message_count_ = 0;
}

bool Logger::saveToFile(const String& file_path) {
    if (!sd_card_enabled_) {
        return false;
    }
    
    File file = SD.open(file_path, FILE_WRITE);
    if (!file) {
        return false;
    }
    
    for (const auto& msg : messages_) {
        String formatted = formatMessage(msg);
        file.println(formatted);
    }
    
    file.close();
    return true;
}

bool Logger::loadFromFile(const String& file_path) {
    if (!sd_card_enabled_) {
        return false;
    }
    
    File file = SD.open(file_path, FILE_READ);
    if (!file) {
        return false;
    }
    
    clearMessages();
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        // Parse log line and add to messages
        // This is a simplified implementation
        if (line.length() > 0) {
            LogMessage msg;
            msg.message = line;
            msg.timestamp = millis();
            msg.level = LogLevel::INFO;
            msg.tag = "FILE";
            addMessage(msg);
        }
    }
    
    file.close();
    return true;
}

String Logger::getStatistics() {
    DynamicJsonDocument doc(1024);
    
    doc["total_messages"] = total_messages_;
    doc["debug_count"] = debug_count_;
    doc["info_count"] = info_count_;
    doc["warn_count"] = warn_count_;
    doc["error_count"] = error_count_;
    doc["critical_count"] = critical_count_;
    doc["buffer_size"] = messages_.size();
    doc["max_buffer_size"] = config_.max_messages;
    
    String output;
    serializeJson(doc, output);
    return output;
}

void Logger::resetStatistics() {
    debug_count_ = 0;
    info_count_ = 0;
    warn_count_ = 0;
    error_count_ = 0;
    critical_count_ = 0;
    total_messages_ = 0;
}

void Logger::process() {
    // Auto-flush if enabled
    if (config_.auto_flush && (millis() - last_flush_time_) > config_.flush_interval) {
        flush();
    }
}

void Logger::flush() {
    if (sd_card_enabled_ && file_output_enabled_) {
        saveToFile(log_file_path_);
    }
    last_flush_time_ = millis();
}

void Logger::writeMessage(const LogMessage& message) {
    if (serial_enabled_) {
        writeToSerial(message);
    }
    
    if (sd_card_enabled_) {
        writeToSDCard(message);
    }
    
    if (mqtt_enabled_) {
        writeToMQTT(message);
    }
    
    if (display_enabled_) {
        writeToDisplay(message);
    }
}

void Logger::writeToSerial(const LogMessage& message) {
    String formatted = formatMessage(message);
    Serial.println(formatted);
}

void Logger::writeToSDCard(const LogMessage& message) {
    if (!SD.begin()) {
        return;
    }
    
    File file = SD.open(log_file_path_, FILE_APPEND);
    if (file) {
        String formatted = formatMessage(message);
        file.println(formatted);
        file.close();
    }
}

void Logger::writeToMQTT(const LogMessage& message) {
    // MQTT logging will be implemented when MQTT manager is available
    // For now, this is a placeholder
}

void Logger::writeToDisplay(const LogMessage& message) {
    // Display logging will be implemented when display manager is available
    // For now, this is a placeholder
}

String Logger::formatMessage(const LogMessage& message) {
    String formatted;
    
    // Add timestamp if enabled
    if (config_.enable_timestamp) {
        formatted += "[" + formatTimestamp(message.timestamp) + "] ";
    }
    
    // Add uptime if enabled
    if (config_.enable_uptime) {
        formatted += "[" + formatUptime(message.uptime) + "] ";
    }
    
    // Add level
    formatted += "[" + getLevelString(message.level) + "] ";
    
    // Add source if enabled
    if (config_.enable_source) {
        formatted += "[" + message.source + "] ";
    }
    
    // Add tag
    formatted += "[" + message.tag + "] ";
    
    // Add message
    formatted += message.message;
    
    return formatted;
}

String Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

String Logger::getLevelColor(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "\033[36m"; // Cyan
        case LogLevel::INFO:
            return "\033[32m"; // Green
        case LogLevel::WARN:
            return "\033[33m"; // Yellow
        case LogLevel::ERROR:
            return "\033[31m"; // Red
        case LogLevel::CRITICAL:
            return "\033[35m"; // Magenta
        default:
            return "\033[0m"; // Reset
    }
}

void Logger::addMessage(const LogMessage& message) {
    messages_.push_back(message);
    message_count_++;
    total_messages_++;
    
    // Remove oldest messages if buffer is full
    while (messages_.size() > config_.max_messages) {
        messages_.erase(messages_.begin());
    }
    
    updateStatistics(message.level);
}

void Logger::updateStatistics(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            debug_count_++;
            break;
        case LogLevel::INFO:
            info_count_++;
            break;
        case LogLevel::WARN:
            warn_count_++;
            break;
        case LogLevel::ERROR:
            error_count_++;
            break;
        case LogLevel::CRITICAL:
            critical_count_++;
            break;
    }
}

// ===== GLOBAL LOGGER FUNCTIONS =====

bool initializeLogger() {
    if (g_logger) {
        return true;
    }
    
    g_logger = new Logger();
    if (!g_logger) {
        return false;
    }
    
    return g_logger->initialize();
}

Logger* getLogger() {
    return g_logger;
}

void logDebug(const String& tag, const String& message) {
    if (g_logger) {
        g_logger->debug(tag, message);
    }
}

void logInfo(const String& tag, const String& message) {
    if (g_logger) {
        g_logger->info(tag, message);
    }
}

void logWarn(const String& tag, const String& message) {
    if (g_logger) {
        g_logger->warn(tag, message);
    }
}

void logError(const String& tag, const String& message) {
    if (g_logger) {
        g_logger->error(tag, message);
    }
}

void logCritical(const String& tag, const String& message) {
    if (g_logger) {
        g_logger->critical(tag, message);
    }
}

String getLogLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

LogLevel parseLogLevel(const String& level_string) {
    if (level_string.equalsIgnoreCase("DEBUG")) {
        return LogLevel::DEBUG;
    } else if (level_string.equalsIgnoreCase("INFO")) {
        return LogLevel::INFO;
    } else if (level_string.equalsIgnoreCase("WARN")) {
        return LogLevel::WARN;
    } else if (level_string.equalsIgnoreCase("ERROR")) {
        return LogLevel::ERROR;
    } else if (level_string.equalsIgnoreCase("CRITICAL")) {
        return LogLevel::CRITICAL;
    }
    return LogLevel::INFO;
}

String formatTimestamp(uint32_t timestamp) {
    uint32_t seconds = timestamp / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
    return String(buffer);
}

String formatUptime(uint32_t uptime_ms) {
    uint32_t seconds = uptime_ms / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    uint32_t days = hours / 24;
    
    if (days > 0) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%lud %02lu:%02lu:%02lu", days, hours % 24, minutes % 60, seconds % 60);
        return String(buffer);
    } else {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
        return String(buffer);
    }
} 