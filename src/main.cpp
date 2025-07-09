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

static const char* TAG = "Main";
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
    Logger::setLogLevel(Logger::LOG_DEBUG);
    Logger::info(TAG, "Logging system initialized");
    
    Logger::info(TAG, "T-Deck-Pro OS v2.0 initialization started");
    
    // Phase 1: Hardware initialization
    setup_hardware();
    
    // Phase 2: Storage system initialization
    setup_storage();
    
    // Phase 3: Initialize LVGL and display
    lv_init();
    
    if (!eink_manager.initialize()) {
        Logger::error("Main", "Failed to initialize E-ink display");
        while (1) delay(1000);
    }
    
    
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
    Logger::info("Main", "T-Deck-Pro OS v2.0 initialization completed");
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
        
        Logger::debug(TAG, "System heartbeat - Free heap: %d KB, Flash: %.1f%%, SD: %.1f%%",
                      free_heap / 1024, STORAGE_MGR.getFlashUsagePercent(), STORAGE_MGR.getSDUsagePercent());
        
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
    Logger::info(TAG, "Initializing hardware components");
    
    // NOTE: board_init() is called by the display manager
    
    // Check battery status
    uint16_t battery_mv = board_get_battery_voltage();
    bool usb_connected = board_is_usb_connected();
    
    Logger::info(TAG, String("Battery: ") + String(battery_mv) + " mV, USB: " + (usb_connected ? "Connected" : "Disconnected"));
    
    if (battery_mv < 3300 && !usb_connected) {
        Logger::error(TAG, "Critical battery level, entering deep sleep");
        esp_deep_sleep_start();
    }
    
    Logger::info(TAG, "Hardware initialization completed");
}

/**
 * @brief Initialize storage systems (Flash + SD Card)
 */
void setup_storage() {
    Logger::info(TAG, "Initializing storage systems");
    
    if (!STORAGE_MGR.initialize()) {
        Logger::error(TAG, "Failed to initialize storage manager");
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
    STORAGE_MGR.setFlashThresholds(0.8f, 0.95f); // 80% warning, 95% critical
    
    // Optimize storage on startup
    STORAGE_MGR.optimizeStorage();
    STORAGE_MGR.cleanupTempFiles();
    
    StorageStats stats = STORAGE_MGR.getStorageStats();
    Logger::info(TAG, "Storage initialized - Flash: %d/%d KB, SD: %d/%d KB",
                 stats.flashUsed / 1024, stats.flashTotal / 1024,
                 stats.sdUsed / 1024, stats.sdTotal / 1024);
}

/**
 * @brief Initialize communication systems
 */
void setup_communication() {
    Logger::info(TAG, "Initializing communication systems");
    
    auto* commMgr = CommunicationManager::getInstance();
    if (!commMgr->initialize()) {
        Logger::error(TAG, "Failed to initialize communication manager");
        return;
    }
    
    commMgr->setPreferredInterface(CommInterface::WIFI);
    commMgr->setAutoFailover(true);
    
    Logger::info(TAG, "Communication systems initialized successfully");
}

/**
 * @brief Initialize server integration
 */
void setup_server_integration() {
    Logger::info(TAG, "Initializing server integration");
    
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
    SERVER_MGR->setConfigUpdateHandler(onServerConfigUpdate);
    SERVER_MGR->setOTAUpdateHandler(onServerOTAUpdate);
    SERVER_MGR->setAppCommandHandler(onServerAppCommand);
    
    if (!SERVER_MGR->initialize(serverConfig)) {
        Logger::warning(TAG, "Server integration initialization failed - will retry later");
    } else {
        Logger::info(TAG, "Server integration initialized successfully");
    }
}

/**
 * @brief Initialize applications
 */
void setup_applications() {
    Logger::info(TAG, "Initializing applications");
    
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
        Logger::info(TAG, "Found installed app: " + appId);
    }
    
    // Auto-start applications
    appManager.autoStartApps();
    
    Logger::info(TAG, "Applications initialized - %d apps registered", appManager.getRegisteredApps().size());
}

/**
 * @brief Initialize launcher UI
 */
void setup_launcher() {
    Logger::info(TAG, "Initializing launcher UI");
    
    auto* launcher = Launcher::getInstance();
    if (!launcher->init()) {
        Logger::error(TAG, "Failed to initialize launcher");
        while (1) delay(1000);
    }
    
    launcherActive = true;
    
    Logger::info(TAG, "Launcher initialized and active");
}

/**
 * @brief Main system task - System monitoring and management
 */
void main_task(void* parameter) {
    Logger::info(TAG, "Main task started");
    
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
                Logger::warning(TAG, "Flash storage critical - triggering optimization");
                STORAGE_MGR.optimizeStorage();
                appManager.handleMemoryWarning();
            }
            lastStorageCheck = millis();
        }
        
        // System health monitoring
        size_t free_heap = ESP.getFreeHeap();
        if (free_heap < 50000) { // Less than 50KB free
            Logger::warning(TAG, "Low memory warning: %d bytes free", free_heap);
            appManager.handleMemoryWarning();
        }
        
        // Power management
        uint16_t battery_mv = board_get_battery_voltage();
        if (battery_mv < 3400) {
            Logger::warning(TAG, "Low battery: %d mV", battery_mv);
            // TODO: Trigger power saving mode
        }
        
        // Periodic maintenance
        static uint32_t task_counter = 0;
        task_counter++;
        
        if (task_counter % 60 == 0) { // Every minute
            // Flush logs periodically
            
            AppManager::SystemStats stats = appManager.getSystemStats();
            Logger::debug(TAG, "System Stats - Apps: %d/%d, Memory: %d KB, Uptime: %d min",
                          stats.runningApps, stats.totalApps, stats.totalMemoryUsed / 1024, stats.uptime / 60000);
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
    Logger::info(TAG, "UI task started");
    
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
            Launcher::getInstance()->update();
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
    Logger::info(TAG, "Communication task started");
    
    const TickType_t xDelay = pdMS_TO_TICKS(1000);
    auto* commMgr = CommunicationManager::getInstance();
    
    uint8_t rxBuffer[256];
    size_t receivedLength;
    CommInterface sourceInterface;
    
    while (1) {
        if (!systemInitialized) {
            vTaskDelay(xDelay);
            continue;
        }
        
        // TODO: Implement message receiving
        
        // Update communication statistics for launcher
        static uint32_t lastStatsUpdate = 0;
        if (millis() - lastStatsUpdate > 5000) { // Every 5 seconds
            if (launcherActive) {
                // TODO: Implement status updates in launcher
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
    Logger::info(TAG, "Server task started");
    
    const TickType_t xDelay = pdMS_TO_TICKS(1000);
    
    while (1) {
        if (!systemInitialized) {
            vTaskDelay(xDelay);
            continue;
        }
        
        // Update server integration
        SERVER_MGR->update();
        
        // Auto-reconnect if needed
        if (!SERVER_MGR->isConnected() && WiFi.isConnected()) {
            static uint32_t lastReconnectAttempt = 0;
            if (millis() - lastReconnectAttempt > 30000) { // Try every 30 seconds
                Logger::info(TAG, "Attempting to reconnect to server");
                SERVER_MGR->reconnect();
                lastReconnectAttempt = millis();
            }
        }
        
        vTaskDelay(xDelay);
    }
}

// ===== SERVER EVENT HANDLERS =====

void onServerConfigUpdate(const JsonObject& config) {
    Logger::info(TAG, "Received configuration update from server");
    
    // Apply configuration changes
    if (config.containsKey("telemetry_interval")) {
        SERVER_MGR->setTelemetryInterval(config["telemetry_interval"]);
    }
    
    // TODO: Apply other config settings
    
    // Show notification in launcher
    if (launcherActive) {
        Launcher::getInstance()->addNotification("Config Updated", "Settings applied from server");
    }
}

void onServerOTAUpdate(const OTAUpdate& ota) {
    Logger::info(TAG, "OTA update available: %s v%s", ota.type.c_str(), ota.version.c_str());
    
    if (launcherActive) {
        String message = "Update available: " + ota.version;
        Launcher::getInstance()->addNotification("OTA Update", message);
    }
    
    // TODO: Implement OTA update process
}

void onServerAppCommand(const AppCommand& command) {
    Logger::info(TAG, "App command: %s for %s", command.action.c_str(), command.appId.c_str());
    
    AppManager& appManager = AppManager::getInstance();
    
    if (command.action == "install") {
        Logger::info(TAG, "Installing app: " + command.appId);
        // TODO: Download and install app
    } else if (command.action == "remove") {
        appManager.unregisterApp(command.appId);
        STORAGE_MGR.uninstallApp(command.appId);
    } else if (command.action == "update") {
        // TODO: Update existing app
    }
    
    // Refresh launcher app list
    if (launcherActive) {
        // launcher->refreshApps(); // TODO: Implement public method
    }
}