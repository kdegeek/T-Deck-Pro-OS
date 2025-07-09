/**
 * @file launcher.h
 * @brief T-Deck-Pro Launcher - Main OS interface
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include "../config/os_config.h"

/**
 * @brief App icon information for launcher
 */
struct AppIcon {
    String name;
    String display_name;
    String icon_path;
    String description;
    bool is_plugin;
    bool has_notification;
    int notification_count;
    lv_obj_t* icon_obj;
    lv_obj_t* label_obj;
    lv_obj_t* badge_obj;
};

/**
 * @brief System status information
 */
struct SystemStatus {
    uint16_t battery_voltage;
    int battery_percentage;
    bool usb_connected;
    bool wifi_connected;
    bool cellular_connected;
    bool mqtt_connected;
    bool tailscale_connected;
    String wifi_ssid;
    int wifi_signal;
    String cellular_operator;
    int cellular_signal;
    float temperature;
    uint32_t uptime;
    uint32_t free_memory;
};

/**
 * @brief Main launcher interface for the simplified OS
 */
class Launcher {
public:
    Launcher();
    ~Launcher();
    
    /**
     * @brief Initialize the launcher
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Show the launcher interface
     */
    void show();
    
    /**
     * @brief Hide the launcher interface
     */
    void hide();
    
    /**
     * @brief Update launcher (call periodically)
     */
    void update();
    
    /**
     * @brief Show boot complete screen
     * @param boot_duration Boot time in milliseconds
     */
    void show_boot_complete(uint32_t boot_duration);
    
    /**
     * @brief Add app icon to launcher
     * @param app_info App information
     * @return true if successful
     */
    bool add_app_icon(const AppIcon& app_info);
    
    /**
     * @brief Remove app icon from launcher
     * @param app_name App name to remove
     * @return true if successful
     */
    bool remove_app_icon(const String& app_name);
    
    /**
     * @brief Update app notification badge
     * @param app_name App name
     * @param count Notification count (0 to hide badge)
     * @return true if successful
     */
    bool update_app_notification(const String& app_name, int count);
    
    /**
     * @brief Update system status display
     * @param status System status information
     */
    void update_system_status(const SystemStatus& status);
    
    /**
     * @brief Show settings panel
     */
    void show_settings();
    
    /**
     * @brief Show system information
     */
    void show_system_info();
    
    /**
     * @brief Handle app launch request
     * @param app_name App to launch
     * @return true if launch initiated
     */
    bool launch_app(const String& app_name);
    
    /**
     * @brief Check if launcher is currently visible
     * @return true if visible
     */
    bool is_visible() const;
    
    /**
     * @brief Refresh app list from SD card
     */
    void refresh_app_list();
    
private:
    /**
     * @brief Create the main launcher UI
     */
    void create_ui();
    
    /**
     * @brief Create status bar
     */
    void create_status_bar();
    
    /**
     * @brief Create app grid
     */
    void create_app_grid();
    
    /**
     * @brief Create bottom navigation
     */
    void create_bottom_nav();
    
    /**
     * @brief Update status bar content
     */
    void update_status_bar();
    
    /**
     * @brief Update app grid layout
     */
    void update_app_grid();
    
    /**
     * @brief Handle app icon click
     * @param event LVGL event
     */
    static void app_icon_clicked(lv_event_t* event);
    
    /**
     * @brief Handle settings button click
     * @param event LVGL event
     */
    static void settings_clicked(lv_event_t* event);
    
    /**
     * @brief Handle system info button click
     * @param event LVGL event
     */
    static void system_info_clicked(lv_event_t* event);
    
    /**
     * @brief Create app icon object
     * @param parent Parent container
     * @param app_info App information
     * @param x X position
     * @param y Y position
     * @return Created icon object
     */
    lv_obj_t* create_app_icon_obj(lv_obj_t* parent, const AppIcon& app_info, int x, int y);
    
    /**
     * @brief Create notification badge
     * @param parent Parent object
     * @param count Notification count
     * @return Created badge object
     */
    lv_obj_t* create_notification_badge(lv_obj_t* parent, int count);
    
    /**
     * @brief Load built-in apps
     */
    void load_builtin_apps();
    
    /**
     * @brief Load plugin apps from SD card
     */
    void load_plugin_apps();
    
    /**
     * @brief Get battery icon based on level and charging status
     * @param percentage Battery percentage
     * @param charging Is charging
     * @return Battery icon symbol
     */
    String get_battery_icon(int percentage, bool charging);
    
    /**
     * @brief Get signal strength icon
     * @param signal_strength Signal strength (0-100)
     * @return Signal icon symbol
     */
    String get_signal_icon(int signal_strength);
    
    // UI state
    bool initialized;
    bool visible;
    lv_obj_t* main_container;
    lv_obj_t* status_bar;
    lv_obj_t* app_grid;
    lv_obj_t* bottom_nav;
    
    // Status bar elements
    lv_obj_t* time_label;
    lv_obj_t* battery_label;
    lv_obj_t* wifi_label;
    lv_obj_t* cellular_label;
    lv_obj_t* mqtt_label;
    lv_obj_t* tailscale_label;
    
    // Bottom navigation elements
    lv_obj_t* settings_btn;
    lv_obj_t* info_btn;
    lv_obj_t* refresh_btn;
    
    // App management
    std::vector<AppIcon> app_icons;
    SystemStatus current_status;
    
    // Layout configuration
    static const int ICON_SIZE = 64;
    static const int ICON_SPACING = 20;
    static const int ICONS_PER_ROW = 3;
    static const int STATUS_BAR_HEIGHT = 30;
    static const int BOTTOM_NAV_HEIGHT = 50;
    
    // Static instance for event callbacks
    static Launcher* instance;
};

#endif // LAUNCHER_H