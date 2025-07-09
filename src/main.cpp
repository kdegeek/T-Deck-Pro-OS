/**
 * @file main.cpp
 * @brief T-Deck-Pro Simplified OS Main Application
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <SD.h>
#include <Wire.h>
#include <SPI.h>

// Core OS Components
#include "config/os_config.h"
#include "drivers/hardware_manager.h"
#include "core/boot_manager.h"
#include "core/launcher.h"
#include "core/communication/mqtt_manager.h"
#include "core/communication/tailscale_manager.h"
#include "core/apps/plugin_manager.h"

// Global OS Components
HardwareManager hardware;
BootManager boot;
Launcher launcher;
MQTTManager mqtt;
TailscaleManager tailscale;
PluginManager plugins;

// System State
bool system_ready = false;
bool emergency_mode = false;
String last_error = "";
uint32_t last_heartbeat = 0;
uint32_t last_status_update = 0;
uint32_t boot_start_time = 0;

// Forward declarations
void handle_emergency_mode();
void send_system_heartbeat();
void update_system_status();
void handle_mqtt_app_launch(const String& app_name, const String& parameters);
void handle_system_shutdown();
void print_system_info();

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
    boot_start_time = millis();
    
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial && millis() < 5000) {
        delay(10);
    }
    
    Serial.println("\n" + String("=").substring(0, 50));
    Serial.println("T-Deck-Pro Simplified OS v2.0");
    Serial.println("Build: " + String(__DATE__) + " " + String(__TIME__));
    Serial.println(String("=").substring(0, 50) + "\n");
    
    // Phase 1: Boot Manager Initialization
    Serial.println("[BOOT] Phase 1: Boot Manager");
    boot.set_boot_stage(BootStage::POWER_ON);
    
    if (!boot.initialize()) {
        Serial.println("[ERROR] Boot manager initialization failed");
        emergency_mode = true;
        return;
    }
    
    boot.show_splash_screen();
    boot.set_boot_stage(BootStage::HARDWARE_INIT);
    
    // Phase 2: Hardware Initialization
    Serial.println("[BOOT] Phase 2: Hardware Initialization");
    boot.display_progress("Initializing Hardware...", 10);
    
    if (!hardware.initialize()) {
        Serial.println("[ERROR] Hardware initialization failed");
        boot.display_error("Hardware Init Failed", hardware.get_last_error());
        emergency_mode = true;
        return;
    }
    
    boot.set_boot_stage(BootStage::STORAGE_INIT);
    
    // Phase 3: Storage Initialization
    Serial.println("[BOOT] Phase 3: Storage Systems");
    boot.display_progress("Initializing Storage...", 25);
    
    // Initialize SPIFFS for system data
    if (!SPIFFS.begin(true)) {
        Serial.println("[WARNING] SPIFFS initialization failed");
        // Continue without SPIFFS - not critical
    } else {
        Serial.println("[BOOT] SPIFFS initialized");
    }
    
    // Initialize SD card for apps and data
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("[WARNING] SD card initialization failed");
        // Continue without SD - plugins won't work but core OS will
    } else {
        Serial.println("[BOOT] SD card initialized");
    }
    
    boot.set_boot_stage(BootStage::DISPLAY_INIT);
    
    // Phase 4: Display and Input
    Serial.println("[BOOT] Phase 4: Display and Input");
    boot.display_progress("Initializing Display...", 40);
    
    if (!hardware.is_display_ready()) {
        Serial.println("[ERROR] Display not ready");
        boot.display_error("Display Failed", "E-ink display not responding");
        emergency_mode = true;
        return;
    }
    
    if (!hardware.is_touch_ready()) {
        Serial.println("[WARNING] Touch not ready - continuing without touch");
    }
    
    if (!hardware.is_keyboard_ready()) {
        Serial.println("[WARNING] Keyboard not ready - continuing without keyboard");
    }
    
    boot.set_boot_stage(BootStage::CONNECTIVITY_INIT);
    
    // Phase 5: Connectivity
    Serial.println("[BOOT] Phase 5: Connectivity");
    boot.display_progress("Initializing Connectivity...", 55);
    
    // Initialize WiFi
    WiFi.mode(WIFI_STA);
    Serial.println("[BOOT] WiFi initialized in station mode");
    
    // Initialize other connectivity (4G, LoRa, Bluetooth)
    if (hardware.is_modem_ready()) {
        Serial.println("[BOOT] 4G modem ready");
    }
    
    if (hardware.is_lora_ready()) {
        Serial.println("[BOOT] LoRa ready");
    }
    
    boot.set_boot_stage(BootStage::SERVICES_INIT);
    
    // Phase 6: Core Services
    Serial.println("[BOOT] Phase 6: Core Services");
    boot.display_progress("Starting Services...", 70);
    
    // Initialize MQTT Manager
    if (!mqtt.initialize()) {
        Serial.println("[WARNING] MQTT initialization failed - continuing without MQTT");
    } else {
        Serial.println("[BOOT] MQTT manager initialized");
    }
    
    // Initialize Tailscale Manager
    if (!tailscale.initialize()) {
        Serial.println("[WARNING] Tailscale initialization failed - continuing without VPN");
    } else {
        Serial.println("[BOOT] Tailscale manager initialized");
    }
    
    // Initialize Plugin Manager
    if (!plugins.initialize()) {
        Serial.println("[WARNING] Plugin manager initialization failed - no SD card apps");
    } else {
        Serial.println("[BOOT] Plugin manager initialized");
        
        // Start autostart plugins
        int autostart_count = plugins.start_autostart_plugins();
        Serial.printf("[BOOT] Started %d autostart plugins\n", autostart_count);
    }
    
    boot.display_progress("Starting Launcher...", 85);
    
    // Phase 7: Launcher
    Serial.println("[BOOT] Phase 7: Launcher");
    if (!launcher.initialize()) {
        Serial.println("[ERROR] Launcher initialization failed");
        boot.display_error("Launcher Failed", "Cannot start main interface");
        emergency_mode = true;
        return;
    }
    
    boot.display_progress("System Ready", 100);
    boot.set_boot_stage(BootStage::COMPLETE);
    
    // Phase 8: System Ready
    Serial.println("[BOOT] Phase 8: System Ready");
    
    // Show boot completion on launcher
    launcher.display_boot_completion(millis() - boot_start_time);
    
    // Connect to saved WiFi networks
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[SYSTEM] Attempting WiFi connection...");
        // In real implementation, would load saved networks and connect
    }
    
    // Connect MQTT if WiFi is available
    if (WiFi.status() == WL_CONNECTED && !mqtt.is_connected()) {
        mqtt.connect();
    }
    
    // Start Tailscale if configured
    if (tailscale.is_configured() && !tailscale.is_connected()) {
        tailscale.connect();
    }
    
    system_ready = true;
    last_heartbeat = millis();
    last_status_update = millis();
    
    print_system_info();
    
    Serial.println("[SYSTEM] T-Deck-Pro OS ready!");
    Serial.printf("[SYSTEM] Boot time: %lu ms\n", millis() - boot_start_time);
    Serial.printf("[SYSTEM] Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("[SYSTEM] Free PSRAM: %d bytes\n", ESP.getFreePsram());
}

/**
 * @brief Arduino loop function - System monitoring
 */
void loop() {
    uint32_t loop_start = millis();
    
    // Handle emergency mode
    if (emergency_mode) {
        handle_emergency_mode();
        delay(1000);
        return;
    }
    
    // Update core components
    if (system_ready) {
        // Update hardware status
        hardware.update();
        
        // Update launcher UI
        launcher.update();
        
        // Update connectivity services
        if (WiFi.status() == WL_CONNECTED) {
            mqtt.update();
            tailscale.update();
        }
        
        // Update plugin manager
        plugins.update();
        
        // Handle MQTT app launch requests
        if (mqtt.is_connected() && mqtt.has_pending_app_launch()) {
            auto launch_request = mqtt.get_pending_app_launch();
            handle_mqtt_app_launch(launch_request.app_name, launch_request.parameters);
        }
        
        // Send system heartbeat
        if (loop_start - last_heartbeat > 30000) { // 30 second heartbeat
            send_system_heartbeat();
            last_heartbeat = loop_start;
        }
        
        // Update system status
        if (loop_start - last_status_update > 5000) { // 5 second status update
            update_system_status();
            last_status_update = loop_start;
        }
        
        // Check for system shutdown request
        if (hardware.is_power_button_long_pressed()) {
            handle_system_shutdown();
        }
    }
    
    // Feed watchdog
    esp_task_wdt_reset();
    
    // Maintain target loop frequency
     uint32_t loop_time = millis() - loop_start;
     if (loop_time < MAIN_LOOP_INTERVAL) {
         delay(MAIN_LOOP_INTERVAL - loop_time);
     }
 }

void handle_emergency_mode() {
    static uint32_t last_emergency_update = 0;
    uint32_t now = millis();
    
    if (now - last_emergency_update > 5000) {
        Serial.println("[EMERGENCY] System in emergency mode");
        Serial.println("[EMERGENCY] Last error: " + last_error);
        Serial.println("[EMERGENCY] Press reset to restart");
        
        // Try to show emergency message on display if possible
        if (hardware.is_display_ready()) {
            boot.display_error("EMERGENCY MODE", "System Error - Reset Required");
        }
        
        last_emergency_update = now;
    }
    
    // Blink LED if available
    static bool led_state = false;
    static uint32_t last_blink = 0;
    
    if (now - last_blink > 500) {
        led_state = !led_state;
        // Set LED state if available
        last_blink = now;
    }
}

void send_system_heartbeat() {
    if (!mqtt.is_connected()) {
        return;
    }
    
    DynamicJsonDocument heartbeat(512);
    heartbeat["timestamp"] = millis();
    heartbeat["uptime"] = millis() - boot_start_time;
    heartbeat["free_heap"] = ESP.getFreeHeap();
    heartbeat["free_psram"] = ESP.getFreePsram();
    heartbeat["wifi_rssi"] = WiFi.RSSI();
    heartbeat["running_plugins"] = plugins.get_running_plugins().size();
    heartbeat["system_ready"] = system_ready;
    heartbeat["emergency_mode"] = emergency_mode;
    
    String heartbeat_str;
    serializeJson(heartbeat, heartbeat_str);
    
    mqtt.publish_telemetry("heartbeat", heartbeat_str);
}

void update_system_status() {
    // Update launcher with current system status
    SystemStatus status;
    
    // Connectivity status
    status.wifi_connected = (WiFi.status() == WL_CONNECTED);
    status.wifi_rssi = WiFi.RSSI();
    status.mqtt_connected = mqtt.is_connected();
    status.tailscale_connected = tailscale.is_connected();
    status.modem_connected = hardware.is_modem_ready();
    status.lora_enabled = hardware.is_lora_ready();
    
    // System resources
    status.free_heap = ESP.getFreeHeap();
    status.free_psram = ESP.getFreePsram();
    status.uptime = millis() - boot_start_time;
    
    // Hardware status
    status.battery_level = hardware.get_battery_level();
    status.charging = hardware.is_charging();
    status.sd_card_present = hardware.is_sd_card_ready();
    
    // Plugin status
    status.running_plugins = plugins.get_running_plugins().size();
    status.total_plugins = plugins.get_available_plugins().size();
    
    launcher.update_system_status(status);
}

void handle_mqtt_app_launch(const String& app_name, const String& parameters) {
    Serial.printf("[SYSTEM] MQTT app launch request: %s\n", app_name.c_str());
    
    // Check if it's a system command
    if (app_name == "settings") {
        launcher.show_settings();
        mqtt.send_app_launch_result(app_name, true, "Settings opened");
        return;
    }
    
    if (app_name == "system_info") {
        launcher.show_system_info();
        mqtt.send_app_launch_result(app_name, true, "System info opened");
        return;
    }
    
    if (app_name == "launcher") {
        launcher.show();
        mqtt.send_app_launch_result(app_name, true, "Launcher shown");
        return;
    }
    
    // Try to launch plugin
    if (plugins.launch_plugin(app_name, parameters)) {
        mqtt.send_app_launch_result(app_name, true, "Plugin launched successfully");
        
        // Hide launcher when app starts
        launcher.hide();
    } else {
        String error_msg = "Plugin not found or failed to launch";
        mqtt.send_app_launch_result(app_name, false, error_msg);
        
        // Show notification on launcher
        NotificationMessage notification;
        notification.title = "App Launch Failed";
        notification.message = "Could not launch " + app_name;
        notification.type = "error";
        notification.timestamp = millis();
        
        launcher.show_notification(notification);
    }
}

void handle_system_shutdown() {
    Serial.println("[SYSTEM] Shutdown requested");
    
    // Show shutdown screen
    if (hardware.is_display_ready()) {
        boot.display_progress("Shutting down...", 0);
    }
    
    // Stop all plugins
    Serial.println("[SHUTDOWN] Stopping plugins...");
    plugins.stop_all_plugins(true);
    
    // Disconnect services
    Serial.println("[SHUTDOWN] Disconnecting services...");
    mqtt.disconnect();
    tailscale.disconnect();
    WiFi.disconnect();
    
    // Save system state
    Serial.println("[SHUTDOWN] Saving system state...");
    // In real implementation, would save current state
    
    // Hardware cleanup
    Serial.println("[SHUTDOWN] Hardware cleanup...");
    hardware.shutdown();
    
    // Final shutdown message
    Serial.println("[SHUTDOWN] System shutdown complete");
    
    // Enter deep sleep or restart
    esp_deep_sleep_start();
}

void print_system_info() {
    Serial.println("\n[INFO] System Information:");
    Serial.printf("[INFO] Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("[INFO] CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("[INFO] Flash Size: %d bytes\n", ESP.getFlashChipSize());
    Serial.printf("[INFO] Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("[INFO] Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.printf("[INFO] SDK Version: %s\n", ESP.getSdkVersion());
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[INFO] WiFi SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("[INFO] WiFi IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[INFO] WiFi RSSI: %d dBm\n", WiFi.RSSI());
    }
    
    Serial.printf("[INFO] Available Plugins: %d\n", plugins.get_available_plugins().size());
    Serial.printf("[INFO] Running Plugins: %d\n", plugins.get_running_plugins().size());
    Serial.println();
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
    
    auto* commMgr = TDeckOS::Communication::CommunicationManager::getInstance();
    if (!commMgr->initialize()) {
        Logger::error(TAG, "Failed to initialize communication manager");
        return;
    }
    
    commMgr->setPreferredInterface(TDeckOS::Communication::CommInterface::WIFI);
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
    auto* commMgr = TDeckOS::Communication::CommunicationManager::getInstance();
    
    uint8_t rxBuffer[256];
    size_t receivedLength;
    TDeckOS::Communication::CommInterface sourceInterface;
    
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