/**
 * @file      simple_hardware.h
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Simplified Arduino-compatible hardware manager for Phase 1
 */

#ifndef SIMPLE_HARDWARE_H
#define SIMPLE_HARDWARE_H

#include <Arduino.h>
#include <WiFi.h>
#include <SD.h>
#include <Wire.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <TouchDrvCSTXXX.hpp>
#include "utilities.h"
#include "lvgl_integration.h"

enum HardwareStatus {
    HW_NOT_INITIALIZED,
    HW_INITIALIZING,
    HW_READY,
    HW_ERROR,
    HW_DISABLED
};

struct TouchPoint {
    int16_t x;
    int16_t y;
    bool pressed;
    uint32_t timestamp;
};

struct WiFiNetworkInfo {
    String ssid;
    int32_t rssi;
    uint8_t channel;
    wifi_auth_mode_t encryption;
};

struct BatteryInfo {
    float voltage;
    int percentage;
    bool charging;
    bool present;
};

class SimpleHardware {
private:
    static SimpleHardware* instance;
    
    // Hardware components
    GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>* display;
    TouchDrvCSTXXX* touch;
    
    // Component status
    HardwareStatus display_status;
    HardwareStatus touch_status;
    HardwareStatus wifi_status;
    HardwareStatus sd_status;
    
    // State tracking
    TouchPoint last_touch;
    uint32_t last_status_update;
    
    // Private constructor for singleton
    SimpleHardware();
    
    // Private initialization methods
    bool initDisplay();
    bool initTouch();
    bool initWiFi();
    bool initSD();
    bool initPowerManagement();

public:
    // Singleton access
    static SimpleHardware* getInstance();
    
    // System initialization
    bool init();
    void update();
    bool runDiagnostics();
    
    // Display management
    bool updateDisplay(const char* text, int16_t x = 10, int16_t y = 30);
    bool refreshDisplay(bool full_refresh = false);
    void clearDisplay();
    void displayStatus();

    // LVGL integration
    bool initLVGL();
    void updateLVGL();
    bool isLVGLReady();
    
    // Touch input
    TouchPoint getTouchInput();
    bool isTouchPressed();
    
    // WiFi management
    bool connectWiFi(const char* ssid, const char* password);
    void disconnectWiFi();
    bool isWiFiConnected();
    int scanWiFiNetworks(WiFiNetworkInfo* networks, int max_networks);
    String getWiFiInfo();
    int32_t getWiFiRSSI();
    
    // SD card management
    bool isSDCardAvailable();
    uint64_t getSDCardSize();
    uint64_t getSDCardUsed();
    
    // Battery management (stubbed for Phase 1)
    BatteryInfo getBatteryInfo();
    
    // Power management
    void enableComponent(const char* component, bool enabled);
    void setLowPowerMode(bool enabled);
    
    // Status and diagnostics
    HardwareStatus getComponentStatus(const char* component);
    String getSystemInfo();
    void printDiagnostics();
    
    // Utility
    uint32_t getUptime();
    uint32_t getFreeHeap();
    uint32_t getFreePSRAM();
};

// Global hardware manager instance
extern SimpleHardware* Hardware;

#endif // SIMPLE_HARDWARE_H
