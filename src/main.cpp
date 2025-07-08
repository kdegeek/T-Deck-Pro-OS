/**
 * @file main.cpp
 * @brief T-Deck-Pro OS Main Application Entry Point
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

// Core OS Components
#include "core/hal/board_config.h"
#include "core/utils/logger.h"
#include "core/display/eink_manager.h"

// LVGL Configuration
#include "lvgl.h"

// Application Framework
#include "apps/app_manager.h"
#include "apps/meshtastic/meshtastic_app.h"

// Communication Stack
#include "comm/lora_manager.h"
#include "comm/wifi_manager.h"
#include "comm/cellular_manager.h"

// System Services
#include "services/power_manager.h"
#include "services/file_manager.h"
#include "services/ota_manager.h"

// ===== GLOBAL VARIABLES =====
static TaskHandle_t main_task_handle = NULL;
static TaskHandle_t ui_task_handle = NULL;
static TaskHandle_t comm_task_handle = NULL;

// ===== FUNCTION DECLARATIONS =====
void setup_hardware(void);
void setup_filesystem(void);
void setup_communication(void);
void setup_applications(void);
void main_task(void* parameter);
void ui_task(void* parameter);
void comm_task(void* parameter);

// ===== LVGL TICK CALLBACK =====
static void lv_tick_task(void* arg) {
    (void) arg;
    lv_tick_inc(portTICK_PERIOD_MS);
}

/**
 * @brief Arduino setup function - System initialization
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial && millis() < 5000) {
        delay(10);
    }
    
    Serial.println("\n=== T-Deck-Pro OS Starting ===");
    Serial.printf("Build: %s %s\n", __DATE__, __TIME__);
    Serial.printf("ESP32-S3 Chip: %s\n", ESP.getChipModel());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("PSRAM Size: %d MB\n", ESP.getPsramSize() / (1024 * 1024));
    
    // Initialize logging system
    log_config_t log_config = {
        .level = LOG_LEVEL_INFO,
        .destinations = LOG_DEST_SERIAL,
        .include_timestamp = true,
        .include_function = true,
        .include_line_number = true,
        .color_output = true,
        .log_file_path = "/logs/system.log",
        .buffer_size = 1024
    };
    
    if (!log_init(&log_config)) {
        Serial.println("WARNING: Failed to initialize logging system");
    }
    
    LOG_INFO("T-Deck-Pro OS initialization started");
    
    // Initialize hardware
    setup_hardware();
    
    // Initialize filesystem
    setup_filesystem();
    
    // Initialize LVGL
    lv_init();
    
    // Initialize E-ink display
    if (!eink_manager.initialize()) {
        LOG_ERROR("Failed to initialize E-ink display");
        while (1) {
            delay(1000);
        }
    }
    
    // Setup LVGL tick
    esp_timer_handle_t lvgl_tick_timer;
    esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lv_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, portTICK_PERIOD_MS * 1000);
    
    // Initialize communication systems
    setup_communication();
    
    // Initialize applications
    setup_applications();
    
    // Create main tasks
    xTaskCreatePinnedToCore(
        main_task,
        "main_task",
        8192,
        NULL,
        1,
        &main_task_handle,
        0
    );
    
    xTaskCreatePinnedToCore(
        ui_task,
        "ui_task",
        16384,
        NULL,
        2,
        &ui_task_handle,
        1
    );
    
    xTaskCreatePinnedToCore(
        comm_task,
        "comm_task",
        8192,
        NULL,
        1,
        &comm_task_handle,
        0
    );
    
    // Start E-ink maintenance task
    xTaskCreate(
        eink_maintenance_task,
        "eink_maintenance",
        4096,
        NULL,
        1,
        NULL
    );
    
    LOG_INFO("T-Deck-Pro OS initialization completed");
    Serial.println("=== System Ready ===\n");
}

/**
 * @brief Arduino loop function - Main system loop
 */
void loop() {
    // Main loop is handled by FreeRTOS tasks
    // This loop just handles watchdog and basic housekeeping
    
    static uint32_t last_heartbeat = 0;
    uint32_t current_time = millis();
    
    if (current_time - last_heartbeat >= 30000) { // 30 second heartbeat
        LOG_DEBUG("System heartbeat - Free heap: %d bytes", ESP.getFreeHeap());
        last_heartbeat = current_time;
    }
    
    // Feed watchdog
    esp_task_wdt_reset();
    
    // Small delay to prevent tight loop
    delay(100);
}

/**
 * @brief Initialize hardware components
 */
void setup_hardware() {
    LOG_INFO("Initializing hardware components");
    
    // Initialize board configuration
    if (!board_init()) {
        LOG_ERROR("Failed to initialize board hardware");
        while (1) delay(1000);
    }
    
    // Initialize power management
    if (!board_set_power_state(BOARD_POWER_ACTIVE)) {
        LOG_WARN("Failed to set initial power state");
    }
    
    // Check battery status
    uint16_t battery_mv = board_get_battery_voltage();
    bool usb_connected = board_is_usb_connected();
    
    LOG_INFO("Battery: %d mV, USB: %s", battery_mv, usb_connected ? "Connected" : "Disconnected");
    
    if (battery_mv < BOARD_BAT_CRIT_MV && !usb_connected) {
        LOG_ERROR("Critical battery level, entering deep sleep");
        esp_deep_sleep_start();
    }
    
    LOG_INFO("Hardware initialization completed");
}

/**
 * @brief Initialize filesystem
 */
void setup_filesystem() {
    LOG_INFO("Initializing filesystem");
    
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        LOG_ERROR("Failed to initialize SPIFFS");
        return;
    }
    
    // Create necessary directories
    if (!SPIFFS.exists("/apps")) {
        SPIFFS.mkdir("/apps");
    }
    if (!SPIFFS.exists("/logs")) {
        SPIFFS.mkdir("/logs");
    }
    if (!SPIFFS.exists("/config")) {
        SPIFFS.mkdir("/config");
    }
    if (!SPIFFS.exists("/data")) {
        SPIFFS.mkdir("/data");
    }
    
    // Log filesystem info
    size_t total_bytes = SPIFFS.totalBytes();
    size_t used_bytes = SPIFFS.usedBytes();
    LOG_INFO("SPIFFS: %d/%d bytes used (%.1f%%)", 
             used_bytes, total_bytes, (float)used_bytes / total_bytes * 100.0);
    
    LOG_INFO("Filesystem initialization completed");
}

/**
 * @brief Initialize communication systems
 */
void setup_communication() {
    LOG_INFO("Initializing communication systems");
    
    // Initialize WiFi (but don't connect yet)
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    // TODO: Initialize LoRa manager
    // TODO: Initialize cellular manager
    // TODO: Initialize Bluetooth
    
    LOG_INFO("Communication systems initialized");
}

/**
 * @brief Initialize applications
 */
void setup_applications() {
    LOG_INFO("Initializing applications");
    
    // TODO: Initialize application manager
    // TODO: Load installed applications
    // TODO: Initialize Meshtastic app
    
    LOG_INFO("Applications initialized");
}

/**
 * @brief Main system task
 */
void main_task(void* parameter) {
    LOG_INFO("Main task started");
    
    const TickType_t xDelay = pdMS_TO_TICKS(1000); // 1 second
    
    while (1) {
        // System monitoring and maintenance
        
        // Check memory usage
        size_t free_heap = ESP.getFreeHeap();
        size_t min_free_heap = ESP.getMinFreeHeap();
        
        if (free_heap < 50000) { // Less than 50KB free
            LOG_WARN("Low memory warning: %d bytes free", free_heap);
        }
        
        // Check power status
        uint16_t battery_mv = board_get_battery_voltage();
        if (battery_mv < BOARD_BAT_LOW_MV) {
            LOG_WARN("Low battery: %d mV", battery_mv);
            // TODO: Trigger power saving mode
        }
        
        // Periodic tasks
        static uint32_t task_counter = 0;
        task_counter++;
        
        if (task_counter % 60 == 0) { // Every minute
            log_flush();
        }
        
        if (task_counter % 300 == 0) { // Every 5 minutes
            // TODO: Sync with server
            // TODO: Check for OTA updates
        }
        
        vTaskDelay(xDelay);
    }
}

/**
 * @brief UI task - handles LVGL and display updates
 */
void ui_task(void* parameter) {
    LOG_INFO("UI task started");
    
    const TickType_t xDelay = pdMS_TO_TICKS(10); // 10ms for smooth UI
    
    while (1) {
        // Handle LVGL tasks
        lv_timer_handler();
        
        // Handle E-ink display updates
        eink_task_handler();
        
        vTaskDelay(xDelay);
    }
}

/**
 * @brief Communication task - handles all communication protocols
 */
void comm_task(void* parameter) {
    LOG_INFO("Communication task started");
    
    const TickType_t xDelay = pdMS_TO_TICKS(100); // 100ms
    
    while (1) {
        // TODO: Handle LoRa messages
        // TODO: Handle WiFi events
        // TODO: Handle cellular events
        // TODO: Handle Bluetooth events
        
        vTaskDelay(xDelay);
    }
}