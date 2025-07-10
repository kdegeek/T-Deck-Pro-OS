/**
 * @file settings_app.cpp
 * @brief T-Deck-Pro Settings App - System Configuration and Preferences
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Provides system settings, preferences, and configuration management
 */

#include "settings_app.h"
#include "../core/utils/logger.h"
#include "../core/storage/storage_manager.h"
#include <ArduinoJson.h>

// ===== SETTINGS APP IMPLEMENTATION =====

SettingsApp::SettingsApp() : initialized_(false), current_section_(SettingsSection::SYSTEM),
                             selected_index_(0), config_changed_(false) {
    
    // Initialize settings
    loadDefaultSettings();
    
    // Initialize app info
    app_info_.name = "Settings";
    app_info_.description = "System configuration and preferences";
    app_info_.type = AppType::SYSTEM;
    app_info_.enabled = true;
    app_info_.memory_usage = 0;
    app_info_.last_used = 0;
}

SettingsApp::~SettingsApp() {
    if (initialized_) {
        cleanup();
    }
}

bool SettingsApp::init() {
    if (initialized_) {
        return true;
    }
    
    logInfo("SETTINGS", "Initializing settings app");
    
    // Initialize storage manager if not already done
    if (!getStorageManager()) {
        if (!initializeStorageManager()) {
            logError("SETTINGS", "Failed to initialize storage manager");
            return false;
        }
    }
    
    // Load configuration
    loadConfig();
    
    // Initialize UI
    if (!initUI()) {
        logError("SETTINGS", "Failed to initialize UI");
        return false;
    }
    
    initialized_ = true;
    app_info_.last_used = millis();
    
    logInfo("SETTINGS", "Settings app initialized successfully");
    return true;
}

void SettingsApp::run() {
    if (!initialized_) {
        if (!init()) {
            logError("SETTINGS", "Failed to initialize settings app");
            return;
        }
    }
    
    // Update app usage time
    app_info_.last_used = millis();
    
    // Process user input
    processInput();
    
    // Update display
    updateDisplay();
    
    // Save configuration if changed
    if (config_changed_) {
        saveConfig();
        config_changed_ = false;
    }
}

void SettingsApp::cleanup() {
    if (!initialized_) {
        return;
    }
    
    logInfo("SETTINGS", "Cleaning up settings app");
    
    // Save configuration
    saveConfig();
    
    initialized_ = false;
}

const char* SettingsApp::getName() {
    return "Settings";
}

bool SettingsApp::initUI() {
    // Create main UI elements
    // This will be implemented when LVGL is available
    
    logInfo("SETTINGS", "UI initialized");
    return true;
}

void SettingsApp::loadDefaultSettings() {
    // System settings
    system_settings_.display_brightness = 50;
    system_settings_.display_timeout = 30;
    system_settings_.sleep_timeout = 300;
    system_settings_.auto_sleep = true;
    system_settings_.sound_enabled = true;
    system_settings_.vibration_enabled = true;
    
    // Network settings
    network_settings_.wifi_enabled = true;
    network_settings_.wifi_ssid = "";
    network_settings_.wifi_password = "";
    network_settings_.auto_connect = true;
    network_settings_.mqtt_enabled = false;
    network_settings_.mqtt_server = "";
    network_settings_.mqtt_port = 1883;
    network_settings_.mqtt_username = "";
    network_settings_.mqtt_password = "";
    
    // Communication settings
    comm_settings_.lora_enabled = true;
    comm_settings_.lora_frequency = 915000000;
    comm_settings_.lora_power = 10;
    comm_settings_.lora_spreading_factor = 7;
    comm_settings_.lora_bandwidth = 125000;
    comm_settings_.cellular_enabled = false;
    comm_settings_.cellular_apn = "";
    comm_settings_.cellular_username = "";
    comm_settings_.cellular_password = "";
    
    // Application settings
    app_settings_.max_apps = 10;
    app_settings_.auto_start_apps = false;
    app_settings_.app_timeout = 60;
    app_settings_.debug_mode = false;
    app_settings_.log_level = LogLevel::INFO;
    
    // Power settings
    power_settings_.battery_saver = true;
    power_settings_.cpu_frequency = 240;
    power_settings_.backlight_auto = true;
    power_settings_.low_battery_threshold = 3.3;
    power_settings_.critical_battery_threshold = 3.0;
}

void SettingsApp::loadConfig() {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return;
    }
    
    DynamicJsonDocument doc(4096);
    if (storage->loadJSON("/config/settings.json", doc, StorageType::SPIFFS)) {
        // Load system settings
        if (doc.containsKey("system")) {
            JsonObject system = doc["system"];
            system_settings_.display_brightness = system["display_brightness"] | 50;
            system_settings_.display_timeout = system["display_timeout"] | 30;
            system_settings_.sleep_timeout = system["sleep_timeout"] | 300;
            system_settings_.auto_sleep = system["auto_sleep"] | true;
            system_settings_.sound_enabled = system["sound_enabled"] | true;
            system_settings_.vibration_enabled = system["vibration_enabled"] | true;
        }
        
        // Load network settings
        if (doc.containsKey("network")) {
            JsonObject network = doc["network"];
            network_settings_.wifi_enabled = network["wifi_enabled"] | true;
            network_settings_.wifi_ssid = network["wifi_ssid"] | "";
            network_settings_.wifi_password = network["wifi_password"] | "";
            network_settings_.auto_connect = network["auto_connect"] | true;
            network_settings_.mqtt_enabled = network["mqtt_enabled"] | false;
            network_settings_.mqtt_server = network["mqtt_server"] | "";
            network_settings_.mqtt_port = network["mqtt_port"] | 1883;
            network_settings_.mqtt_username = network["mqtt_username"] | "";
            network_settings_.mqtt_password = network["mqtt_password"] | "";
        }
        
        // Load communication settings
        if (doc.containsKey("communication")) {
            JsonObject comm = doc["communication"];
            comm_settings_.lora_enabled = comm["lora_enabled"] | true;
            comm_settings_.lora_frequency = comm["lora_frequency"] | 915000000;
            comm_settings_.lora_power = comm["lora_power"] | 10;
            comm_settings_.lora_spreading_factor = comm["lora_spreading_factor"] | 7;
            comm_settings_.lora_bandwidth = comm["lora_bandwidth"] | 125000;
            comm_settings_.cellular_enabled = comm["cellular_enabled"] | false;
            comm_settings_.cellular_apn = comm["cellular_apn"] | "";
            comm_settings_.cellular_username = comm["cellular_username"] | "";
            comm_settings_.cellular_password = comm["cellular_password"] | "";
        }
        
        // Load application settings
        if (doc.containsKey("application")) {
            JsonObject app = doc["application"];
            app_settings_.max_apps = app["max_apps"] | 10;
            app_settings_.auto_start_apps = app["auto_start_apps"] | false;
            app_settings_.app_timeout = app["app_timeout"] | 60;
            app_settings_.debug_mode = app["debug_mode"] | false;
            app_settings_.log_level = (LogLevel)(app["log_level"] | (int)LogLevel::INFO);
        }
        
        // Load power settings
        if (doc.containsKey("power")) {
            JsonObject power = doc["power"];
            power_settings_.battery_saver = power["battery_saver"] | true;
            power_settings_.cpu_frequency = power["cpu_frequency"] | 240;
            power_settings_.backlight_auto = power["backlight_auto"] | true;
            power_settings_.low_battery_threshold = power["low_battery_threshold"] | 3.3;
            power_settings_.critical_battery_threshold = power["critical_battery_threshold"] | 3.0;
        }
        
        logInfo("SETTINGS", "Configuration loaded");
    } else {
        logInfo("SETTINGS", "No configuration found, using defaults");
    }
}

void SettingsApp::saveConfig() {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return;
    }
    
    DynamicJsonDocument doc(4096);
    
    // Save system settings
    JsonObject system = doc.createNestedObject("system");
    system["display_brightness"] = system_settings_.display_brightness;
    system["display_timeout"] = system_settings_.display_timeout;
    system["sleep_timeout"] = system_settings_.sleep_timeout;
    system["auto_sleep"] = system_settings_.auto_sleep;
    system["sound_enabled"] = system_settings_.sound_enabled;
    system["vibration_enabled"] = system_settings_.vibration_enabled;
    
    // Save network settings
    JsonObject network = doc.createNestedObject("network");
    network["wifi_enabled"] = network_settings_.wifi_enabled;
    network["wifi_ssid"] = network_settings_.wifi_ssid;
    network["wifi_password"] = network_settings_.wifi_password;
    network["auto_connect"] = network_settings_.auto_connect;
    network["mqtt_enabled"] = network_settings_.mqtt_enabled;
    network["mqtt_server"] = network_settings_.mqtt_server;
    network["mqtt_port"] = network_settings_.mqtt_port;
    network["mqtt_username"] = network_settings_.mqtt_username;
    network["mqtt_password"] = network_settings_.mqtt_password;
    
    // Save communication settings
    JsonObject comm = doc.createNestedObject("communication");
    comm["lora_enabled"] = comm_settings_.lora_enabled;
    comm["lora_frequency"] = comm_settings_.lora_frequency;
    comm["lora_power"] = comm_settings_.lora_power;
    comm["lora_spreading_factor"] = comm_settings_.lora_spreading_factor;
    comm["lora_bandwidth"] = comm_settings_.lora_bandwidth;
    comm["cellular_enabled"] = comm_settings_.cellular_enabled;
    comm["cellular_apn"] = comm_settings_.cellular_apn;
    comm["cellular_username"] = comm_settings_.cellular_username;
    comm["cellular_password"] = comm_settings_.cellular_password;
    
    // Save application settings
    JsonObject app = doc.createNestedObject("application");
    app["max_apps"] = app_settings_.max_apps;
    app["auto_start_apps"] = app_settings_.auto_start_apps;
    app["app_timeout"] = app_settings_.app_timeout;
    app["debug_mode"] = app_settings_.debug_mode;
    app["log_level"] = (int)app_settings_.log_level;
    
    // Save power settings
    JsonObject power = doc.createNestedObject("power");
    power["battery_saver"] = power_settings_.battery_saver;
    power["cpu_frequency"] = power_settings_.cpu_frequency;
    power["backlight_auto"] = power_settings_.backlight_auto;
    power["low_battery_threshold"] = power_settings_.low_battery_threshold;
    power["critical_battery_threshold"] = power_settings_.critical_battery_threshold;
    
    if (storage->saveJSON("/config/settings.json", doc, StorageType::SPIFFS)) {
        logInfo("SETTINGS", "Configuration saved");
    } else {
        logError("SETTINGS", "Failed to save configuration");
    }
}

void SettingsApp::processInput() {
    // Handle keyboard input
    // This will be implemented when keyboard input is available
    
    // Handle touch input
    // This will be implemented when touch input is available
}

void SettingsApp::updateDisplay() {
    // Update display with current settings
    // This will be implemented when display manager is available
    
    // For now, just log the current state
    logDebug("SETTINGS", "Display updated - Section: " + String((int)current_section_));
}

// System settings methods
SystemSettings SettingsApp::getSystemSettings() const {
    return system_settings_;
}

void SettingsApp::setSystemSettings(const SystemSettings& settings) {
    system_settings_ = settings;
    config_changed_ = true;
    logInfo("SETTINGS", "System settings updated");
}

void SettingsApp::setDisplayBrightness(uint8_t brightness) {
    system_settings_.display_brightness = brightness;
    config_changed_ = true;
    logInfo("SETTINGS", "Display brightness set to: " + String(brightness));
}

void SettingsApp::setDisplayTimeout(uint16_t timeout) {
    system_settings_.display_timeout = timeout;
    config_changed_ = true;
    logInfo("SETTINGS", "Display timeout set to: " + String(timeout) + " seconds");
}

void SettingsApp::setSleepTimeout(uint16_t timeout) {
    system_settings_.sleep_timeout = timeout;
    config_changed_ = true;
    logInfo("SETTINGS", "Sleep timeout set to: " + String(timeout) + " seconds");
}

void SettingsApp::setAutoSleep(bool enabled) {
    system_settings_.auto_sleep = enabled;
    config_changed_ = true;
    logInfo("SETTINGS", "Auto sleep " + String(enabled ? "enabled" : "disabled"));
}

void SettingsApp::setSoundEnabled(bool enabled) {
    system_settings_.sound_enabled = enabled;
    config_changed_ = true;
    logInfo("SETTINGS", "Sound " + String(enabled ? "enabled" : "disabled"));
}

void SettingsApp::setVibrationEnabled(bool enabled) {
    system_settings_.vibration_enabled = enabled;
    config_changed_ = true;
    logInfo("SETTINGS", "Vibration " + String(enabled ? "enabled" : "disabled"));
}

// Network settings methods
NetworkSettings SettingsApp::getNetworkSettings() const {
    return network_settings_;
}

void SettingsApp::setNetworkSettings(const NetworkSettings& settings) {
    network_settings_ = settings;
    config_changed_ = true;
    logInfo("SETTINGS", "Network settings updated");
}

void SettingsApp::setWiFiEnabled(bool enabled) {
    network_settings_.wifi_enabled = enabled;
    config_changed_ = true;
    logInfo("SETTINGS", "WiFi " + String(enabled ? "enabled" : "disabled"));
}

void SettingsApp::setWiFiCredentials(const String& ssid, const String& password) {
    network_settings_.wifi_ssid = ssid;
    network_settings_.wifi_password = password;
    config_changed_ = true;
    logInfo("SETTINGS", "WiFi credentials updated");
}

void SettingsApp::setMQTTEnabled(bool enabled) {
    network_settings_.mqtt_enabled = enabled;
    config_changed_ = true;
    logInfo("SETTINGS", "MQTT " + String(enabled ? "enabled" : "disabled"));
}

void SettingsApp::setMQTTServer(const String& server, uint16_t port) {
    network_settings_.mqtt_server = server;
    network_settings_.mqtt_port = port;
    config_changed_ = true;
    logInfo("SETTINGS", "MQTT server set to: " + server + ":" + String(port));
}

// Communication settings methods
CommSettings SettingsApp::getCommSettings() const {
    return comm_settings_;
}

void SettingsApp::setCommSettings(const CommSettings& settings) {
    comm_settings_ = settings;
    config_changed_ = true;
    logInfo("SETTINGS", "Communication settings updated");
}

void SettingsApp::setLoRaEnabled(bool enabled) {
    comm_settings_.lora_enabled = enabled;
    config_changed_ = true;
    logInfo("SETTINGS", "LoRa " + String(enabled ? "enabled" : "disabled"));
}

void SettingsApp::setLoRaFrequency(uint32_t frequency) {
    comm_settings_.lora_frequency = frequency;
    config_changed_ = true;
    logInfo("SETTINGS", "LoRa frequency set to: " + String(frequency));
}

void SettingsApp::setLoRaPower(int8_t power) {
    comm_settings_.lora_power = power;
    config_changed_ = true;
    logInfo("SETTINGS", "LoRa power set to: " + String(power) + " dBm");
}

void SettingsApp::setCellularEnabled(bool enabled) {
    comm_settings_.cellular_enabled = enabled;
    config_changed_ = true;
    logInfo("SETTINGS", "Cellular " + String(enabled ? "enabled" : "disabled"));
}

// Application settings methods
AppSettings SettingsApp::getAppSettings() const {
    return app_settings_;
}

void SettingsApp::setAppSettings(const AppSettings& settings) {
    app_settings_ = settings;
    config_changed_ = true;
    logInfo("SETTINGS", "Application settings updated");
}

void SettingsApp::setMaxApps(uint8_t max_apps) {
    app_settings_.max_apps = max_apps;
    config_changed_ = true;
    logInfo("SETTINGS", "Max apps set to: " + String(max_apps));
}

void SettingsApp::setDebugMode(bool enabled) {
    app_settings_.debug_mode = enabled;
    config_changed_ = true;
    logInfo("SETTINGS", "Debug mode " + String(enabled ? "enabled" : "disabled"));
}

void SettingsApp::setLogLevel(LogLevel level) {
    app_settings_.log_level = level;
    config_changed_ = true;
    logInfo("SETTINGS", "Log level set to: " + String((int)level));
}

// Power settings methods
PowerSettings SettingsApp::getPowerSettings() const {
    return power_settings_;
}

void SettingsApp::setPowerSettings(const PowerSettings& settings) {
    power_settings_ = settings;
    config_changed_ = true;
    logInfo("SETTINGS", "Power settings updated");
}

void SettingsApp::setBatterySaver(bool enabled) {
    power_settings_.battery_saver = enabled;
    config_changed_ = true;
    logInfo("SETTINGS", "Battery saver " + String(enabled ? "enabled" : "disabled"));
}

void SettingsApp::setCPUFrequency(uint32_t frequency) {
    power_settings_.cpu_frequency = frequency;
    config_changed_ = true;
    logInfo("SETTINGS", "CPU frequency set to: " + String(frequency) + " MHz");
}

void SettingsApp::setBatteryThresholds(float low, float critical) {
    power_settings_.low_battery_threshold = low;
    power_settings_.critical_battery_threshold = critical;
    config_changed_ = true;
    logInfo("SETTINGS", "Battery thresholds set to: " + String(low) + "V / " + String(critical) + "V");
}

// UI methods
SettingsSection SettingsApp::getCurrentSection() const {
    return current_section_;
}

void SettingsApp::setCurrentSection(SettingsSection section) {
    current_section_ = section;
    selected_index_ = 0;
    logDebug("SETTINGS", "Switched to section: " + String((int)section));
}

uint8_t SettingsApp::getSelectedIndex() const {
    return selected_index_;
}

void SettingsApp::setSelectedIndex(uint8_t index) {
    selected_index_ = index;
}

String SettingsApp::getSettingsSummary() {
    DynamicJsonDocument doc(2048);
    
    // System summary
    JsonObject system = doc.createNestedObject("system");
    system["brightness"] = system_settings_.display_brightness;
    system["display_timeout"] = system_settings_.display_timeout;
    system["sleep_timeout"] = system_settings_.sleep_timeout;
    system["auto_sleep"] = system_settings_.auto_sleep;
    system["sound"] = system_settings_.sound_enabled;
    system["vibration"] = system_settings_.vibration_enabled;
    
    // Network summary
    JsonObject network = doc.createNestedObject("network");
    network["wifi_enabled"] = network_settings_.wifi_enabled;
    network["mqtt_enabled"] = network_settings_.mqtt_enabled;
    
    // Communication summary
    JsonObject comm = doc.createNestedObject("communication");
    comm["lora_enabled"] = comm_settings_.lora_enabled;
    comm["cellular_enabled"] = comm_settings_.cellular_enabled;
    
    // Application summary
    JsonObject app = doc.createNestedObject("application");
    app["max_apps"] = app_settings_.max_apps;
    app["debug_mode"] = app_settings_.debug_mode;
    app["log_level"] = (int)app_settings_.log_level;
    
    // Power summary
    JsonObject power = doc.createNestedObject("power");
    power["battery_saver"] = power_settings_.battery_saver;
    power["cpu_frequency"] = power_settings_.cpu_frequency;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ===== GLOBAL SETTINGS APP INSTANCE =====
SettingsApp* g_settings_app = nullptr;

// ===== SETTINGS APP UTILITY FUNCTIONS =====

bool initializeSettingsApp() {
    if (g_settings_app) {
        return true;
    }
    
    g_settings_app = new SettingsApp();
    if (!g_settings_app) {
        return false;
    }
    
    return g_settings_app->init();
}

SettingsApp* getSettingsApp() {
    return g_settings_app;
} 