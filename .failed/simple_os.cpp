/**
 * @file simple_os.cpp
 * @brief T-Deck-Pro Simplified OS - Core System
 * @author T-Deck-Pro OS Team
 * @date 2025
 * 
 * Simplified OS Architecture:
 * 1. Boot system
 * 2. Hardware drivers initialization
 * 3. Launcher interface
 * 4. Essential settings management
 * 5. MQTT communication system
 * 6. Tailscale integration
 * 7. Plugin/app loader from SD card
 */

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

// Hardware drivers
#include "drivers/hardware_manager.h"
#include "drivers/display_driver.h"
#include "drivers/input_driver.h"
#include "drivers/connectivity_driver.h"
#include "drivers/storage_driver.h"

// Core OS components
#include "core/boot_manager.h"
#include "core/launcher.h"
#include "core/settings_manager.h"
#include "core/mqtt_manager.h"
#include "core/tailscale_manager.h"
#include "core/plugin_manager.h"

// Configuration
#include "config/os_config.h"

static const char* TAG = "SimpleOS";

// Global OS state
struct OSState {
    bool initialized = false;
    bool hardware_ready = false;
    bool connectivity_ready = false;
    bool mqtt_connected = false;
    bool tailscale_connected = false;
    uint32_t boot_time = 0;
    String current_app = "launcher";
} os_state;

// Core managers
BootManager boot_manager;
HardwareManager hardware_manager;
Launcher launcher;
SettingsManager settings_manager;
MQTTManager mqtt_manager;
TailscaleManager tailscale_manager;
PluginManager plugin_manager;

/**
 * @brief Initialize the simplified OS
 */
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== T-Deck-Pro Simplified OS Starting ===");
    
    os_state.boot_time = millis();
    
    // Phase 1: Boot Manager
    Serial.println("[BOOT] Starting boot sequence...");
    if (!boot_manager.initialize()) {
        Serial.println("[ERROR] Boot manager initialization failed!");
        esp_restart();
    }
    
    // Phase 2: Hardware Initialization
    Serial.println("[HARDWARE] Initializing hardware drivers...");
    if (!hardware_manager.initialize()) {
        Serial.println("[ERROR] Hardware initialization failed!");
        boot_manager.show_error("Hardware Init Failed");
        esp_restart();
    }
    os_state.hardware_ready = true;
    
    // Phase 3: Storage Systems
    Serial.println("[STORAGE] Initializing storage systems...");
    if (!StorageDriver::initialize()) {
        Serial.println("[WARNING] Storage initialization failed - limited functionality");
    }
    
    // Phase 4: Settings Manager
    Serial.println("[SETTINGS] Loading system settings...");
    if (!settings_manager.initialize()) {
        Serial.println("[WARNING] Settings manager failed - using defaults");
    }
    
    // Phase 5: Display and Input
    Serial.println("[DISPLAY] Initializing display and input...");
    if (!DisplayDriver::initialize() || !InputDriver::initialize()) {
        Serial.println("[ERROR] Display/Input initialization failed!");
        esp_restart();
    }
    
    // Phase 6: Connectivity
    Serial.println("[CONNECTIVITY] Initializing connectivity...");
    if (ConnectivityDriver::initialize()) {
        os_state.connectivity_ready = true;
        
        // Phase 7: MQTT Manager
        Serial.println("[MQTT] Starting MQTT manager...");
        if (mqtt_manager.initialize()) {
            os_state.mqtt_connected = true;
        }
        
        // Phase 8: Tailscale Manager
        Serial.println("[TAILSCALE] Starting Tailscale...");
        if (tailscale_manager.initialize()) {
            os_state.tailscale_connected = true;
        }
    } else {
        Serial.println("[WARNING] Connectivity failed - offline mode");
    }
    
    // Phase 9: Plugin Manager
    Serial.println("[PLUGINS] Initializing plugin manager...");
    if (!plugin_manager.initialize()) {
        Serial.println("[WARNING] Plugin manager failed - no external apps");
    }
    
    // Phase 10: Launcher
    Serial.println("[LAUNCHER] Starting launcher interface...");
    if (!launcher.initialize()) {
        Serial.println("[ERROR] Launcher initialization failed!");
        esp_restart();
    }
    
    os_state.initialized = true;
    uint32_t boot_duration = millis() - os_state.boot_time;
    Serial.printf("[BOOT] OS initialized successfully in %lu ms\n", boot_duration);
    
    // Show boot complete screen
    launcher.show_boot_complete(boot_duration);
}

/**
 * @brief Main OS loop
 */
void loop() {
    static uint32_t last_update = 0;
    uint32_t now = millis();
    
    // Update LVGL (UI framework)
    lv_timer_handler();
    
    // Update core managers every 100ms
    if (now - last_update > 100) {
        last_update = now;
        
        // Update hardware status
        hardware_manager.update();
        
        // Update connectivity
        if (os_state.connectivity_ready) {
            ConnectivityDriver::update();
        }
        
        // Update MQTT
        if (os_state.mqtt_connected) {
            mqtt_manager.update();
        }
        
        // Update Tailscale
        if (os_state.tailscale_connected) {
            tailscale_manager.update();
        }
        
        // Update current app/launcher
        if (os_state.current_app == "launcher") {
            launcher.update();
        } else {
            plugin_manager.update_current_app();
        }
        
        // Update plugin manager
        plugin_manager.update();
    }
    
    // Handle MQTT messages for app launching
    handle_mqtt_app_requests();
    
    // Yield to other tasks
    vTaskDelay(pdMS_TO_TICKS(10));
}

/**
 * @brief Handle MQTT messages that request app launches
 */
void handle_mqtt_app_requests() {
    if (!os_state.mqtt_connected) return;
    
    String pending_app = mqtt_manager.get_pending_app_request();
    if (pending_app.length() > 0) {
        Serial.printf("[MQTT] App launch request: %s\n", pending_app.c_str());
        
        if (pending_app == "launcher") {
            // Return to launcher
            plugin_manager.close_current_app();
            os_state.current_app = "launcher";
            launcher.show();
        } else {
            // Launch plugin/app
            if (plugin_manager.launch_app(pending_app)) {
                os_state.current_app = pending_app;
            } else {
                Serial.printf("[ERROR] Failed to launch app: %s\n", pending_app.c_str());
                // Send error back via MQTT
                mqtt_manager.send_app_launch_result(pending_app, false, "App not found");
            }
        }
    }
}

/**
 * @brief Get current OS state for debugging
 */
OSState get_os_state() {
    return os_state;
}

/**
 * @brief Emergency restart function
 */
void emergency_restart(const String& reason) {
    Serial.printf("[EMERGENCY] Restarting due to: %s\n", reason.c_str());
    boot_manager.show_error("Emergency Restart: " + reason);
    delay(3000);
    esp_restart();
}