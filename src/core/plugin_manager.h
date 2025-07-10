/**
 * @file plugin_manager.h
 * @brief T-Deck-Pro Plugin Manager - SD Card Application Management
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Handles dynamic loading of applications from SD card
 */

#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <functional>

#include "hal/board_config_corrected.h"

// ===== PLUGIN STATES =====
enum class PluginState {
    INIT,
    SCANNING,
    LOADING,
    READY,
    ERROR
};

// ===== PLUGIN TYPES =====
enum class PluginType {
    APPLICATION,    // Full application
    WIDGET,        // UI widget
    SERVICE,       // Background service
    DRIVER         // Hardware driver
};

// ===== PLUGIN CONFIGURATION =====
#define PLUGIN_MAX_NAME_LENGTH 32
#define PLUGIN_MAX_PATH_LENGTH 128
#define PLUGIN_MAX_MEMORY_USAGE 1048576  // 1MB
#define PLUGIN_SCAN_INTERVAL_MS 30000

/**
 * @brief Plugin information
 */
struct PluginInfo {
    String name;
    String description;
    String version;
    String author;
    String path;
    PluginType type;
    bool enabled;
    bool loaded;
    size_t memory_usage;
    uint32_t last_used;
    uint32_t load_time;
    String dependencies;
    String icon;
};

/**
 * @brief Plugin function pointers
 */
struct PluginFunctions {
    std::function<bool()> init;
    std::function<void()> run;
    std::function<void()> cleanup;
    std::function<String()> getStatus;
    std::function<void(const String&)> handleCommand;
};

/**
 * @brief Plugin manager for T-Deck-Pro
 * @note Handles dynamic loading of applications from SD card
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
     * @brief Check if plugin manager is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Get plugin manager state
     * @return Current plugin state
     */
    PluginState getState() const { return current_state_; }
    
    /**
     * @brief Scan for plugins on SD card
     * @return Number of plugins found
     */
    int scanPlugins();
    
    /**
     * @brief Load plugin by name
     * @param name Plugin name
     * @return true if successful
     */
    bool loadPlugin(const String& name);
    
    /**
     * @brief Unload plugin by name
     * @param name Plugin name
     * @return true if successful
     */
    bool unloadPlugin(const String& name);
    
    /**
     * @brief Launch plugin application
     * @param name Plugin name
     * @return true if successful
     */
    bool launchPlugin(const String& name);
    
    /**
     * @brief Stop plugin application
     * @param name Plugin name
     * @return true if successful
     */
    bool stopPlugin(const String& name);
    
    /**
     * @brief Get plugin information
     * @param name Plugin name
     * @return Plugin information
     */
    PluginInfo getPlugin(const String& name);
    
    /**
     * @brief Get all plugins
     * @return Vector of plugin information
     */
    std::vector<PluginInfo> getPlugins() const;
    
    /**
     * @brief Get loaded plugins
     * @return Vector of loaded plugin names
     */
    std::vector<String> getLoadedPlugins() const;
    
    /**
     * @brief Check if plugin is loaded
     * @param name Plugin name
     * @return true if loaded
     */
    bool isPluginLoaded(const String& name);
    
    /**
     * @brief Enable/disable plugin
     * @param name Plugin name
     * @param enabled true to enable
     * @return true if successful
     */
    bool setPluginEnabled(const String& name, bool enabled);
    
    /**
     * @brief Check if plugin is enabled
     * @param name Plugin name
     * @return true if enabled
     */
    bool isPluginEnabled(const String& name);
    
    /**
     * @brief Get plugin memory usage
     * @param name Plugin name
     * @return Memory usage in bytes
     */
    size_t getPluginMemoryUsage(const String& name);
    
    /**
     * @brief Get total memory usage
     * @return Total memory usage in bytes
     */
    size_t getTotalMemoryUsage();
    
    /**
     * @brief Get plugin status
     * @param name Plugin name
     * @return Status JSON string
     */
    String getPluginStatus(const String& name);
    
    /**
     * @brief Send command to plugin
     * @param name Plugin name
     * @param command Command string
     * @return true if successful
     */
    bool sendCommand(const String& name, const String& command);
    
    /**
     * @brief Install plugin from file
     * @param file_path Plugin file path
     * @return true if successful
     */
    bool installPlugin(const String& file_path);
    
    /**
     * @brief Uninstall plugin
     * @param name Plugin name
     * @return true if successful
     */
    bool uninstallPlugin(const String& name);
    
    /**
     * @brief Update plugin
     * @param name Plugin name
     * @param file_path Update file path
     * @return true if successful
     */
    bool updatePlugin(const String& name, const String& file_path);
    
    /**
     * @brief Get plugin dependencies
     * @param name Plugin name
     * @return Dependencies string
     */
    String getPluginDependencies(const String& name);
    
    /**
     * @brief Check plugin dependencies
     * @param name Plugin name
     * @return true if dependencies satisfied
     */
    bool checkPluginDependencies(const String& name);
    
    /**
     * @brief Process plugin events
     */
    void process();
    
    /**
     * @brief Get plugin manager statistics
     * @return Statistics JSON string
     */
    String getStatistics();
    
    /**
     * @brief Reset plugin manager statistics
     */
    void resetStatistics();

private:
    // ===== PLUGIN OPERATIONS =====
    
    /**
     * @brief Initialize plugin system
     * @return true if successful
     */
    bool initPluginSystem();
    
    /**
     * @brief Scan plugin directory
     * @param directory Directory path
     * @return Number of plugins found
     */
    int scanPluginDirectory(const String& directory);
    
    /**
     * @brief Load plugin from file
     * @param file_path Plugin file path
     * @return Plugin information
     */
    PluginInfo loadPluginFromFile(const String& file_path);
    
    /**
     * @brief Parse plugin manifest
     * @param manifest_path Manifest file path
     * @return Plugin information
     */
    PluginInfo parsePluginManifest(const String& manifest_path);
    
    /**
     * @brief Validate plugin
     * @param info Plugin information
     * @return true if valid
     */
    bool validatePlugin(const PluginInfo& info);
    
    /**
     * @brief Load plugin code
     * @param info Plugin information
     * @return true if successful
     */
    bool loadPluginCode(const PluginInfo& info);
    
    /**
     * @brief Unload plugin code
     * @param name Plugin name
     * @return true if successful
     */
    bool unloadPluginCode(const String& name);
    
    /**
     * @brief Set plugin state
     * @param state New state
     */
    void setState(PluginState state);
    
    /**
     * @brief Log plugin event
     * @param event Event description
     */
    void logEvent(const String& event);
    
    /**
     * @brief Update plugin statistics
     * @param plugin_name Plugin name
     * @param action Action performed
     */
    void updateStatistics(const String& plugin_name, const String& action);
    
    /**
     * @brief Handle plugin error
     * @param plugin_name Plugin name
     * @param error Error message
     */
    void handlePluginError(const String& plugin_name, const String& error);

private:
    // ===== MEMBER VARIABLES =====
    bool initialized_;
    PluginState current_state_;
    
    // Plugins
    std::vector<PluginInfo> plugins_;
    std::map<String, PluginInfo> plugin_map_;
    std::map<String, PluginFunctions> plugin_functions_;
    std::map<String, void*> plugin_handles_;
    
    // Plugin directory
    String plugin_directory_;
    
    // Statistics
    uint32_t total_plugins_;
    uint32_t loaded_plugins_;
    uint32_t plugin_loads_;
    uint32_t plugin_unloads_;
    uint32_t plugin_launches_;
    uint32_t plugin_errors_;
    
    // Timing
    uint32_t last_scan_time_;
    uint32_t last_check_time_;
    uint32_t scan_interval_;
    
    // Memory tracking
    size_t total_memory_usage_;
    std::map<String, size_t> plugin_memory_usage_;
};

// ===== GLOBAL PLUGIN MANAGER INSTANCE =====
extern PluginManager* g_plugin_manager;

// ===== PLUGIN UTILITY FUNCTIONS =====

/**
 * @brief Initialize global plugin manager
 * @return true if successful
 */
bool initializePluginManager();

/**
 * @brief Get global plugin manager instance
 * @return Plugin manager pointer
 */
PluginManager* getPluginManager();

/**
 * @brief Create plugin information
 * @param name Plugin name
 * @param description Plugin description
 * @param type Plugin type
 * @return Plugin information
 */
PluginInfo createPluginInfo(const String& name, const String& description, PluginType type);

/**
 * @brief Get plugin type string
 * @param type Plugin type
 * @return Type string
 */
String getPluginTypeString(PluginType type);

/**
 * @brief Parse plugin type from string
 * @param type_string Type string
 * @return Plugin type
 */
PluginType parsePluginType(const String& type_string);

/**
 * @brief Validate plugin file
 * @param file_path Plugin file path
 * @return true if valid
 */
bool validatePluginFile(const String& file_path);

/**
 * @brief Get plugin file size
 * @param file_path Plugin file path
 * @return File size in bytes
 */
size_t getPluginFileSize(const String& file_path);

#endif // PLUGIN_MANAGER_H 