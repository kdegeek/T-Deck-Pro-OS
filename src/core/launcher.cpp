/**
 * @file launcher.cpp
 * @brief T-Deck-Pro Launcher - App Management and UI Navigation
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Manages app switching, UI navigation, and system state
 */

#include "launcher.h"
#include "utils/logger.h"
#include "storage/storage_manager.h"
#include "../../apps/file_manager_app.h"
#include "../../apps/meshtastic_app.h"
#include "../../apps/settings_app.h"
#include <ArduinoJson.h>

// ===== LAUNCHER IMPLEMENTATION =====

Launcher::Launcher() : initialized_(false), current_app_(nullptr), 
                       app_list_(), selected_index_(0), launcher_mode_(true) {
    
    // Initialize app list
    app_list_.reserve(20);
}

Launcher::~Launcher() {
    if (initialized_) {
        cleanup();
    }
}

bool Launcher::initialize() {
    if (initialized_) {
        return true;
    }
    
    logInfo("LAUNCHER", "Initializing launcher");
    
    // Initialize storage manager if not already done
    if (!getStorageManager()) {
        if (!initializeStorageManager()) {
            logError("LAUNCHER", "Failed to initialize storage manager");
            return false;
        }
    }
    
    // Initialize logger if not already done
    if (!getLogger()) {
        if (!initializeLogger()) {
            logError("LAUNCHER", "Failed to initialize logger");
            return false;
        }
    }
    
    // Register built-in apps
    registerBuiltInApps();
    
    // Load app list from storage
    loadAppList();
    
    // Initialize UI
    if (!initUI()) {
        logError("LAUNCHER", "Failed to initialize UI");
        return false;
    }
    
    initialized_ = true;
    
    logInfo("LAUNCHER", "Launcher initialized successfully");
    return true;
}

void Launcher::process() {
    if (!initialized_) {
        if (!initialize()) {
            logError("LAUNCHER", "Failed to initialize launcher");
            return;
        }
    }
    
    // Process current app if running
    if (current_app_ && !launcher_mode_) {
        current_app_->run();
    } else {
        // Process launcher UI
        processLauncherUI();
    }
    
    // Process system-wide input
    processSystemInput();
}

void Launcher::cleanup() {
    if (!initialized_) {
        return;
    }
    
    logInfo("LAUNCHER", "Cleaning up launcher");
    
    // Stop current app
    if (current_app_) {
        stopCurrentApp();
    }
    
    // Save app list
    saveAppList();
    
    initialized_ = false;
}

bool Launcher::initUI() {
    // Create launcher UI elements
    // This will be implemented when LVGL is available
    
    logInfo("LAUNCHER", "UI initialized");
    return true;
}

void Launcher::registerBuiltInApps() {
    // Register File Manager
    AppInfo file_manager;
    file_manager.name = "File Manager";
    file_manager.description = "Browse and manage files";
    file_manager.type = AppType::SYSTEM;
    file_manager.enabled = true;
    file_manager.memory_usage = 0;
    file_manager.last_used = 0;
    file_manager.app_ptr = nullptr; // Will be set when app is loaded
    
    app_list_.push_back(file_manager);
    
    // Register Meshtastic
    AppInfo meshtastic;
    meshtastic.name = "Meshtastic";
    meshtastic.description = "LoRa mesh networking";
    meshtastic.type = AppType::SYSTEM;
    meshtastic.enabled = true;
    meshtastic.memory_usage = 0;
    meshtastic.last_used = 0;
    meshtastic.app_ptr = nullptr;
    
    app_list_.push_back(meshtastic);
    
    // Register Settings
    AppInfo settings;
    settings.name = "Settings";
    settings.description = "System configuration";
    settings.type = AppType::SYSTEM;
    settings.enabled = true;
    settings.memory_usage = 0;
    settings.last_used = 0;
    settings.app_ptr = nullptr;
    
    app_list_.push_back(settings);
    
    logInfo("LAUNCHER", "Registered " + String(app_list_.size()) + " built-in apps");
}

void Launcher::loadAppList() {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return;
    }
    
    DynamicJsonDocument doc(2048);
    if (storage->loadJSON("/config/app_list.json", doc, StorageType::SPIFFS)) {
        // Load app list from JSON
        if (doc.containsKey("apps")) {
            JsonArray apps = doc["apps"];
            for (JsonObject app : apps) {
                AppInfo app_info;
                app_info.name = app["name"] | "";
                app_info.description = app["description"] | "";
                app_info.type = (AppType)(app["type"] | (int)AppType::USER);
                app_info.enabled = app["enabled"] | true;
                app_info.memory_usage = app["memory_usage"] | 0;
                app_info.last_used = app["last_used"] | 0;
                app_info.app_ptr = nullptr;
                
                app_list_.push_back(app_info);
            }
        }
        
        logInfo("LAUNCHER", "App list loaded: " + String(app_list_.size()) + " apps");
    } else {
        logInfo("LAUNCHER", "No app list found, using built-in apps only");
    }
}

void Launcher::saveAppList() {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return;
    }
    
    DynamicJsonDocument doc(2048);
    JsonArray apps = doc.createNestedArray("apps");
    
    for (const auto& app : app_list_) {
        JsonObject app_obj = apps.createNestedObject();
        app_obj["name"] = app.name;
        app_obj["description"] = app.description;
        app_obj["type"] = (int)app.type;
        app_obj["enabled"] = app.enabled;
        app_obj["memory_usage"] = app.memory_usage;
        app_obj["last_used"] = app.last_used;
    }
    
    if (storage->saveJSON("/config/app_list.json", doc, StorageType::SPIFFS)) {
        logInfo("LAUNCHER", "App list saved");
    } else {
        logError("LAUNCHER", "Failed to save app list");
    }
}

bool Launcher::launchApp(const String& app_name) {
    logInfo("LAUNCHER", "Launching app: " + app_name);
    
    // Find app in list
    for (auto& app : app_list_) {
        if (app.name.equalsIgnoreCase(app_name)) {
            return launchAppByIndex(&app - &app_list_[0]);
        }
    }
    
    logError("LAUNCHER", "App not found: " + app_name);
    return false;
}

bool Launcher::launchAppByIndex(uint8_t index) {
    if (index >= app_list_.size()) {
        logError("LAUNCHER", "Invalid app index: " + String(index));
        return false;
    }
    
    AppInfo& app_info = app_list_[index];
    
    if (!app_info.enabled) {
        logError("LAUNCHER", "App is disabled: " + app_info.name);
        return false;
    }
    
    // Stop current app if running
    if (current_app_) {
        stopCurrentApp();
    }
    
    // Load app if not already loaded
    if (!app_info.app_ptr) {
        if (!loadApp(app_info)) {
            logError("LAUNCHER", "Failed to load app: " + app_info.name);
            return false;
        }
    }
    
    // Set current app
    current_app_ = app_info.app_ptr;
    launcher_mode_ = false;
    selected_index_ = index;
    
    // Update app usage time
    app_info.last_used = millis();
    
    logInfo("LAUNCHER", "App launched: " + app_info.name);
    return true;
}

bool Launcher::loadApp(AppInfo& app_info) {
    // Load app based on name
    if (app_info.name.equalsIgnoreCase("File Manager")) {
        if (!getFileManagerApp()) {
            if (!initializeFileManagerApp()) {
                return false;
            }
        }
        app_info.app_ptr = getFileManagerApp();
        
    } else if (app_info.name.equalsIgnoreCase("Meshtastic")) {
        if (!getMeshtasticApp()) {
            if (!initializeMeshtasticApp()) {
                return false;
            }
        }
        app_info.app_ptr = getMeshtasticApp();
        
    } else if (app_info.name.equalsIgnoreCase("Settings")) {
        if (!getSettingsApp()) {
            if (!initializeSettingsApp()) {
                return false;
            }
        }
        app_info.app_ptr = getSettingsApp();
        
    } else {
        logError("LAUNCHER", "Unknown app: " + app_info.name);
        return false;
    }
    
    logInfo("LAUNCHER", "App loaded: " + app_info.name);
    return true;
}

void Launcher::stopCurrentApp() {
    if (!current_app_) {
        return;
    }
    
    logInfo("LAUNCHER", "Stopping current app");
    
    // App cleanup will be handled by the app itself
    current_app_ = nullptr;
    launcher_mode_ = true;
}

void Launcher::returnToLauncher() {
    logInfo("LAUNCHER", "Returning to launcher");
    
    if (current_app_) {
        stopCurrentApp();
    }
    
    launcher_mode_ = true;
}

void Launcher::processLauncherUI() {
    // Handle launcher UI input
    // This will be implemented when input system is available
    
    // Handle touch input for app selection
    // This will be implemented when touch input is available
    
    // Handle keyboard input for navigation
    // This will be implemented when keyboard input is available
}

void Launcher::processSystemInput() {
    // Handle system-wide input (like home button)
    // This will be implemented when input system is available
}

void Launcher::updateDisplay() {
    if (launcher_mode_) {
        // Update launcher display
        // This will be implemented when display manager is available
        
        logDebug("LAUNCHER", "Launcher display updated - " + String(app_list_.size()) + " apps");
    } else if (current_app_) {
        // Let current app update display
        // The app's run() method handles this
    }
}

std::vector<AppInfo> Launcher::getAppList() const {
    return app_list_;
}

App* Launcher::getCurrentApp() const {
    return current_app_;
}

bool Launcher::isLauncherMode() const {
    return launcher_mode_;
}

uint8_t Launcher::getSelectedIndex() const {
    return selected_index_;
}

void Launcher::setSelectedIndex(uint8_t index) {
    if (index < app_list_.size()) {
        selected_index_ = index;
    }
}

String Launcher::getLauncherStatus() {
    DynamicJsonDocument doc(1024);
    
    doc["launcher_mode"] = launcher_mode_;
    doc["app_count"] = app_list_.size();
    doc["selected_index"] = selected_index_;
    
    if (current_app_) {
        doc["current_app"] = current_app_->getName();
    } else {
        doc["current_app"] = "None";
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ===== GLOBAL LAUNCHER INSTANCE =====
Launcher* g_launcher = nullptr;

// ===== LAUNCHER UTILITY FUNCTIONS =====

bool initializeLauncher() {
    if (g_launcher) {
        return true;
    }
    
    g_launcher = new Launcher();
    if (!g_launcher) {
        return false;
    }
    
    return g_launcher->initialize();
}

Launcher* getLauncher() {
    return g_launcher;
} 