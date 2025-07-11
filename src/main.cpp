/**
 * @file      main.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     T-Deck-Pro OS Main Application - Phase 1 Implementation
 */

#include <Arduino.h>
#include "utilities.h"

// Phase 1 Core Components
#include "drivers/hardware_manager.h"
#include "core/logger.h"
#include "core/power_manager.h"

// Configuration
#include "config/os_config.h"

// Global system state
bool system_initialized = false;
uint32_t last_status_update = 0;
uint32_t boot_time = 0;

// Test configuration (will be moved to config system in Phase 3)
const char* test_wifi_ssid = "YourWiFiNetwork";
const char* test_wifi_password = "YourPassword";

/**
 * @brief Initialize the T-Deck-Pro OS system
 * @return true if initialization successful
 */
bool initializeSystem() {
    LOG_INFO("System", "Starting T-Deck-Pro OS initialization...");

    // Phase 1: Initialize core components in order

    // 1. Initialize Logger first (for debugging)
    if (!Log.init()) {
        Serial.println("FATAL: Logger initialization failed");
        return false;
    }

    LOG_INFO("System", "Logger initialized successfully");

    // 2. Initialize Hardware Manager
    if (!Hardware.init()) {
        LOG_ERROR("System", "Hardware initialization failed");
        return false;
    }

    // Set logger reference in hardware manager
    Hardware.setLogger(&Log);
    LOG_INFO("System", "Hardware manager initialized successfully");

    // 3. Initialize Power Manager
    if (!Power.init()) {
        LOG_ERROR("System", "Power management initialization failed");
        return false;
    }

    // Set hardware reference in power manager
    Power.setHardwareManager(&Hardware);
    Power.setLogger(&Log);
    LOG_INFO("System", "Power manager initialized successfully");

    // 4. Run hardware diagnostics
    if (!Hardware.runDiagnostics()) {
        LOG_WARN("System", "Some hardware components failed diagnostics");
    }

    // 5. Display welcome message
    Hardware.updateDisplay("T-Deck-Pro OS\nPhase 1 Active\n\nInitializing...", 10, 30);
    Hardware.refreshDisplay();

    LOG_INFO("System", "System initialization completed successfully");
    return true;
}

/**
 * @brief Test WiFi connectivity
 */
void testWiFiConnection() {
    LOG_INFO("WiFi", "Testing WiFi connectivity...");

    // Scan for networks
    auto networks = Hardware.scanWiFiNetworks();
    LOG_INFO("WiFi", "Found %d networks", networks.size());

    // Try to connect to test network
    if (Hardware.connectWiFi(test_wifi_ssid, test_wifi_password)) {
        LOG_INFO("WiFi", "WiFi connection test successful");

        // Display connection info
        String wifi_info = Hardware.getWiFiInfo();
        Hardware.updateDisplay(wifi_info.c_str(), 10, 30);
        Hardware.refreshDisplay();

        delay(5000); // Show info for 5 seconds
    } else {
        LOG_ERROR("WiFi", "WiFi connection test failed");
        Hardware.updateDisplay("WiFi Test Failed\nCheck credentials", 10, 30);
        Hardware.refreshDisplay();
    }
}

/**
 * @brief Test power management features
 */
void testPowerManagement() {
    LOG_INFO("Power", "Testing power management...");

    // Get power statistics
    auto stats = Power.getPowerStats();

    String power_info = "Power Stats:\n";
    power_info += "Uptime: " + String(stats.uptime_ms / 1000) + "s\n";
    power_info += "CPU: " + String(stats.cpu_frequency_mhz) + "MHz\n";
    power_info += "Battery: " + String(stats.battery_percentage) + "%\n";
    power_info += "Mode: " + String((int)Power.getPowerMode()) + "\n";

    Hardware.updateDisplay(power_info.c_str(), 10, 30);
    Hardware.refreshDisplay();

    LOG_INFO("Power", "Power management test completed");
}

/**
 * @brief Update system status display
 */
void updateStatusDisplay() {
    uint32_t now = millis();

    if (now - last_status_update < 10000) { // Update every 10 seconds
        return;
    }
    last_status_update = now;

    // Get system information
    String status = "T-Deck-Pro OS\n";
    status += "Phase 1 Active\n\n";
    status += "Uptime: " + String((now - boot_time) / 1000) + "s\n";

    // Hardware status
    if (Hardware.isWiFiConnected()) {
        status += "WiFi: Connected\n";
        status += "RSSI: " + String(Hardware.getWiFiRSSI()) + "dBm\n";
    } else {
        status += "WiFi: Disconnected\n";
    }

    // Battery info
    auto battery = Hardware.getBatteryInfo();
    status += "Battery: " + String(battery.percentage) + "%\n";

    // Power mode
    status += "Power: Mode " + String((int)Power.getPowerMode()) + "\n";

    // Memory info
    status += "Free Heap: " + String(ESP.getFreeHeap() / 1024) + "KB\n";

    Hardware.updateDisplay(status.c_str(), 10, 30);
    Hardware.refreshDisplay();

    LOG_DEBUG("Status", "Status display updated");
}

/**
 * @brief Arduino setup function
 */
void setup() {
    // Record boot time
    boot_time = millis();

    // Initialize serial for early debugging
    Serial.begin(115200);
    delay(1000); // Allow serial to stabilize

    Serial.println("\n=== T-Deck-Pro OS Phase 1 ===");
    Serial.println("Initializing system...");

    // Initialize the system
    if (!initializeSystem()) {
        Serial.println("FATAL: System initialization failed!");
        Serial.println("System will halt.");

        // Try to show error on display if possible
        Hardware.updateDisplay("SYSTEM INIT\nFAILED!\n\nCheck Serial\nfor details", 10, 30);
        Hardware.refreshDisplay();

        // Halt system
        while (true) {
            delay(1000);
        }
    }

    system_initialized = true;

    // Set power mode to balanced for normal operation
    Power.setPowerMode(PowerMode::BALANCED);

    // Test WiFi connection (optional - comment out if no WiFi available)
    // testWiFiConnection();

    // Test power management
    testPowerManagement();

    LOG_INFO("System", "Setup completed successfully");
    LOG_INFO("System", "Entering main loop...");
}

/**
 * @brief Arduino main loop function
 */
void loop() {
    if (!system_initialized) {
        delay(100);
        return;
    }

    // Update core system components
    Hardware.update();
    Power.update();

    // Handle touch input
    TouchEvent touch_event = Hardware.getTouchEvent();
    if (touch_event.pressed) {
        LOG_INFO("Touch", "Touch detected at (%d, %d)", touch_event.x, touch_event.y);

        // Reset idle timer on user activity
        Power.resetIdleTimer();

        // Simple touch response - cycle through power modes
        static int power_mode_index = 0;
        PowerMode modes[] = {PowerMode::HIGH_PERFORMANCE, PowerMode::BALANCED, PowerMode::POWER_SAVE, PowerMode::ULTRA_LOW_POWER};

        power_mode_index = (power_mode_index + 1) % 4;
        Power.setPowerMode(modes[power_mode_index]);

        LOG_INFO("Touch", "Switched to power mode %d", power_mode_index);
    }

    // Update status display periodically
    updateStatusDisplay();

    // Check for any key input (if keyboard is available)
    char key = Hardware.getKeyInput();
    if (key != 0) {
        LOG_INFO("Keyboard", "Key pressed: %c", key);
        Power.resetIdleTimer();

        // Handle special keys
        switch (key) {
            case 'r': // Reset statistics
                Power.resetStats();
                LOG_INFO("System", "Statistics reset");
                break;
            case 'w': // Test WiFi
                testWiFiConnection();
                break;
            case 'p': // Test power management
                testPowerManagement();
                break;
            case 's': // Enter light sleep for 10 seconds
                LOG_INFO("System", "Entering light sleep for 10 seconds...");
                Hardware.updateDisplay("Sleeping for\n10 seconds...", 10, 30);
                Hardware.refreshDisplay();
                Power.enterLightSleep(10000);
                break;
        }
    }

    // Small delay to prevent overwhelming the system
    delay(50);
}

/**
 * @brief Phase 1 Implementation Notes:
 *
 * This is the Phase 1 implementation of T-Deck-Pro OS focusing on:
 * 1. Hardware Abstraction Layer (HAL) - Unified hardware interface
 * 2. Unified Logging System - Multi-output logging (Serial, SD, MQTT)
 * 3. Robust WiFi Management - Connection handling with retry logic
 * 4. Basic Power Management - ESP32-S3 power optimization
 * 5. Updated Library Dependencies - AsyncMqttClient, LittleFS, etc.
 *
 * Next phases will add:
 * - Phase 2: Core Services (MQTT, WireGuard, Boot Manager, Plugin Manager)
 * - Phase 3: Storage & Configuration System
 * - Phase 4: UI/UX Framework with LVGL optimization
 * - Phase 5: Application Framework
 * - Phase 6: Optimization & Production
 *
 * Usage:
 * - Touch screen to cycle through power modes
 * - Use keyboard keys: 'r' (reset stats), 'w' (test WiFi), 'p' (power test), 's' (sleep)
 * - Monitor serial output for detailed logging
 * - Status updates on e-paper display every 10 seconds
 */
