/**
 * @file plugin_manager.h
 * @brief T-Deck-Pro Plugin Manager - SD card app management
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <vector>
#include "config/os_config_corrected.h"

/**
 * @brief Plugin/App information structure
 */
struct PluginInfo {
    String name;
    String display_name;
    String version;
    String description;
    String author;
    String icon_path;
    String executable_path;
    String config_path;
    String data_path;
    bool enabled;
    bool autostart;
    int priority;
    String permissions;
    String dependencies;
    uint32_t install_time;
    uint32_t last_run;
    String category;
    String mqtt_topics;
    bool system_app;
};

/**
 * @brief Plugin execution state
 */
enum class PluginState {
    STOPPED,
    STARTING,
    RUNNING,
    PAUSED,
    STOPPING,
    ERROR,
    CRASHED
};

/**
 * @brief Running plugin instance
 */
struct PluginInstance {
    PluginInfo info;
    PluginState state;
    uint32_t start_time;
    uint32_t memory_usage;
    String error_message;
    int exit_code;
    bool auto_restart;
};

/**
 * @brief Plugin installation result
 */
struct InstallResult {
    bool success;
    String error_message;
    String plugin_name;
    String version;
};

/**
 * @brief Plugin Manager handles SD card apps
 */
class PluginManager {
public:
    PluginManager();
    ~PluginManager();
    
    /**
     * @brief Initialize plugin manager
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Update plugin manager (call periodically)
     */
    void update();
    
    /**
     * @brief Scan SD card for plugins
     * @return Number of plugins found
     */
    int scan_plugins();
    
    /**
     * @brief Get list of available plugins
     * @return Vector of plugin information
     */
    std::vector<PluginInfo> get_available_plugins() const;
    
    /**
     * @brief Get list of running plugins
     * @return Vector of running plugin instances
     */
    std::vector<PluginInstance> get_running_plugins() const;
    
    /**
     * @brief Launch a plugin
     * @param plugin_name Plugin name to launch
     * @param parameters Launch parameters (optional)
     * @return true if launched successfully
     */
    bool launch_plugin(const String& plugin_name, const String& parameters = "");
    
    /**
     * @brief Stop a running plugin
     * @param plugin_name Plugin name to stop
     * @param force Force stop if true
     * @return true if stopped successfully
     */
    bool stop_plugin(const String& plugin_name, bool force = false);
    
    /**
     * @brief Pause a running plugin
     * @param plugin_name Plugin name to pause
     * @return true if paused successfully
     */
    bool pause_plugin(const String& plugin_name);
    
    /**
     * @brief Resume a paused plugin
     * @param plugin_name Plugin name to resume
     * @return true if resumed successfully
     */
    bool resume_plugin(const String& plugin_name);
    
    /**
     * @brief Restart a plugin
     * @param plugin_name Plugin name to restart
     * @return true if restarted successfully
     */
    bool restart_plugin(const String& plugin_name);
    
    /**
     * @brief Install a plugin from SD card
     * @param plugin_path Path to plugin package
     * @return Installation result
     */
    InstallResult install_plugin(const String& plugin_path);
    
    /**
     * @brief Uninstall a plugin
     * @param plugin_name Plugin name to uninstall
     * @return true if uninstalled successfully
     */
    bool uninstall_plugin(const String& plugin_name);
    
    /**
     * @brief Enable/disable a plugin
     * @param plugin_name Plugin name
     * @param enabled Enable state
     * @return true if successful
     */
    bool set_plugin_enabled(const String& plugin_name, bool enabled);
    
    /**
     * @brief Get plugin information
     * @param plugin_name Plugin name
     * @return Plugin information or empty struct if not found
     */
    PluginInfo get_plugin_info(const String& plugin_name) const;
    
    /**
     * @brief Get plugin state
     * @param plugin_name Plugin name
     * @return Plugin state
     */
    PluginState get_plugin_state(const String& plugin_name) const;
    
    /**
     * @brief Check if plugin is running
     * @param plugin_name Plugin name
     * @return true if running
     */
    bool is_plugin_running(const String& plugin_name) const;
    
    /**
     * @brief Get plugin by MQTT topic
     * @param topic MQTT topic
     * @return Plugin name or empty string if not found
     */
    String get_plugin_by_mqtt_topic(const String& topic) const;
    
    /**
     * @brief Get plugins by category
     * @param category Plugin category
     * @return Vector of plugin names
     */
    std::vector<String> get_plugins_by_category(const String& category) const;
    
    /**
     * @brief Start autostart plugins
     * @return Number of plugins started
     */
    int start_autostart_plugins();
    
    /**
     * @brief Stop all running plugins
     * @param force Force stop if true
     * @return Number of plugins stopped
     */
    int stop_all_plugins(bool force = false);
    
    /**
     * @brief Get plugin statistics
     * @return JSON string with plugin statistics
     */
    String get_statistics() const;
    
    /**
     * @brief Get plugin log
     * @param plugin_name Plugin name
     * @param lines Number of lines to retrieve
     * @return Plugin log content
     */
    String get_plugin_log(const String& plugin_name, int lines = 50) const;
    
    /**
     * @brief Clear plugin log
     * @param plugin_name Plugin name
     * @return true if successful
     */
    bool clear_plugin_log(const String& plugin_name);
    
    /**
     * @brief Set plugin configuration
     * @param plugin_name Plugin name
     * @param config_json JSON configuration
     * @return true if successful
     */
    bool set_plugin_config(const String& plugin_name, const String& config_json);
    
    /**
     * @brief Get plugin configuration
     * @param plugin_name Plugin name
     * @return JSON configuration string
     */
    String get_plugin_config(const String& plugin_name) const;
    
private:
    /**
     * @brief Load plugin manifest
     * @param plugin_path Path to plugin directory
     * @return Plugin information or empty struct if failed
     */
    PluginInfo load_plugin_manifest(const String& plugin_path);
    
    /**
     * @brief Validate plugin manifest
     * @param manifest Plugin manifest JSON
     * @return true if valid
     */
    bool validate_plugin_manifest(const DynamicJsonDocument& manifest);
    
    /**
     * @brief Execute plugin binary
     * @param plugin Plugin information
     * @param parameters Launch parameters
     * @return true if execution started
     */
    bool execute_plugin(const PluginInfo& plugin, const String& parameters);
    
    /**
     * @brief Monitor running plugins
     */
    void monitor_plugins();
    
    /**
     * @brief Handle plugin crash
     * @param plugin_name Plugin name
     * @param error_message Error message
     */
    void handle_plugin_crash(const String& plugin_name, const String& error_message);
    
    /**
     * @brief Create plugin sandbox
     * @param plugin Plugin information
     * @return true if successful
     */
    bool create_plugin_sandbox(const PluginInfo& plugin);
    
    /**
     * @brief Cleanup plugin sandbox
     * @param plugin_name Plugin name
     */
    void cleanup_plugin_sandbox(const String& plugin_name);
    
    /**
     * @brief Check plugin permissions
     * @param plugin Plugin information
     * @param operation Operation to check
     * @return true if allowed
     */
    bool check_plugin_permissions(const PluginInfo& plugin, const String& operation);
    
    /**
     * @brief Get plugin data directory
     * @param plugin_name Plugin name
     * @return Data directory path
     */
    String get_plugin_data_dir(const String& plugin_name) const;
    
    /**
     * @brief Get plugin log file path
     * @param plugin_name Plugin name
     * @return Log file path
     */
    String get_plugin_log_path(const String& plugin_name) const;
    
    /**
     * @brief Save plugin registry
     */
    void save_plugin_registry();
    
    /**
     * @brief Load plugin registry
     */
    void load_plugin_registry();
    
    /**
     * @brief Update plugin registry entry
     * @param plugin Plugin information
     */
    void update_plugin_registry(const PluginInfo& plugin);
    
    /**
     * @brief Remove plugin from registry
     * @param plugin_name Plugin name
     */
    void remove_from_registry(const String& plugin_name);
    
    /**
     * @brief Cleanup old plugin data
     */
    void cleanup_old_data();
    
    // Plugin storage
    std::vector<PluginInfo> available_plugins;
    std::vector<PluginInstance> running_plugins;
    
    // SD card status
    bool sd_card_available;
    String plugins_directory;
    String data_directory;
    
    // Statistics
    struct PluginStats {
        int total_plugins;
        int running_plugins;
        int enabled_plugins;
        int crashed_plugins;
        uint32_t total_memory_usage;
        uint32_t last_scan_time;
        String last_error;
    } stats;
    
    // Configuration
    int max_running_plugins;
    int plugin_timeout;
    bool auto_restart_crashed;
    bool sandbox_enabled;
    
    // Timing
    uint32_t last_scan;
    uint32_t last_monitor;
    uint32_t last_cleanup;
    
    // Constants
    static const uint32_t SCAN_INTERVAL = 30000;      // 30 seconds
    static const uint32_t MONITOR_INTERVAL = 5000;    // 5 seconds
    static const uint32_t CLEANUP_INTERVAL = 300000;  // 5 minutes
    static const int DEFAULT_PLUGIN_TIMEOUT = 30000;  // 30 seconds
    static const int MAX_PLUGIN_INSTANCES = 10;
    static const int MAX_LOG_SIZE = 10240;             // 10KB
    
    // File paths
    static const String PLUGINS_DIR;
    static const String DATA_DIR;
    static const String REGISTRY_FILE;
    static const String MANIFEST_FILE;
};

#endif // PLUGIN_MANAGER_H