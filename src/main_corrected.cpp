/**
 * @file main.cpp
 * @brief T-Deck-Pro OS - Main Entry Point - CORRECTED VERSION
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Completely rewritten to implement actual OS functionality
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_sleep.h>
#include <esp_partition.h>

// Core system includes - using corrected configurations
#include "config/os_config_corrected.h"
#include "core/hal/board_config_corrected.h"
#include "core/boot_manager.h"
#include "drivers/hardware_manager.h"
#include "core/display/eink_manager.h"
#include "core/communication/wifi_manager.h"
#include "core/storage/storage_manager.h"
#include "core/ui/launcher.h"
#include "core/plugin_manager.h"

// System state management
#include "core/system_state.h"
#include "core/error_handler.h"
#include "core/watchdog.h"

// Logging tag
static const char *TAG = "T-DECK-PRO-OS";

// ===== GLOBAL SYSTEM OBJECTS =====
static BootManager* g_boot_manager = nullptr;
static HardwareManager* g_hardware_manager = nullptr;
static EinkManager* g_display_manager = nullptr;
static WiFiManager* g_wifi_manager = nullptr;
static StorageManager* g_storage_manager = nullptr;
static Launcher* g_launcher = nullptr;
static PluginManager* g_plugin_manager = nullptr;

// System state
static volatile bool g_system_initialized = false;
static volatile bool g_system_running = false;
static uint32_t g_boot_start_time = 0;

// ===== FORWARD DECLARATIONS =====
void system_panic(const char* reason);
bool initialize_core_systems();
bool initialize_hardware();
bool initialize_display();
bool initialize_communication();
bool initialize_applications();
void system_main_loop();
void handle_system_events();
void handle_power_management();
void print_system_info();

/**
 * @brief System panic handler - display error and halt
 */
void system_panic(const char* reason) {
    ESP_LOGE(TAG, "SYSTEM PANIC: %s", reason);
    
    // Try to display error on screen if possible
    if (g_display_manager && g_display_manager->isInitialized()) {
        g_display_manager->showError("SYSTEM PANIC", reason);
        g_display_manager->refresh();
    }
    
    // Flash LED pattern to indicate panic
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < 20; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
    }
    
    // Force restart after delay
    delay(5000);
    ESP.restart();
}

/**
 * @brief Initialize core system components
 */
bool initialize_core_systems() {
    ESP_LOGI(TAG, "Initializing core systems...");
    
    // Initialize error handler first
    if (!ErrorHandler::initialize()) {
        ESP_LOGE(TAG, "Failed to initialize error handler");
        return false;
    }
    
    // Initialize watchdog
    if (!Watchdog::initialize(WATCHDOG_TIMEOUT_SEC)) {
        ESP_LOGE(TAG, "Failed to initialize watchdog");
        return false;
    }
    
    // Initialize system state manager
    if (!SystemState::initialize()) {
        ESP_LOGE(TAG, "Failed to initialize system state");
        return false;
    }
    
    ESP_LOGI(TAG, "Core systems initialized successfully");
    return true;
}

/**
 * @brief Initialize hardware subsystem with corrected configuration
 */
bool initialize_hardware() {
    ESP_LOGI(TAG, "Initializing hardware subsystem...");
    
    // Create hardware manager with corrected configuration
    g_hardware_manager = new HardwareManager();
    if (!g_hardware_manager) {
        ESP_LOGE(TAG, "Failed to create hardware manager");
        return false;
    }
    
    // Initialize hardware with corrected pins
    if (!g_hardware_manager->initialize()) {
        ESP_LOGE(TAG, "Failed to initialize hardware manager");
        delete g_hardware_manager;
        g_hardware_manager = nullptr;
        return false;
    }
    
    // Validate hardware configuration
    if (!g_hardware_manager->validateConfiguration()) {
        ESP_LOGE(TAG, "Hardware configuration validation failed");
        return false;
    }
    
    // Initialize power management
    if (!g_hardware_manager->initializePowerManagement()) {
        ESP_LOGE(TAG, "Failed to initialize power management");
        return false;
    }
    
    ESP_LOGI(TAG, "Hardware subsystem initialized successfully");
    return true;
}

/**
 * @brief Initialize display subsystem with corrected E-ink configuration
 */
bool initialize_display() {
    ESP_LOGI(TAG, "Initializing display subsystem...");
    
    // Create display manager with corrected configuration
    g_display_manager = new EinkManager();
    if (!g_display_manager) {
        ESP_LOGE(TAG, "Failed to create display manager");
        return false;
    }
    
    // Initialize E-ink display with correct hardware model and pins
    if (!g_display_manager->initialize()) {
        ESP_LOGE(TAG, "Failed to initialize display manager");
        delete g_display_manager;
        g_display_manager = nullptr;
        return false;
    }
    
    // Show boot splash screen
    g_display_manager->showBootSplash("T-Deck-Pro OS", "Initializing...");
    g_display_manager->refresh();
    
    ESP_LOGI(TAG, "Display subsystem initialized successfully");
    return true;
}

/**
 * @brief Initialize communication subsystems
 */
bool initialize_communication() {
    ESP_LOGI(TAG, "Initializing communication subsystems...");
    
    // Initialize WiFi manager
    g_wifi_manager = new WiFiManager();
    if (!g_wifi_manager) {
        ESP_LOGE(TAG, "Failed to create WiFi manager");
        return false;
    }
    
    if (!g_wifi_manager->initialize()) {
        ESP_LOGE(TAG, "Failed to initialize WiFi manager");
        return false;
    }
    
    // Initialize 4G modem (if available)
    if (g_hardware_manager->isModemAvailable()) {
        if (!g_hardware_manager->initializeModem()) {
            ESP_LOGW(TAG, "4G modem initialization failed, continuing without it");
        }
    }
    
    // Initialize LoRa (if available)
    if (g_hardware_manager->isLoRaAvailable()) {
        if (!g_hardware_manager->initializeLoRa()) {
            ESP_LOGW(TAG, "LoRa initialization failed, continuing without it");
        }
    }
    
    ESP_LOGI(TAG, "Communication subsystems initialized successfully");
    return true;
}

/**
 * @brief Initialize application layer
 */
bool initialize_applications() {
    ESP_LOGI(TAG, "Initializing application layer...");
    
    // Initialize storage manager
    g_storage_manager = new StorageManager();
    if (!g_storage_manager || !g_storage_manager->initialize()) {
        ESP_LOGE(TAG, "Failed to initialize storage manager");
        return false;
    }
    
    // Initialize plugin manager
    g_plugin_manager = new PluginManager();
    if (!g_plugin_manager || !g_plugin_manager->initialize()) {
        ESP_LOGE(TAG, "Failed to initialize plugin manager");
        return false;
    }
    
    // Initialize launcher UI
    g_launcher = new Launcher(g_display_manager);
    if (!g_launcher || !g_launcher->initialize()) {
        ESP_LOGE(TAG, "Failed to initialize launcher");
        return false;
    }
    
    // Load and start core plugins
    if (!g_plugin_manager->loadCorePlugins()) {
        ESP_LOGW(TAG, "Some core plugins failed to load");
    }
    
    ESP_LOGI(TAG, "Application layer initialized successfully");
    return true;
}

/**
 * @brief Print comprehensive system information
 */
void print_system_info() {
    ESP_LOGI(TAG, "=== T-DECK-PRO OS SYSTEM INFO ===");
    ESP_LOGI(TAG, "OS Version: %s", OS_VERSION);
    ESP_LOGI(TAG, "Build Date: %s %s", __DATE__, __TIME__);
    ESP_LOGI(TAG, "ESP32-S3 Chip: Rev %d", ESP.getChipRevision());
    ESP_LOGI(TAG, "CPU Frequency: %d MHz", ESP.getCpuFreqMHz());
    ESP_LOGI(TAG, "Flash Size: %d MB", ESP.getFlashChipSize() / (1024 * 1024));
    ESP_LOGI(TAG, "PSRAM Size: %d MB", ESP.getPsramSize() / (1024 * 1024));
    ESP_LOGI(TAG, "Free Heap: %d bytes", ESP.getFreeHeap());
    ESP_LOGI(TAG, "Boot Time: %d ms", millis() - g_boot_start_time);
    
    if (g_hardware_manager) {
        ESP_LOGI(TAG, "Hardware Status:");
        ESP_LOGI(TAG, "  - Display: %s", g_hardware_manager->isDisplayAvailable() ? "OK" : "FAIL");
        ESP_LOGI(TAG, "  - WiFi: %s", g_hardware_manager->isWiFiAvailable() ? "OK" : "FAIL");
        ESP_LOGI(TAG, "  - 4G Modem: %s", g_hardware_manager->isModemAvailable() ? "OK" : "FAIL");
        ESP_LOGI(TAG, "  - LoRa: %s", g_hardware_manager->isLoRaAvailable() ? "OK" : "FAIL");
        ESP_LOGI(TAG, "  - GPS: %s", g_hardware_manager->isGPSAvailable() ? "OK" : "FAIL");
        ESP_LOGI(TAG, "  - SD Card: %s", g_hardware_manager->isSDCardAvailable() ? "OK" : "FAIL");
        ESP_LOGI(TAG, "  - Battery: %.2fV", g_hardware_manager->getBatteryVoltage());
    }
    ESP_LOGI(TAG, "================================");
}

/**
 * @brief Handle system events and messages
 */
void handle_system_events() {
    // Feed watchdog
    Watchdog::feed();
    
    // Process hardware events
    if (g_hardware_manager) {
        g_hardware_manager->processEvents();
    }
    
    // Process display events
    if (g_display_manager) {
        g_display_manager->processEvents();
    }
    
    // Process communication events
    if (g_wifi_manager) {
        g_wifi_manager->processEvents();
    }
    
    // Process application events
    if (g_launcher) {
        g_launcher->processEvents();
    }
    
    // Process plugin events
    if (g_plugin_manager) {
        g_plugin_manager->processEvents();
    }
}

/**
 * @brief Handle power management tasks
 */
void handle_power_management() {
    static uint32_t last_power_check = 0;
    uint32_t now = millis();
    
    // Check power status every 30 seconds
    if (now - last_power_check > 30000) {
        if (g_hardware_manager) {
            float battery_voltage = g_hardware_manager->getBatteryVoltage();
            
            // Update display battery indicator
            if (g_display_manager) {
                g_display_manager->updateBatteryStatus(battery_voltage);
            }
            
            // Handle low battery
            if (battery_voltage < BATTERY_LOW_VOLTAGE) {
                ESP_LOGW(TAG, "Low battery: %.2fV", battery_voltage);
                if (g_display_manager) {
                    g_display_manager->showLowBatteryWarning();
                }
                
                // Enter power saving mode if critical
                if (battery_voltage < BATTERY_CRITICAL_VOLTAGE) {
                    ESP_LOGE(TAG, "Critical battery: %.2fV - entering deep sleep", battery_voltage);
                    SystemState::enterDeepSleep();
                }
            }
        }
        last_power_check = now;
    }
}

/**
 * @brief Main system loop
 */
void system_main_loop() {
    ESP_LOGI(TAG, "Entering main system loop");
    g_system_running = true;
    
    // Update display to show system ready
    if (g_display_manager) {
        g_display_manager->showSystemReady();
        g_display_manager->refresh();
    }
    
    // Show launcher
    if (g_launcher) {
        g_launcher->show();
    }
    
    while (g_system_running) {
        // Handle system events
        handle_system_events();
        
        // Handle power management
        handle_power_management();
        
        // Small delay to prevent overwhelming the system
        delay(10);
    }
}

/**
 * @brief Arduino setup function - OS initialization
 */
void setup() {
    g_boot_start_time = millis();
    
    // Initialize serial console
    Serial.begin(SERIAL_BAUD_RATE);
    
    // Wait for serial connection in debug builds
    #ifdef DEBUG_MODE
    while (!Serial && (millis() - g_boot_start_time) < 5000) {
        delay(100);
    }
    #endif
    
    ESP_LOGI(TAG, "=== T-DECK-PRO OS BOOT SEQUENCE ===");
    ESP_LOGI(TAG, "Starting T-Deck-Pro OS v%s", OS_VERSION);
    
    // Initialize boot manager
    g_boot_manager = new BootManager();
    if (!g_boot_manager || !g_boot_manager->initialize()) {
        system_panic("Boot manager initialization failed");
        return;
    }
    
    // Phase 1: Core Systems
    ESP_LOGI(TAG, "Boot Phase 1: Core Systems");
    if (!initialize_core_systems()) {
        system_panic("Core system initialization failed");
        return;
    }
    
    // Phase 2: Hardware
    ESP_LOGI(TAG, "Boot Phase 2: Hardware");
    if (!initialize_hardware()) {
        system_panic("Hardware initialization failed");
        return;
    }
    
    // Phase 3: Display
    ESP_LOGI(TAG, "Boot Phase 3: Display");
    if (!initialize_display()) {
        system_panic("Display initialization failed");
        return;
    }
    
    // Phase 4: Communication
    ESP_LOGI(TAG, "Boot Phase 4: Communication");
    if (!initialize_communication()) {
        system_panic("Communication initialization failed");
        return;
    }
    
    // Phase 5: Applications
    ESP_LOGI(TAG, "Boot Phase 5: Applications");
    if (!initialize_applications()) {
        system_panic("Application initialization failed");
        return;
    }
    
    // Mark system as initialized
    g_system_initialized = true;
    
    // Print system information
    print_system_info();
    
    ESP_LOGI(TAG, "=== BOOT COMPLETE - SYSTEM READY ===");
}

/**
 * @brief Arduino loop function - Main OS loop
 */
void loop() {
    // Ensure system is properly initialized
    if (!g_system_initialized) {
        ESP_LOGE(TAG, "System not initialized, restarting...");
        delay(1000);
        ESP.restart();
        return;
    }
    
    // Run main system loop
    system_main_loop();
}