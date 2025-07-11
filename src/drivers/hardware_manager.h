/**
 * @file      hardware_manager.h
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Hardware Abstraction Layer for T-Deck-Pro device
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <GxEPD2_BW.h>
#include <TouchDrvCSTXXX.hpp>
#include <TinyGPS++.h>
#include <RadioLib.h>
#include <XPowersLib.h>
#include <vector>
#include <algorithm>
#include "utilities.h"

// Forward declarations
class Logger;

/**
 * @brief Hardware initialization status
 */
enum class HardwareStatus {
    NOT_INITIALIZED,
    INITIALIZING,
    READY,
    ERROR,
    DISABLED
};

/**
 * @brief Hardware component identifiers
 */
enum class HardwareComponent {
    DISPLAY,
    TOUCH,
    KEYBOARD,
    LORA,
    GPS,
    CELLULAR_4G,
    WIFI,
    SD_CARD,
    BATTERY,
    GYROSCOPE,
    LIGHT_SENSOR,
    AUDIO,
    POWER_MANAGEMENT
};

/**
 * @brief Display refresh modes
 */
enum class DisplayRefreshMode {
    FULL,
    PARTIAL,
    FAST
};

/**
 * @brief Network connection types
 */
enum class NetworkType {
    NONE,
    WIFI,
    CELLULAR_4G,
    LORA
};

/**
 * @brief GPS data structure
 */
struct GPSData {
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    double speed = 0.0;
    uint32_t satellites = 0;
    bool valid = false;
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
};

/**
 * @brief Battery information structure
 */
struct BatteryInfo {
    float voltage = 0.0f;
    float current = 0.0f;
    uint8_t percentage = 0;
    bool charging = false;
    bool present = false;
    float temperature = 0.0f;
};

/**
 * @brief Sensor data structure
 */
struct SensorData {
    float accel_x = 0.0f;
    float accel_y = 0.0f;
    float accel_z = 0.0f;
    float gyro_x = 0.0f;
    float gyro_y = 0.0f;
    float gyro_z = 0.0f;
    uint16_t light_level = 0;
    uint16_t proximity = 0;
};

/**
 * @brief Touch event structure
 */
struct TouchEvent {
    uint16_t x = 0;
    uint16_t y = 0;
    bool pressed = false;
    bool released = false;
    uint8_t gesture = 0;
};

/**
 * @brief WiFi network information structure
 */
struct WiFiNetwork {
    String ssid;
    int32_t rssi = 0;
    uint8_t channel = 0;
    wifi_auth_mode_t encryption_type = WIFI_AUTH_OPEN;
    bool is_hidden = false;
};

/**
 * @brief Hardware Manager Class
 * 
 * Provides unified interface to all T-Deck-Pro hardware components
 */
class HardwareManager {
public:
    /**
     * @brief Get singleton instance
     */
    static HardwareManager& getInstance();

    /**
     * @brief Initialize all hardware components
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Shutdown all hardware components
     */
    void shutdown();

    /**
     * @brief Update hardware states (call in main loop)
     */
    void update();

    // === DISPLAY MANAGEMENT ===
    
    /**
     * @brief Initialize display
     * @return true if successful
     */
    bool initDisplay();

    /**
     * @brief Update display with text
     * @param text Text to display
     * @param x X coordinate
     * @param y Y coordinate
     * @return true if successful
     */
    bool updateDisplay(const char* text, int16_t x = 0, int16_t y = 0);

    /**
     * @brief Refresh display
     * @param mode Refresh mode (full/partial/fast)
     * @return true if successful
     */
    bool refreshDisplay(DisplayRefreshMode mode = DisplayRefreshMode::PARTIAL);

    /**
     * @brief Clear display
     */
    void clearDisplay();

    /**
     * @brief Set display power state
     * @param enabled true to enable, false to disable
     */
    void setDisplayPower(bool enabled);

    // === INPUT MANAGEMENT ===

    /**
     * @brief Check for button/key input
     * @return Key character or 0 if no input
     */
    char getKeyInput();

    /**
     * @brief Get touch event
     * @return TouchEvent structure
     */
    TouchEvent getTouchEvent();

    /**
     * @brief Check if touch is currently pressed
     * @return true if touch is pressed
     */
    bool isTouchPressed();

    // === SENSOR ACCESS ===

    /**
     * @brief Get battery information
     * @return BatteryInfo structure
     */
    BatteryInfo getBatteryInfo();

    /**
     * @brief Get GPS data
     * @return GPSData structure
     */
    GPSData getGPSData();

    /**
     * @brief Get sensor data (accelerometer, gyroscope, light)
     * @return SensorData structure
     */
    SensorData getSensorData();

    // === COMMUNICATION INTERFACES ===

    /**
     * @brief Initialize WiFi
     * @return true if successful
     */
    bool initWiFi();

    /**
     * @brief Connect to WiFi network
     * @param ssid Network SSID
     * @param password Network password
     * @return true if connected
     */
    bool connectWiFi(const char* ssid, const char* password);

    /**
     * @brief Check WiFi connection status
     * @return true if connected
     */
    bool isWiFiConnected();

    /**
     * @brief Get WiFi signal strength
     * @return RSSI value
     */
    int32_t getWiFiRSSI();

    /**
     * @brief Reconnect to WiFi using stored credentials
     * @return true if reconnected successfully
     */
    bool reconnectWiFi();

    /**
     * @brief Disconnect from WiFi
     */
    void disconnectWiFi();

    /**
     * @brief Scan for available WiFi networks
     * @return Vector of WiFiNetwork structures
     */
    std::vector<WiFiNetwork> scanWiFiNetworks();

    /**
     * @brief Set WiFi country code for regulatory compliance
     * @param country_code Two-letter country code (e.g., "US", "EU")
     */
    void setWiFiCountry(const char* country_code);

    /**
     * @brief Enable/disable WiFi power save mode
     * @param enable true to enable power save, false to disable
     */
    void setWiFiPowerSave(bool enable);

    /**
     * @brief Get detailed WiFi information
     * @return WiFi information as string
     */
    String getWiFiInfo();

    /**
     * @brief Initialize LoRa radio
     * @return true if successful
     */
    bool initLoRa();

    /**
     * @brief Send LoRa message
     * @param message Message to send
     * @param length Message length
     * @return true if sent successfully
     */
    bool sendLoRaMessage(const uint8_t* message, size_t length);

    /**
     * @brief Receive LoRa message
     * @param buffer Buffer to store message
     * @param max_length Maximum buffer length
     * @return Number of bytes received, 0 if no message
     */
    size_t receiveLoRaMessage(uint8_t* buffer, size_t max_length);

    /**
     * @brief Initialize 4G cellular modem
     * @return true if successful
     */
    bool init4G();

    /**
     * @brief Check 4G connection status
     * @return true if connected
     */
    bool is4GConnected();

    // === STORAGE ===

    /**
     * @brief Initialize SD card
     * @return true if successful
     */
    bool initSDCard();

    /**
     * @brief Check if SD card is present
     * @return true if SD card is available
     */
    bool isSDCardPresent();

    /**
     * @brief Get SD card size in MB
     * @return Size in megabytes
     */
    uint64_t getSDCardSize();

    // === POWER MANAGEMENT ===

    /**
     * @brief Set component power state
     * @param component Hardware component
     * @param enabled true to enable, false to disable
     */
    void setComponentPower(HardwareComponent component, bool enabled);

    /**
     * @brief Enter low power mode
     * @param duration_ms Duration in milliseconds (0 = indefinite)
     */
    void enterLowPowerMode(uint32_t duration_ms = 0);

    /**
     * @brief Wake up from low power mode
     */
    void wakeUp();

    // === STATUS AND DIAGNOSTICS ===

    /**
     * @brief Get component status
     * @param component Hardware component
     * @return Component status
     */
    HardwareStatus getComponentStatus(HardwareComponent component);

    /**
     * @brief Run hardware diagnostics
     * @return true if all components pass
     */
    bool runDiagnostics();

    /**
     * @brief Get hardware information string
     * @return Hardware info as string
     */
    String getHardwareInfo();

    /**
     * @brief Set logger instance
     * @param logger Logger instance pointer
     */
    void setLogger(Logger* logger);

private:
    // Singleton pattern
    HardwareManager() = default;
    ~HardwareManager() = default;
    HardwareManager(const HardwareManager&) = delete;
    HardwareManager& operator=(const HardwareManager&) = delete;

    // Hardware component instances
    GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>* display_driver = nullptr;
    TouchDrvCSTXXX* touch_driver = nullptr;
    SX1262* lora_radio = nullptr;
    TinyGPSPlus* gps_parser = nullptr;
    XPowersPPM* power_manager = nullptr;
    BQ27220* battery_monitor = nullptr;

    // Component status tracking
    HardwareStatus component_status[13];
    
    // Logger instance
    Logger* logger = nullptr;

    // Internal state
    bool initialized = false;
    uint32_t last_update = 0;
    
    // Private initialization methods
    bool initPowerManagement();
    bool initTouch();
    bool initKeyboard();
    bool initGPS();
    bool initBattery();
    bool initGyroscope();
    bool initLightSensor();
    bool initAudio();
    
    // Private utility methods
    void logError(const char* component, const char* message);
    void logInfo(const char* component, const char* message);
    bool enableComponentPower(HardwareComponent component);
    bool disableComponentPower(HardwareComponent component);
    void updateComponentStatus();
    String getWiFiStatusString(wl_status_t status);
};

// Global hardware manager instance
extern HardwareManager& Hardware;
