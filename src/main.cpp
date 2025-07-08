/**
 * @file main_updated.cpp
 * @brief T-Deck-Pro OS Main Application with Complete Integration
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

// Core OS Components
#include "core/hal/board_config.h"
#include "core/utils/logger.h"
#include "core/display/eink_manager.h"
#include "core/storage/storage_manager.h"
#include "core/ui/launcher.h"
#include "core/server/server_integration.h"

// LVGL Configuration
#include "lvgl.h"

// Application Framework
#include "core/apps/app_manager.h"
#include "apps/meshtastic_app.h"
#include "apps/file_manager_app.h"
#include "apps/settings_app.h"

// Communication Stack
#include "core/communication/communication_manager.h"

// ===== GLOBAL VARIABLES =====
static TaskHandle_t main_task_handle = NULL;
static TaskHandle_t ui_task_handle = NULL;
static TaskHandle_t comm_task_handle = NULL;
static TaskHandle_t server_task_handle = NULL;

static bool systemInitialized = false;
static bool launcherActive = false;

// ===== FUNCTION DECLARATIONS =====
void setup_hardware(void);
void setup_storage(void);
void setup_communication(void);
void setup_server_integration(void);
void setup_applications(void);
void setup_launcher(void);
void main_task(void* parameter);
void ui_task(void* parameter);
void comm_task(void* parameter);
void server_task(void* parameter);

// Server event handlers
void onServerConfigUpdate(const JsonObject& config);
void onServerOTAUpdate(const OTAUpdate& ota);
void onServerAppCommand(const AppCommand& command);

// ===== LVGL TICK CALLBACK =====
static void lv_tick_task(void* arg) {
    (void) arg;
    lv_tick_inc(portTICK_PERIOD_MS);
}

/**
 * @brief Arduino setup function - Complete system initialization
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial && millis() < 5000) {
        delay(10);
    }
    
    Serial.println("\n=== T-Deck-Pro OS v2.0 Starting ===");
    Serial.printf("Build: %s %s\n", __DATE__, __TIME__);
    Serial.printf("ESP32-S3 Chip: %s\n", ESP.getChipModel());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("PSRAM Size: %d MB\n", ESP.getPsramSize() / (1024 * 1024));
    
    // Initialize logging system
    log_config_t log_config = {
        .level = LOG_LEVEL_INFO,
        .destinations = LOG_DEST_SERIAL | LOG_DEST_FILE,
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
    
    LOG_INFO("T-Deck-Pro OS v2.0 initialization started");
    
    // Phase 1: Hardware initialization
    setup_hardware();
    
    // Phase 2: Storage system initialization
    setup_storage();
    
    // Phase 3: Initialize LVGL and display
    lv_init();
    
    if (!eink_manager.initialize()) {
        LOG_ERROR("Failed to initialize E-ink display");
        while (1) delay(1000);
    }
    
    // Setup LVGL tick
    esp_timer_handle_t lvgl_tick_timer;
    esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lv_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, portTICK_PERIOD_MS * 1000);
    
    // Phase 4: Communication systems
    setup_communication();
    
    // Phase 5: Server integration
    setup_server_integration();
    
    // Phase 6: Application framework
    setup_applications();
    
    // Phase 7: Launcher UI
    setup_launcher();
    
    // Create system tasks
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
        3,  // Higher priority for UI responsiveness
        &ui_task_handle,
        1
    );
    
    xTaskCreatePinnedToCore(
        comm_task,
        "comm_task",
        8192,
        NULL,
        2,
        &comm_task_handle,
        0
    );
    
    xTaskCreatePinnedToCore(
        server_task,
        "server_task",
        8192,
        NULL,
        2,
        &server_task_handle,
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
    
    systemInitialized = true;
    LOG_INFO("T-Deck-Pro OS v2.0 initialization completed");
    Serial.println("=== System Ready - Launcher Active ===\n");
}

/**
 * @brief Arduino loop function - System monitoring
 */
void loop() {
    static uint32_t last_heartbeat = 0;
    uint32_t current_time = millis();
    
    if (current_time - last_heartbeat >= 30000) { // 30 second heartbeat
        size_t free_heap = ESP.getFreeHeap();
        StorageStats storage = STORAGE_MGR.getStorageStats();
        
        LOG_DEBUG("System heartbeat - Free heap: %d KB, Flash: %.1f%%, SD: %.1f%%", 
                 free_heap / 1024,
                 STORAGE_MGR.getFlashUsagePercent(),
                 STORAGE_MGR.getSDUsagePercent());
        
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
    
    if (!board_init()) {
        LOG_ERROR("Failed to initialize board hardware");
        while (1) delay(1000);
    }
    
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
 * @brief Initialize storage systems (Flash + SD Card)
 */
void setup_storage() {
    LOG_INFO("Initializing storage systems");
    
    if (!STORAGE_MGR.initialize()) {
        LOG_ERROR("Failed to initialize storage manager");
        while (1) delay(1000);
    }
    
    // Create essential directories
    STORAGE_MGR.createDirectory(STORAGE_PATH_APPS, STORAGE_AUTO);
    STORAGE_MGR.createDirectory(STORAGE_PATH_DATA, STORAGE_AUTO);
    STORAGE_MGR.createDirectory(STORAGE_PATH_CONFIG, STORAGE_FLASH);
    STORAGE_MGR.createDirectory(STORAGE_PATH_LOGS, STORAGE_FLASH);
    STORAGE_MGR.createDirectory(STORAGE_PATH_TEMP, STORAGE_AUTO);
    STORAGE_MGR.createDirectory(STORAGE_PATH_CACHE, STORAGE_AUTO);
    
    // Set storage priorities
    STORAGE_MGR.setStoragePriority(STORAGE_PRIORITY_BALANCED);
    STORAGE_MGR.setFlashThresholds(0.8, 0.95); // 80% warning, 95% critical
    
    // Optimize storage on startup
    STORAGE_MGR.optimizeStorage();
    STORAGE_MGR.cleanupTempFiles();
    
    StorageStats stats = STORAGE_MGR.getStorageStats();
    LOG_INFO("Storage initialized - Flash: %d/%d KB (%.1f%%), SD: %d/%d KB (%.1f%%)",
             stats.flashUsed / 1024, stats.flashTotal / 1024, STORAGE_MGR.getFlashUsagePercent(),
             stats.sdUsed / 1024, stats.sdTotal / 1024, STORAGE_MGR.getSDUsagePercent());
}

/**
 * @brief Initialize communication systems
 */
void setup_communication() {
    LOG_INFO("Initializing communication systems");
    
    CommunicationManager* commMgr = CommunicationManager::getInstance();
    if (!commMgr->initialize()) {
        LOG_ERROR("Failed to initialize communication manager");
        return;
    }
    
    commMgr->setPreferredInterface(COMM_INTERFACE_WIFI);
    commMgr->setAutoFailover(true);
    
    LOG_INFO("Communication systems initialized successfully");
}

/**
 * @brief Initialize server integration
 */
void setup_server_integration() {
    LOG_INFO("Initializing server integration");
    
    // Configure server connection
    ServerConfig serverConfig;
    serverConfig.brokerHost = "your-tailscale-server";  // TODO: Load from config
    serverConfig.brokerPort = 1883;
    serverConfig.deviceId = WiFi.macAddress();
    serverConfig.deviceType = "t-deck-pro";
    serverConfig.autoReconnect = true;
    serverConfig.reconnectInterval = 30;
    serverConfig.telemetryInterval = 300;  // 5 minutes
    serverConfig.heartbeatInterval = 60;   // 1 minute
    serverConfig.enableOTA = true;
    serverConfig.enableAppManagement = true;
    serverConfig.enableMeshForwarding = true;
    
    // Set event handlers
    SERVER_MGR.setConfigUpdateHandler(onServerConfigUpdate);
    SERVER_MGR.setOTAUpdateHandler(onServerOTAUpdate);
    SERVER_MGR.setAppCommandHandler(onServerAppCommand);
    
    if (!SERVER_MGR.initialize(serverConfig)) {
        LOG_WARN("Server integration initialization failed - will retry later");
    } else {
        LOG_INFO("Server integration initialized successfully");
    }
}

/**
 * @brief Initialize applications
 */
void setup_applications() {
    LOG_INFO("Initializing applications");
    
    AppManager& appManager = AppManager::getInstance();
    appManager.initialize();
    
    // Register core applications (built-in)
    REGISTER_APP(MeshtasticApp, "meshtastic", true);
    REGISTER_APP(FileManagerApp, "file_manager", false);
    REGISTER_APP(SettingsApp, "settings", false);
    
    // Load applications from storage
    std::vector<String> installedApps = STORAGE_MGR.getInstalledApps();
    for (const String& appId : installedApps) {
        // TODO: Load and register dynamic apps from storage
        LOG_INFO("Found installed app: %s", appId.c_str());
    }
    
    // Auto-start applications
    appManager.autoStartApps();
    
    LOG_INFO("Applications initialized - %d apps registered",
             appManager.getRegisteredApps().size());
}

/**
 * @brief Initialize launcher UI
 */
void setup_launcher() {
    LOG_INFO("Initializing launcher UI");
    
    if (!LAUNCHER.initialize()) {
        LOG_ERROR("Failed to initialize launcher");
        while (1) delay(1000);
    }
    
    // Show home screen
    LAUNCHER.showHome();
    launcherActive = true;
    
    LOG_INFO("Launcher initialized and active");
}

/**
 * @brief Main system task - System monitoring and management
 */
void main_task(void* parameter) {
    LOG_INFO("Main task started");
    
    const TickType_t xDelay = pdMS_TO_TICKS(1000);
    AppManager& appManager = AppManager::getInstance();
    
    while (1) {
        if (!systemInitialized) {
            vTaskDelay(xDelay);
            continue;
        }
        
        // Update application manager
        appManager.update();
        
        // Update storage manager
        static uint32_t lastStorageCheck = 0;
        if (millis() - lastStorageCheck > 60000) { // Every minute
            if (STORAGE_MGR.isFlashCritical()) {
                LOG_WARN("Flash storage critical - triggering optimization");
                STORAGE_MGR.optimizeStorage();
                appManager.handleMemoryWarning();
            }
            lastStorageCheck = millis();
        }
        
        // System health monitoring
        size_t free_heap = ESP.getFreeHeap();
        if (free_heap < 50000) { // Less than 50KB free
            LOG_WARN("Low memory warning: %d bytes free", free_heap);
            appManager.handleMemoryWarning();
        }
        
        // Power management
        uint16_t battery_mv = board_get_battery_voltage();
        if (battery_mv < BOARD_BAT_LOW_MV) {
            LOG_WARN("Low battery: %d mV", battery_mv);
            // TODO: Trigger power saving mode
        }
        
        // Periodic maintenance
        static uint32_t task_counter = 0;
        task_counter++;
        
        if (task_counter % 60 == 0) { // Every minute
            log_flush();
            
            AppManager::SystemStats stats = appManager.getSystemStats();
            LOG_DEBUG("System Stats - Apps: %d/%d, Memory: %d KB, Uptime: %d min",
                     stats.runningApps, stats.totalApps,
                     stats.totalMemoryUsed / 1024,
                     stats.uptime / 60000);
        }
        
        if (task_counter % 300 == 0) { // Every 5 minutes
            appManager.saveSystemConfig();
            STORAGE_MGR.cleanupTempFiles();
        }
        
        vTaskDelay(xDelay);
    }
}

/**
 * @brief UI task - handles LVGL, launcher, and display updates
 */
void ui_task(void* parameter) {
    LOG_INFO("UI task started");
    
    const TickType_t xDelay = pdMS_TO_TICKS(10); // 10ms for smooth UI
    
    while (1) {
        if (!systemInitialized) {
            vTaskDelay(xDelay);
            continue;
        }
        
        // Handle LVGL tasks
        lv_timer_handler();
        
        // Update launcher
        if (launcherActive) {
            LAUNCHER.update();
        }
        
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
    
    const TickType_t xDelay = pdMS_TO_TICKS(1000);
    CommunicationManager* commMgr = CommunicationManager::getInstance();
    
    uint8_t rxBuffer[256];
    size_t receivedLength;
    comm_interface_t sourceInterface;
    
    while (1) {
        if (!systemInitialized) {
            vTaskDelay(xDelay);
            continue;
        }
        
        // Check for incoming messages
        if (commMgr->receiveMessage(rxBuffer, sizeof(rxBuffer), &receivedLength, &sourceInterface)) {
            LOG_DEBUG("Received message (%d bytes) from interface %d", receivedLength, sourceInterface);
            
            // Forward mesh messages to server if connected
            if (SERVER_MGR.isConnected()) {
                // TODO: Parse and forward mesh messages
            }
        }
        
        // Update communication statistics for launcher
        static uint32_t lastStatsUpdate = 0;
        if (millis() - lastStatsUpdate > 5000) { // Every 5 seconds
            if (launcherActive) {
                StatusInfo status;
                status.wifiConnected = WiFi.isConnected();
                status.wifiSignal = WiFi.RSSI();
                status.cellularConnected = false; // TODO: Get from cellular manager
                status.loraActive = true; // TODO: Get from LoRa manager
                status.batteryPercent = map(board_get_battery_voltage(), BOARD_BAT_CRIT_MV, BOARD_BAT_FULL_MV, 0, 100);
                status.usbConnected = board_is_usb_connected();
                status.serverConnected = SERVER_MGR.isConnected();
                status.runningApps = AppManager::getInstance().getRunningApps().size();
                
                LAUNCHER.updateStatus(status);
            }
            lastStatsUpdate = millis();
        }
        
        vTaskDelay(xDelay);
    }
}

/**
 * @brief Server task - handles server communication and integration
 */
void server_task(void* parameter) {
    LOG_INFO("Server task started");
    
    const TickType_t xDelay = pdMS_TO_TICKS(1000);
    
    while (1) {
        if (!systemInitialized) {
            vTaskDelay(xDelay);
            continue;
        }
        
        // Update server integration
        SERVER_MGR.update();
        
        // Auto-reconnect if needed
        if (!SERVER_MGR.isConnected() && WiFi.isConnected()) {
            static uint32_t lastReconnectAttempt = 0;
            if (millis() - lastReconnectAttempt > 30000) { // Try every 30 seconds
                LOG_INFO("Attempting to reconnect to server");
                SERVER_MGR.reconnect();
                lastReconnectAttempt = millis();
            }
        }
        
        vTaskDelay(xDelay);
    }
}

// ===== SERVER EVENT HANDLERS =====

void onServerConfigUpdate(const JsonObject& config) {
    LOG_INFO("Received configuration update from server");
    
    // Apply configuration changes
    if (config.containsKey("telemetry_interval")) {
        SERVER_MGR.setTelemetryInterval(config["telemetry_interval"]);
    }
    
    if (config.containsKey("display_settings")) {
        // TODO: Apply display settings
    }
    
    // Show notification in launcher
    if (launcherActive) {
        LAUNCHER.showNotification("Configuration Updated", "Settings applied from server");
    }
}

void onServerOTAUpdate(const OTAUpdate& ota) {
    LOG_INFO("OTA update available: %s v%s", ota.type.c_str(), ota.version.c_str());
    
    if (launcherActive) {
        String message = "Update available: " + ota.version;
        LAUNCHER.showNotification("OTA Update", message, 10000);
    }
    
    // TODO: Implement OTA update process
}

void onServerAppCommand(const AppCommand& command) {
    LOG_INFO("App management command: %s for %s", command.action.c_str(), command.appId.c_str());
    
    AppManager& appManager = AppManager::getInstance();
    
    if (command.action == "install") {
        // TODO: Download and install app
        LOG_INFO("Installing app: %s", command.appId.c_str());
    } else if (command.action == "remove") {
        appManager.unregisterApp(command.appId);
        STORAGE_MGR.uninstallApp(command.appId);
    } else if (command.action == "update") {
        // TODO: Update existing app
    }
    
    // Refresh launcher app list
    if (launcherActive) {
        LAUNCHER.refreshApps();
    }
}