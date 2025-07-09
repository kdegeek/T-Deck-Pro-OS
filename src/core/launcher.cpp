/**
 * @file launcher.cpp
 * @brief T-Deck-Pro Launcher Implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "launcher.h"
#include "utils/logger.h"

// Include UI launcher header directly
#include "ui/launcher.h"

// Helper function to add notifications to UI launcher
void addUINotification(UILauncher* launcher, const String& title, const String& message) {
    if (launcher) {
        launcher->addNotification(title, message);
    }
}

// Macro to get UI launcher instance
#define UI_LAUNCHER() static_cast<UILauncher*>(UILauncher::getInstance())
#include "../drivers/hardware_manager.h"
#include "apps/app_manager.h"
#include "utils/logger.h"
#include <ArduinoJson.h>

static const char* TAG = "Launcher";

Launcher::Launcher() : 
    initialized(false),
    visible(false),
    main_container(nullptr),
    status_bar(nullptr),
    app_grid(nullptr),
    bottom_nav(nullptr),
    time_label(nullptr),
    battery_label(nullptr),
    wifi_label(nullptr),
    cellular_label(nullptr),
    mqtt_label(nullptr),
    tailscale_label(nullptr),
    settings_btn(nullptr),
    info_btn(nullptr),
    refresh_btn(nullptr),
    ui_launcher(nullptr) {
    
    // Get reference to UI launcher singleton
    // Get UI Launcher instance (from ui/launcher.h)
    ui_launcher = UILauncher::getInstance();
    
    // Initialize app icons vector
    app_icons.clear();
}

Launcher::~Launcher() {
    // Cleanup handled by UI launcher
}

bool Launcher::initialize() {
    if (initialized) return true;
    
    Logger::info(TAG, "Initializing launcher...");
    
    // Get UI launcher instance
    ui_launcher = UILauncher::getInstance();
    if (!ui_launcher) {
        Logger::error(TAG, "Failed to get UI launcher instance");
        return false;
    }
    
    // Initialize UI launcher
    if (!UI_LAUNCHER()->init()) {
        Logger::error(TAG, "Failed to initialize UI launcher");
        return false;
    }
    
    initialized = true;
    visible = false;
    
    // Load built-in apps
    load_builtin_apps();
    
    Logger::info(TAG, "Launcher initialized successfully");
    return true;
}

void Launcher::show() {
    if (!initialized) {
        Logger::error(TAG, "Launcher not initialized");
        return;
    }
    
    Logger::info(TAG, "Showing launcher");
    visible = true;
    
    // UI launcher is always visible, just mark as visible
}

void Launcher::hide() {
    if (!initialized) return;
    
    Logger::info(TAG, "Hiding launcher");
    visible = false;
    
    // UI launcher handles its own visibility
}

void Launcher::update() {
    if (!initialized || !ui_launcher) return;
    
    // Update UI launcher
    UI_LAUNCHER()->update();
    
    // Additional update logic if needed
    static uint32_t last_update = 0;
    uint32_t now = millis();
    
    if (now - last_update > 5000) { // Update every 5 seconds
        last_update = now;
        Logger::debug(TAG, "Launcher update tick");
    }
}

void Launcher::show_boot_complete(uint32_t boot_duration) {
    Logger::info(TAG, "Boot completed in " + String(boot_duration) + "ms");
    
    // Show boot complete notification
     if (ui_launcher) {
         addUINotification(ui_launcher, "System Ready", "Boot completed in " + String(boot_duration) + "ms");
     }
    
    Serial.println("[LAUNCHER] Boot Complete!");
    Serial.printf("[LAUNCHER] Boot time: %u ms\n", boot_duration);
}

bool Launcher::add_app_icon(const AppIcon& app_info) {
    Logger::info(TAG, "Adding app icon: " + app_info.name);
    
    // Check if app already exists
    for (const auto& icon : app_icons) {
        if (icon.name == app_info.name) {
            Logger::warning(TAG, "App icon already exists: " + app_info.name);
            return false;
        }
    }
    
    app_icons.push_back(app_info);
    
    // UI launcher will refresh automatically
    
    return true;
}

bool Launcher::remove_app_icon(const String& app_name) {
    Logger::info(TAG, "Removing app icon: " + app_name);
    
    for (auto it = app_icons.begin(); it != app_icons.end(); ++it) {
        if (it->name == app_name) {
            app_icons.erase(it);
            
            // UI launcher will refresh automatically
            
            return true;
        }
    }
    
    Logger::warning(TAG, "App icon not found: " + app_name);
    return false;
}

bool Launcher::update_app_notification(const String& app_name, int count) {
    Logger::debug(TAG, "Updating notification for " + app_name + ": " + String(count));
    
    for (auto& icon : app_icons) {
        if (icon.name == app_name) {
            icon.has_notification = (count > 0);
            icon.notification_count = count;
            
            // Add notification to UI launcher
             if (ui_launcher && count > 0) {
                 addUINotification(ui_launcher, app_name, String(count) + " new notifications");
             }
            
            return true;
        }
    }
    
    Logger::warning(TAG, "App not found for notification update: " + app_name);
    return false;
}

void Launcher::update_system_status(const SystemStatus& status) {
    // UI launcher handles status updates automatically
    // Just log status updates for debugging
    static uint32_t last_status_log = 0;
    uint32_t now = millis();
    
    if (now - last_status_log > 10000) { // Log every 10 seconds
        last_status_log = now;
        Logger::debug(TAG, "System status - Battery: " + String(status.battery_percentage) + "%");
    }
}

void Launcher::show_settings() {
    Logger::info(TAG, "Showing settings");
    
    // Launch settings app
    launch_app("Settings");
}

void Launcher::show_system_info() {
    Logger::info(TAG, "Showing system info");
    
    // Show system info notification
    if (ui_launcher) {
        String info = "Free heap: " + String(ESP.getFreeHeap()) + " bytes\n";
         info += "Uptime: " + String(millis()) + " ms";
         addUINotification(ui_launcher, "System Info", info);
    }
    
    Serial.println("[SYSTEM INFO]");
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Uptime: %u ms\n", millis());
}

bool Launcher::launch_app(const String& app_name) {
    Logger::info(TAG, "Launching app: " + app_name);
    
    // TODO: Integrate with app manager when available
    // For now, just log the launch request
    
    return true;
}

bool Launcher::is_visible() const {
    return visible;
}

void Launcher::refresh_app_list() {
    Logger::info(TAG, "Refreshing app list");
    
    // Clear current apps
    app_icons.clear();
    
    // Reload apps
    load_builtin_apps();
    load_plugin_apps();
    
    // UI launcher will refresh automatically
}

void Launcher::load_builtin_apps() {
    Logger::info(TAG, "Loading built-in apps");
    
    // Add built-in apps
    AppIcon settings_app;
    settings_app.name = "Settings";
    settings_app.display_name = "Settings";
    settings_app.icon_path = "";
    settings_app.description = "System Settings";
    settings_app.is_plugin = false;
    settings_app.has_notification = false;
    settings_app.notification_count = 0;
    settings_app.icon_obj = nullptr;
    settings_app.label_obj = nullptr;
    settings_app.badge_obj = nullptr;
    app_icons.push_back(settings_app);
    
    AppIcon file_manager_app;
    file_manager_app.name = "FileManager";
    file_manager_app.display_name = "File Manager";
    file_manager_app.icon_path = "";
    file_manager_app.description = "File Management";
    file_manager_app.is_plugin = false;
    file_manager_app.has_notification = false;
    file_manager_app.notification_count = 0;
    file_manager_app.icon_obj = nullptr;
    file_manager_app.label_obj = nullptr;
    file_manager_app.badge_obj = nullptr;
    app_icons.push_back(file_manager_app);
    
    AppIcon meshtastic_app;
    meshtastic_app.name = "Meshtastic";
    meshtastic_app.display_name = "Meshtastic";
    meshtastic_app.icon_path = "";
    meshtastic_app.description = "Mesh Networking";
    meshtastic_app.is_plugin = false;
    meshtastic_app.has_notification = false;
    meshtastic_app.notification_count = 0;
    meshtastic_app.icon_obj = nullptr;
    meshtastic_app.label_obj = nullptr;
    meshtastic_app.badge_obj = nullptr;
    app_icons.push_back(meshtastic_app);
    
    Logger::info(TAG, "Loaded " + String(app_icons.size()) + " built-in apps");
}

void Launcher::load_plugin_apps() {
    Logger::info(TAG, "Loading plugin apps");
    
    // TODO: Load plugin apps from SD card when storage manager is available
    // For now, just log that we're trying to load plugins
    
    Logger::info(TAG, "Plugin loading not yet implemented");
}