/**
 * @file launcher.h
 * @brief T-Deck-Pro Launcher - Main UI and Application Grid
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Handles main UI with app grid and system status
 */

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include <map>

#include "hal/board_config_corrected.h"

// ===== LAUNCHER STATES =====
enum class LauncherState {
    INIT,
    LOADING,
    READY,
    RUNNING_APP,
    ERROR
};

// ===== APP TYPES =====
enum class AppType {
    SYSTEM,     // Built-in system apps
    PLUGIN,     // SD card plugins
    UTILITY     // Utility functions
};

/**
 * @brief Application information
 */
struct AppInfo {
    String name;
    String description;
    String icon;
    AppType type;
    bool enabled;
    uint32_t last_used;
    size_t memory_usage;
};

/**
 * @brief System status information
 */
struct SystemStatus {
    float battery_voltage;
    uint8_t battery_percentage;
    bool usb_connected;
    bool wifi_connected;
    bool cellular_connected;
    String current_time;
    String uptime;
    size_t free_memory;
    size_t free_storage;
};

/**
 * @brief Launcher for T-Deck-Pro OS
 * @note Handles main UI with app grid and system status
 */
class Launcher {
public:
    Launcher();
    ~Launcher();
    
    /**
     * @brief Initialize launcher
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Check if launcher is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Get launcher state
     * @return Current launcher state
     */
    LauncherState getState() const { return current_state_; }
    
    /**
     * @brief Show launcher interface
     */
    void show();
    
    /**
     * @brief Hide launcher interface
     */
    void hide();
    
    /**
     * @brief Update launcher display
     */
    void update();
    
    /**
     * @brief Process launcher events
     */
    void process();
    
    /**
     * @brief Add application to launcher
     * @param app App information
     * @return true if successful
     */
    bool addApp(const AppInfo& app);
    
    /**
     * @brief Remove application from launcher
     * @param name App name
     * @return true if successful
     */
    bool removeApp(const String& name);
    
    /**
     * @brief Get application information
     * @param name App name
     * @return App information
     */
    AppInfo getApp(const String& name);
    
    /**
     * @brief Get all applications
     * @return Vector of app information
     */
    std::vector<AppInfo> getApps() const;
    
    /**
     * @brief Launch application
     * @param name App name
     * @return true if successful
     */
    bool launchApp(const String& name);
    
    /**
     * @brief Close current application
     */
    void closeApp();
    
    /**
     * @brief Get current running app
     * @return Current app name
     */
    String getCurrentApp() const { return current_app_; }
    
    /**
     * @brief Update system status
     * @param status System status
     */
    void updateSystemStatus(const SystemStatus& status);
    
    /**
     * @brief Get system status
     * @return System status
     */
    SystemStatus getSystemStatus() const { return system_status_; }
    
    /**
     * @brief Set launcher theme
     * @param theme Theme name
     */
    void setTheme(const String& theme);
    
    /**
     * @brief Get current theme
     * @return Theme name
     */
    String getTheme() const { return current_theme_; }
    
    /**
     * @brief Set grid layout
     * @param columns Number of columns
     * @param rows Number of rows
     */
    void setGridLayout(uint8_t columns, uint8_t rows);
    
    /**
     * @brief Get grid layout
     * @param columns Number of columns
     * @param rows Number of rows
     */
    void getGridLayout(uint8_t& columns, uint8_t& rows) const;
    
    /**
     * @brief Enable/disable app categories
     * @param system Enable system apps
     * @param plugin Enable plugin apps
     * @param utility Enable utility apps
     */
    void setAppCategories(bool system, bool plugin, bool utility);
    
    /**
     * @brief Get app category settings
     * @param system System apps enabled
     * @param plugin Plugin apps enabled
     * @param utility Utility apps enabled
     */
    void getAppCategories(bool& system, bool& plugin, bool& utility) const;
    
    /**
     * @brief Show settings screen
     */
    void showSettings();
    
    /**
     * @brief Show about screen
     */
    void showAbout();
    
    /**
     * @brief Show system status screen
     */
    void showSystemStatus();
    
    /**
     * @brief Handle touch input
     * @param x Touch X coordinate
     * @param y Touch Y coordinate
     * @param pressed true if pressed
     */
    void handleTouch(uint16_t x, uint16_t y, bool pressed);
    
    /**
     * @brief Handle key input
     * @param key Key code
     * @param pressed true if pressed
     */
    void handleKey(uint8_t key, bool pressed);
    
    /**
     * @brief Get launcher statistics
     * @return Statistics JSON string
     */
    String getStatistics();
    
    /**
     * @brief Reset launcher statistics
     */
    void resetStatistics();

private:
    // ===== UI OPERATIONS =====
    
    /**
     * @brief Initialize UI components
     * @return true if successful
     */
    bool initUI();
    
    /**
     * @brief Create main screen
     * @return true if successful
     */
    bool createMainScreen();
    
    /**
     * @brief Create app grid
     * @return true if successful
     */
    bool createAppGrid();
    
    /**
     * @brief Create status bar
     * @return true if successful
     */
    bool createStatusBar();
    
    /**
     * @brief Create navigation bar
     * @return true if successful
     */
    bool createNavigationBar();
    
    /**
     * @brief Update app grid
     */
    void updateAppGrid();
    
    /**
     * @brief Update status bar
     */
    void updateStatusBar();
    
    /**
     * @brief Update navigation bar
     */
    void updateNavigationBar();
    
    /**
     * @brief Create app button
     * @param app App information
     * @param index Button index
     * @return LVGL object
     */
    lv_obj_t* createAppButton(const AppInfo& app, uint8_t index);
    
    /**
     * @brief Handle app button click
     * @param app_name App name
     */
    void handleAppClick(const String& app_name);
    
    /**
     * @brief Set launcher state
     * @param state New state
     */
    void setState(LauncherState state);
    
    /**
     * @brief Log launcher event
     * @param event Event description
     */
    void logEvent(const String& event);
    
    /**
     * @brief Update launcher statistics
     * @param app_launched App name launched
     */
    void updateStatistics(const String& app_launched);

private:
    // ===== MEMBER VARIABLES =====
    bool initialized_;
    LauncherState current_state_;
    String current_app_;
    String current_theme_;
    
    // Applications
    std::vector<AppInfo> apps_;
    std::map<String, AppInfo> app_map_;
    
    // System status
    SystemStatus system_status_;
    
    // UI layout
    uint8_t grid_columns_;
    uint8_t grid_rows_;
    bool show_system_apps_;
    bool show_plugin_apps_;
    bool show_utility_apps_;
    
    // LVGL objects
    lv_obj_t* main_screen_;
    lv_obj_t* app_grid_;
    lv_obj_t* status_bar_;
    lv_obj_t* nav_bar_;
    std::vector<lv_obj_t*> app_buttons_;
    
    // Statistics
    uint32_t total_launches_;
    uint32_t app_switches_;
    uint32_t ui_updates_;
    uint32_t last_update_time_;
    
    // Timing
    uint32_t last_status_update_;
    uint32_t last_grid_update_;
    uint32_t update_interval_;
};

// ===== GLOBAL LAUNCHER INSTANCE =====
extern Launcher* g_launcher;

// ===== LAUNCHER UTILITY FUNCTIONS =====

/**
 * @brief Initialize global launcher
 * @return true if successful
 */
bool initializeLauncher();

/**
 * @brief Get global launcher instance
 * @return Launcher pointer
 */
Launcher* getLauncher();

/**
 * @brief Create default app information
 * @param name App name
 * @param description App description
 * @param type App type
 * @return App information
 */
AppInfo createAppInfo(const String& name, const String& description, AppType type);

/**
 * @brief Format battery percentage
 * @param voltage Battery voltage
 * @return Battery percentage
 */
uint8_t getBatteryPercentage(float voltage);

/**
 * @brief Format uptime string
 * @param uptime_ms Uptime in milliseconds
 * @return Uptime string
 */
String formatUptime(uint32_t uptime_ms);

/**
 * @brief Format memory size
 * @param bytes Memory size in bytes
 * @return Formatted size string
 */
String formatMemorySize(size_t bytes);

#endif // LAUNCHER_H 