/**
 * @file      logger.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Unified Logging System Implementation
 */

#include "logger.h"
#include "../drivers/hardware_manager.h"
#include <SD.h>
#include <SPIFFS.h>
#include <time.h>

// Global logger instance
Logger& Log = Logger::getInstance();

// === LOGGER MAIN CLASS ===

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

bool Logger::init() {
    if (initialized) {
        return true;
    }

    std::lock_guard<std::mutex> lock(log_mutex);

    // Initialize default output handlers
    initializeDefaultHandlers();

    // Initialize all enabled output handlers
    for (auto& handler : output_handlers) {
        if (handler) {
            handler->init();
        }
    }

    initialized = true;
    
    // Log initialization message
    info("Logger", "Logging system initialized successfully");
    
    return true;
}

void Logger::shutdown() {
    if (!initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(log_mutex);

    info("Logger", "Shutting down logging system");
    
    // Flush all handlers
    for (auto& handler : output_handlers) {
        if (handler) {
            handler->flush();
        }
    }

    output_handlers.clear();
    initialized = false;
}

void Logger::log(LogLevel level, const char* component, const char* format, ...) {
    if (!initialized || level < min_level) {
        return;
    }

    va_list args;
    va_start(args, format);
    logv(level, component, format, args);
    va_end(args);
}

void Logger::logv(LogLevel level, const char* component, const char* format, va_list args) {
    if (!initialized || level < min_level) {
        return;
    }

    std::lock_guard<std::mutex> lock(log_mutex);

    // Create log message
    LogMessage message;
    message.timestamp = millis();
    message.level = level;
    message.component = String(component);

    // Format message
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    message.message = String(buffer);
    message.formatted_message = formatMessage(message);

    // Update statistics
    stats.total_messages++;
    switch (level) {
        case LogLevel::DEBUG: stats.debug_count++; break;
        case LogLevel::INFO:  stats.info_count++; break;
        case LogLevel::WARN:  stats.warn_count++; break;
        case LogLevel::ERROR: stats.error_count++; break;
        case LogLevel::FATAL: stats.fatal_count++; break;
    }

    // Add to history
    addToHistory(message);

    // Write to all enabled outputs
    writeToOutputs(message);
}

void Logger::debug(const char* component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logv(LogLevel::DEBUG, component, format, args);
    va_end(args);
}

void Logger::info(const char* component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logv(LogLevel::INFO, component, format, args);
    va_end(args);
}

void Logger::warn(const char* component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logv(LogLevel::WARN, component, format, args);
    va_end(args);
}

void Logger::error(const char* component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logv(LogLevel::ERROR, component, format, args);
    va_end(args);
}

void Logger::fatal(const char* component, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logv(LogLevel::FATAL, component, format, args);
    va_end(args);
}

void Logger::setLevel(LogLevel min_level) {
    this->min_level = min_level;
}

void Logger::enableOutput(LogOutput output, bool enabled) {
    output_enabled[(int)output] = enabled;
}

void Logger::setOutputs(bool serial, bool sd, bool mqtt, bool display) {
    output_enabled[(int)LogOutput::SERIAL] = serial;
    output_enabled[(int)LogOutput::SD_CARD] = sd;
    output_enabled[(int)LogOutput::MQTT] = mqtt;
    output_enabled[(int)LogOutput::DISPLAY] = display;
}

void Logger::addOutputHandler(std::unique_ptr<LogOutputHandler> handler) {
    if (handler) {
        std::lock_guard<std::mutex> lock(log_mutex);
        output_handlers.push_back(std::move(handler));
    }
}

void Logger::setHardwareManager(HardwareManager* hw) {
    hardware_manager = hw;
    
    // Update display handler if it exists
    for (auto& handler : output_handlers) {
        DisplayLogHandler* display_handler = dynamic_cast<DisplayLogHandler*>(handler.get());
        if (display_handler) {
            display_handler->setHardwareManager(hw);
        }
    }
}

void Logger::setMQTTClient(void* mqtt_client) {
    this->mqtt_client = mqtt_client;
    
    // Update MQTT handler if it exists
    for (auto& handler : output_handlers) {
        MQTTLogHandler* mqtt_handler = dynamic_cast<MQTTLogHandler*>(handler.get());
        if (mqtt_handler) {
            mqtt_handler->setMQTTClient(mqtt_client);
        }
    }
}

void Logger::flush() {
    if (!initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(log_mutex);
    
    for (auto& handler : output_handlers) {
        if (handler) {
            handler->flush();
        }
    }
}

std::vector<LogMessage> Logger::getRecentMessages(size_t count) {
    std::lock_guard<std::mutex> lock(log_mutex);
    
    if (count >= message_history.size()) {
        return message_history;
    }
    
    return std::vector<LogMessage>(
        message_history.end() - count,
        message_history.end()
    );
}

void Logger::clearHistory() {
    std::lock_guard<std::mutex> lock(log_mutex);
    message_history.clear();
}

Logger::LogStats Logger::getStatistics() {
    std::lock_guard<std::mutex> lock(log_mutex);
    return stats;
}

// === PRIVATE METHODS ===

void Logger::writeToOutputs(const LogMessage& message) {
    for (size_t i = 0; i < output_handlers.size(); i++) {
        if (output_handlers[i] && output_handlers[i]->isAvailable()) {
            if (!output_handlers[i]->write(message)) {
                stats.dropped_messages++;
            }
        }
    }
}

String Logger::formatMessage(const LogMessage& message) {
    char timestamp_str[32];
    uint32_t seconds = message.timestamp / 1000;
    uint32_t milliseconds = message.timestamp % 1000;
    
    snprintf(timestamp_str, sizeof(timestamp_str), "%lu.%03lu", seconds, milliseconds);
    
    String formatted = "[" + String(timestamp_str) + "] ";
    formatted += "[" + levelToString(message.level) + "] ";
    formatted += "[" + message.component + "] ";
    formatted += message.message;
    
    return formatted;
}

String Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKN ";
    }
}

void Logger::addToHistory(const LogMessage& message) {
    message_history.push_back(message);
    
    // Limit history size
    if (message_history.size() > max_history_size) {
        message_history.erase(message_history.begin());
    }
}

void Logger::initializeDefaultHandlers() {
    // Always add serial handler
    auto serial_handler = std::make_unique<SerialLogHandler>();
    output_handlers.push_back(std::move(serial_handler));
    
    // Add SD handler if enabled
    if (output_enabled[(int)LogOutput::SD_CARD]) {
        auto sd_handler = std::make_unique<SDLogHandler>();
        output_handlers.push_back(std::move(sd_handler));
    }
    
    // Add MQTT handler if enabled
    if (output_enabled[(int)LogOutput::MQTT]) {
        auto mqtt_handler = std::make_unique<MQTTLogHandler>();
        if (mqtt_client) {
            mqtt_handler->setMQTTClient(mqtt_client);
        }
        output_handlers.push_back(std::move(mqtt_handler));
    }
    
    // Add display handler if enabled
    if (output_enabled[(int)LogOutput::DISPLAY]) {
        auto display_handler = std::make_unique<DisplayLogHandler>();
        if (hardware_manager) {
            display_handler->setHardwareManager(hardware_manager);
        }
        output_handlers.push_back(std::move(display_handler));
    }
}

// === SERIAL LOG HANDLER ===

bool SerialLogHandler::init() {
    if (!initialized) {
        Serial.begin(115200);
        initialized = true;
    }
    return true;
}

bool SerialLogHandler::write(const LogMessage& message) {
    if (!initialized) {
        return false;
    }

    Serial.println(message.formatted_message);
    return true;
}

void SerialLogHandler::flush() {
    if (initialized) {
        Serial.flush();
    }
}

bool SerialLogHandler::isAvailable() {
    return initialized;
}

// === SD LOG HANDLER ===

SDLogHandler::SDLogHandler(const char* log_file_path)
    : log_file_path(log_file_path) {
}

SDLogHandler::~SDLogHandler() {
    flush();
}

bool SDLogHandler::init() {
    if (initialized) {
        return true;
    }

    // Check if SD card is available
    if (!SD.begin()) {
        return false;
    }

    // Create logs directory if it doesn't exist
    String dir_path = log_file_path.substring(0, log_file_path.lastIndexOf('/'));
    if (!SD.exists(dir_path)) {
        SD.mkdir(dir_path);
    }

    initialized = true;
    return true;
}

bool SDLogHandler::write(const LogMessage& message) {
    if (!initialized) {
        return false;
    }

    // Check file size and rotate if necessary
    if (SD.exists(log_file_path)) {
        File file = SD.open(log_file_path, FILE_READ);
        if (file && file.size() > max_file_size) {
            file.close();
            rotateLogFiles();
        } else if (file) {
            file.close();
        }
    }

    // Write to log file
    File file = SD.open(log_file_path, FILE_APPEND);
    if (file) {
        file.println(message.formatted_message);
        file.close();
        return true;
    }

    return false;
}

void SDLogHandler::flush() {
    // SD card writes are synchronous, no buffering needed
}

bool SDLogHandler::isAvailable() {
    return initialized && SD.begin();
}

void SDLogHandler::setMaxFileSize(size_t max_size) {
    max_file_size = max_size;
}

void SDLogHandler::setMaxFiles(uint8_t max_files) {
    this->max_files = max_files;
}

void SDLogHandler::rotateLogFiles() {
    // Rotate log files: system.log -> system.log.1 -> system.log.2 -> ... -> system.log.N
    String base_path = log_file_path;

    // Remove oldest file
    String oldest_file = base_path + "." + String(max_files);
    if (SD.exists(oldest_file)) {
        SD.remove(oldest_file);
    }

    // Rotate existing files
    for (int i = max_files - 1; i >= 1; i--) {
        String old_file = base_path + "." + String(i);
        String new_file = base_path + "." + String(i + 1);

        if (SD.exists(old_file)) {
            SD.rename(old_file, new_file);
        }
    }

    // Move current log to .1
    if (SD.exists(base_path)) {
        String backup_file = base_path + ".1";
        SD.rename(base_path, backup_file);
    }
}

String SDLogHandler::getCurrentLogFileName() {
    return log_file_path;
}

// === MQTT LOG HANDLER ===

MQTTLogHandler::MQTTLogHandler(const char* topic_prefix)
    : topic_prefix(topic_prefix) {
}

bool MQTTLogHandler::init() {
    initialized = (mqtt_client != nullptr);
    return initialized;
}

bool MQTTLogHandler::write(const LogMessage& message) {
    if (!initialized || !mqtt_client) {
        return false;
    }

    // Create topic based on log level
    String topic = topic_prefix + "/" + message.component.c_str();
    topic.toLowerCase();

    // For now, we'll just return true since MQTT client integration
    // will be completed in Phase 2
    return true;
}

void MQTTLogHandler::flush() {
    // MQTT messages are sent immediately
}

bool MQTTLogHandler::isAvailable() {
    return initialized && mqtt_client != nullptr;
}

void MQTTLogHandler::setMQTTClient(void* client) {
    mqtt_client = client;
    initialized = (client != nullptr);
}

// === DISPLAY LOG HANDLER ===

DisplayLogHandler::DisplayLogHandler() {
}

bool DisplayLogHandler::init() {
    initialized = (hardware != nullptr);
    return initialized;
}

bool DisplayLogHandler::write(const LogMessage& message) {
    if (!initialized || !hardware) {
        return false;
    }

    // Only show ERROR and FATAL messages on display
    if (message.level < LogLevel::ERROR) {
        return true;
    }

    // Add to display buffer
    display_buffer.push_back(message.formatted_message);

    // Limit buffer size
    if (display_buffer.size() > max_lines) {
        display_buffer.erase(display_buffer.begin());
    }

    // Update display with recent messages
    String display_text = "";
    for (const auto& line : display_buffer) {
        display_text += line + "\n";
    }

    hardware->updateDisplay(display_text.c_str());

    return true;
}

void DisplayLogHandler::flush() {
    // Display updates are immediate
}

bool DisplayLogHandler::isAvailable() {
    return initialized && hardware != nullptr;
}

void DisplayLogHandler::setHardwareManager(HardwareManager* hw) {
    hardware = hw;
    initialized = (hw != nullptr);
}
