/**
 * @file plugin_manager.cpp
 * @brief T-Deck-Pro Plugin Manager Implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "plugin_manager.h"
#include <SPIFFS.h>
#include <esp_system.h>

// Static file paths
const String PluginManager::PLUGINS_DIR = "/sd/apps";
const String PluginManager::DATA_DIR = "/sd/data";
const String PluginManager::REGISTRY_FILE = "/plugins/registry.json";
const String PluginManager::MANIFEST_FILE = "manifest.json";

PluginManager::PluginManager() :
    sd_card_available(false),
    plugins_directory(PLUGINS_DIR),
    data_directory(DATA_DIR),
    max_running_plugins(MAX_PLUGIN_INSTANCES),
    plugin_timeout(DEFAULT_PLUGIN_TIMEOUT),
    auto_restart_crashed(true),
    sandbox_enabled(true),
    last_scan(0),
    last_monitor(0),
    last_cleanup(0) {
    
    // Initialize statistics
    memset(&stats, 0, sizeof(stats));
}

PluginManager::~PluginManager() {
    stop_all_plugins(true);
}

bool PluginManager::initialize() {
    Serial.println("[Plugin] Initializing Plugin Manager");
    
    // Check SD card availability
    if (SD.begin(BOARD_SD_CS)) {
        sd_card_available = true;
        Serial.println("[Plugin] SD card detected");
        
        // Create directories if they don't exist
        if (!SD.exists(plugins_directory)) {
            SD.mkdir(plugins_directory);
            Serial.printf("[Plugin] Created directory: %s\n", plugins_directory.c_str());
        }
        
        if (!SD.exists(data_directory)) {
            SD.mkdir(data_directory);
            Serial.printf("[Plugin] Created directory: %s\n", data_directory.c_str());
        }
        
        // Load plugin registry
        load_plugin_registry();
        
        // Initial plugin scan
        scan_plugins();
        
        return true;
    } else {
        Serial.println("[Plugin] SD card not available");
        return false;
    }
}

void PluginManager::update() {
    if (!sd_card_available) {
        return;
    }
    
    uint32_t now = millis();
    
    // Periodic plugin scan
    if (now - last_scan > SCAN_INTERVAL) {
        scan_plugins();
        last_scan = now;
    }
    
    // Monitor running plugins
    if (now - last_monitor > MONITOR_INTERVAL) {
        monitor_plugins();
        last_monitor = now;
    }
    
    // Cleanup old data
    if (now - last_cleanup > CLEANUP_INTERVAL) {
        cleanup_old_data();
        last_cleanup = now;
    }
}

int PluginManager::scan_plugins() {
    if (!sd_card_available) {
        return 0;
    }
    
    Serial.println("[Plugin] Scanning for plugins");
    
    available_plugins.clear();
    
    File root = SD.open(plugins_directory);
    if (!root || !root.isDirectory()) {
        Serial.println("[Plugin] Failed to open plugins directory");
        return 0;
    }
    
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            String plugin_path = plugins_directory + "/" + String(file.name());
            PluginInfo plugin = load_plugin_manifest(plugin_path);
            
            if (!plugin.name.isEmpty()) {
                available_plugins.push_back(plugin);
                Serial.printf("[Plugin] Found plugin: %s v%s\n", 
                             plugin.display_name.c_str(), plugin.version.c_str());
            }
        }
        file = root.openNextFile();
    }
    
    root.close();
    
    stats.total_plugins = available_plugins.size();
    stats.last_scan_time = millis();
    
    // Save updated registry
    save_plugin_registry();
    
    Serial.printf("[Plugin] Found %d plugins\n", available_plugins.size());
    return available_plugins.size();
}

std::vector<PluginInfo> PluginManager::get_available_plugins() const {
    return available_plugins;
}

std::vector<PluginInstance> PluginManager::get_running_plugins() const {
    return running_plugins;
}

bool PluginManager::launch_plugin(const String& plugin_name, const String& parameters) {
    // Check if plugin is already running
    for (const auto& instance : running_plugins) {
        if (instance.info.name == plugin_name) {
            Serial.printf("[Plugin] Plugin %s is already running\n", plugin_name.c_str());
            return false;
        }
    }
    
    // Check running plugin limit
    if (running_plugins.size() >= max_running_plugins) {
        Serial.println("[Plugin] Maximum running plugins reached");
        return false;
    }
    
    // Find plugin
    PluginInfo plugin;
    bool found = false;
    for (const auto& p : available_plugins) {
        if (p.name == plugin_name && p.enabled) {
            plugin = p;
            found = true;
            break;
        }
    }
    
    if (!found) {
        Serial.printf("[Plugin] Plugin %s not found or disabled\n", plugin_name.c_str());
        return false;
    }
    
    Serial.printf("[Plugin] Launching plugin: %s\n", plugin.display_name.c_str());
    
    // Create plugin instance
    PluginInstance instance;
    instance.info = plugin;
    instance.state = PluginState::STARTING;
    instance.start_time = millis();
    instance.memory_usage = 0;
    instance.auto_restart = auto_restart_crashed;
    
    // Create sandbox if enabled
    if (sandbox_enabled) {
        if (!create_plugin_sandbox(plugin)) {
            Serial.printf("[Plugin] Failed to create sandbox for %s\n", plugin_name.c_str());
            return false;
        }
    }
    
    // Execute plugin
    if (execute_plugin(plugin, parameters)) {
        instance.state = PluginState::RUNNING;
        running_plugins.push_back(instance);
        
        // Update statistics
        stats.running_plugins++;
        
        // Update last run time
        for (auto& p : available_plugins) {
            if (p.name == plugin_name) {
                p.last_run = millis();
                break;
            }
        }
        
        Serial.printf("[Plugin] Plugin %s launched successfully\n", plugin_name.c_str());
        return true;
    } else {
        instance.state = PluginState::ERROR;
        instance.error_message = "Failed to execute plugin";
        
        if (sandbox_enabled) {
            cleanup_plugin_sandbox(plugin_name);
        }
        
        Serial.printf("[Plugin] Failed to launch plugin %s\n", plugin_name.c_str());
        return false;
    }
}

bool PluginManager::stop_plugin(const String& plugin_name, bool force) {
    for (auto it = running_plugins.begin(); it != running_plugins.end(); ++it) {
        if (it->info.name == plugin_name) {
            Serial.printf("[Plugin] Stopping plugin: %s\n", plugin_name.c_str());
            
            it->state = PluginState::STOPPING;
            
            // Cleanup sandbox
            if (sandbox_enabled) {
                cleanup_plugin_sandbox(plugin_name);
            }
            
            // Remove from running list
            running_plugins.erase(it);
            
            // Update statistics
            stats.running_plugins--;
            
            Serial.printf("[Plugin] Plugin %s stopped\n", plugin_name.c_str());
            return true;
        }
    }
    
    Serial.printf("[Plugin] Plugin %s not running\n", plugin_name.c_str());
    return false;
}

bool PluginManager::pause_plugin(const String& plugin_name) {
    for (auto& instance : running_plugins) {
        if (instance.info.name == plugin_name && instance.state == PluginState::RUNNING) {
            instance.state = PluginState::PAUSED;
            Serial.printf("[Plugin] Plugin %s paused\n", plugin_name.c_str());
            return true;
        }
    }
    return false;
}

bool PluginManager::resume_plugin(const String& plugin_name) {
    for (auto& instance : running_plugins) {
        if (instance.info.name == plugin_name && instance.state == PluginState::PAUSED) {
            instance.state = PluginState::RUNNING;
            Serial.printf("[Plugin] Plugin %s resumed\n", plugin_name.c_str());
            return true;
        }
    }
    return false;
}

bool PluginManager::restart_plugin(const String& plugin_name) {
    if (stop_plugin(plugin_name)) {
        delay(1000); // Wait for cleanup
        return launch_plugin(plugin_name);
    }
    return false;
}

InstallResult PluginManager::install_plugin(const String& plugin_path) {
    InstallResult result = {false, "", "", ""};
    
    if (!sd_card_available) {
        result.error_message = "SD card not available";
        return result;
    }
    
    Serial.printf("[Plugin] Installing plugin from: %s\n", plugin_path.c_str());
    
    // Load and validate manifest
    PluginInfo plugin = load_plugin_manifest(plugin_path);
    if (plugin.name.isEmpty()) {
        result.error_message = "Invalid plugin manifest";
        return result;
    }
    
    // Check if plugin already exists
    for (const auto& existing : available_plugins) {
        if (existing.name == plugin.name) {
            result.error_message = "Plugin already installed";
            return result;
        }
    }
    
    // Copy plugin to plugins directory
    String dest_path = plugins_directory + "/" + plugin.name;
    if (SD.exists(dest_path)) {
        result.error_message = "Plugin directory already exists";
        return result;
    }
    
    // Create destination directory
    if (!SD.mkdir(dest_path)) {
        result.error_message = "Failed to create plugin directory";
        return result;
    }
    
    // Copy files (simplified - in real implementation would copy all files)
    // For now, just mark as installed
    plugin.install_time = millis();
    plugin.enabled = true;
    
    available_plugins.push_back(plugin);
    update_plugin_registry(plugin);
    
    result.success = true;
    result.plugin_name = plugin.name;
    result.version = plugin.version;
    
    Serial.printf("[Plugin] Plugin %s v%s installed successfully\n", 
                 plugin.display_name.c_str(), plugin.version.c_str());
    
    return result;
}

bool PluginManager::uninstall_plugin(const String& plugin_name) {
    // Stop plugin if running
    stop_plugin(plugin_name, true);
    
    // Remove from available plugins
    for (auto it = available_plugins.begin(); it != available_plugins.end(); ++it) {
        if (it->name == plugin_name) {
            String plugin_path = plugins_directory + "/" + plugin_name;
            
            // Remove plugin directory (simplified)
            // In real implementation, would recursively delete directory
            
            available_plugins.erase(it);
            remove_from_registry(plugin_name);
            
            Serial.printf("[Plugin] Plugin %s uninstalled\n", plugin_name.c_str());
            return true;
        }
    }
    
    return false;
}

bool PluginManager::set_plugin_enabled(const String& plugin_name, bool enabled) {
    for (auto& plugin : available_plugins) {
        if (plugin.name == plugin_name) {
            plugin.enabled = enabled;
            update_plugin_registry(plugin);
            
            if (!enabled) {
                stop_plugin(plugin_name, true);
            }
            
            Serial.printf("[Plugin] Plugin %s %s\n", 
                         plugin_name.c_str(), enabled ? "enabled" : "disabled");
            return true;
        }
    }
    return false;
}

PluginInfo PluginManager::get_plugin_info(const String& plugin_name) const {
    for (const auto& plugin : available_plugins) {
        if (plugin.name == plugin_name) {
            return plugin;
        }
    }
    return PluginInfo(); // Empty struct
}

PluginState PluginManager::get_plugin_state(const String& plugin_name) const {
    for (const auto& instance : running_plugins) {
        if (instance.info.name == plugin_name) {
            return instance.state;
        }
    }
    return PluginState::STOPPED;
}

bool PluginManager::is_plugin_running(const String& plugin_name) const {
    PluginState state = get_plugin_state(plugin_name);
    return state == PluginState::RUNNING || state == PluginState::PAUSED;
}

String PluginManager::get_plugin_by_mqtt_topic(const String& topic) const {
    for (const auto& plugin : available_plugins) {
        if (!plugin.mqtt_topics.isEmpty()) {
            // Simple topic matching (in real implementation, would use proper MQTT topic matching)
            if (plugin.mqtt_topics.indexOf(topic) >= 0) {
                return plugin.name;
            }
        }
    }
    return "";
}

std::vector<String> PluginManager::get_plugins_by_category(const String& category) const {
    std::vector<String> result;
    for (const auto& plugin : available_plugins) {
        if (plugin.category == category) {
            result.push_back(plugin.name);
        }
    }
    return result;
}

int PluginManager::start_autostart_plugins() {
    int started = 0;
    for (const auto& plugin : available_plugins) {
        if (plugin.enabled && plugin.autostart && !is_plugin_running(plugin.name)) {
            if (launch_plugin(plugin.name)) {
                started++;
            }
        }
    }
    
    Serial.printf("[Plugin] Started %d autostart plugins\n", started);
    return started;
}

int PluginManager::stop_all_plugins(bool force) {
    int stopped = 0;
    
    // Create a copy of the vector to avoid iterator invalidation
    auto plugins_to_stop = running_plugins;
    
    for (const auto& instance : plugins_to_stop) {
        if (stop_plugin(instance.info.name, force)) {
            stopped++;
        }
    }
    
    Serial.printf("[Plugin] Stopped %d plugins\n", stopped);
    return stopped;
}

String PluginManager::get_statistics() const {
    DynamicJsonDocument doc(1024);
    
    doc["total_plugins"] = stats.total_plugins;
    doc["running_plugins"] = stats.running_plugins;
    doc["enabled_plugins"] = stats.enabled_plugins;
    doc["crashed_plugins"] = stats.crashed_plugins;
    doc["total_memory_usage"] = stats.total_memory_usage;
    doc["last_scan_time"] = stats.last_scan_time;
    doc["last_error"] = stats.last_error;
    doc["sd_card_available"] = sd_card_available;
    doc["max_running_plugins"] = max_running_plugins;
    doc["sandbox_enabled"] = sandbox_enabled;
    
    String result;
    serializeJson(doc, result);
    return result;
}

String PluginManager::get_plugin_log(const String& plugin_name, int lines) const {
    String log_path = get_plugin_log_path(plugin_name);
    
    if (!SD.exists(log_path)) {
        return "No log file found";
    }
    
    File log_file = SD.open(log_path, "r");
    if (!log_file) {
        return "Failed to open log file";
    }
    
    String log_content = log_file.readString();
    log_file.close();
    
    // Return last N lines (simplified implementation)
    return log_content;
}

bool PluginManager::clear_plugin_log(const String& plugin_name) {
    String log_path = get_plugin_log_path(plugin_name);
    
    if (SD.exists(log_path)) {
        SD.remove(log_path);
        return true;
    }
    
    return false;
}

bool PluginManager::set_plugin_config(const String& plugin_name, const String& config_json) {
    PluginInfo plugin = get_plugin_info(plugin_name);
    if (plugin.name.isEmpty()) {
        return false;
    }
    
    String config_path = plugin.config_path;
    if (config_path.isEmpty()) {
        config_path = plugins_directory + "/" + plugin_name + "/config.json";
    }
    
    File config_file = SD.open(config_path, "w");
    if (config_file) {
        config_file.print(config_json);
        config_file.close();
        return true;
    }
    
    return false;
}

String PluginManager::get_plugin_config(const String& plugin_name) const {
    PluginInfo plugin = get_plugin_info(plugin_name);
    if (plugin.name.isEmpty()) {
        return "";
    }
    
    String config_path = plugin.config_path;
    if (config_path.isEmpty()) {
        config_path = plugins_directory + "/" + plugin_name + "/config.json";
    }
    
    if (!SD.exists(config_path)) {
        return "{}";
    }
    
    File config_file = SD.open(config_path, "r");
    if (config_file) {
        String config = config_file.readString();
        config_file.close();
        return config;
    }
    
    return "{}";
}

PluginInfo PluginManager::load_plugin_manifest(const String& plugin_path) {
    PluginInfo plugin;
    
    String manifest_path = plugin_path + "/" + MANIFEST_FILE;
    
    if (!SD.exists(manifest_path)) {
        return plugin; // Empty plugin
    }
    
    File manifest_file = SD.open(manifest_path, "r");
    if (!manifest_file) {
        return plugin;
    }
    
    String manifest_content = manifest_file.readString();
    manifest_file.close();
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, manifest_content);
    
    if (error) {
        Serial.printf("[Plugin] Failed to parse manifest: %s\n", manifest_path.c_str());
        return plugin;
    }
    
    if (!validate_plugin_manifest(doc)) {
        Serial.printf("[Plugin] Invalid manifest: %s\n", manifest_path.c_str());
        return plugin;
    }
    
    // Parse manifest
    plugin.name = doc["name"].as<String>();
    plugin.display_name = doc["display_name"].as<String>();
    plugin.version = doc["version"].as<String>();
    plugin.description = doc["description"].as<String>();
    plugin.author = doc["author"].as<String>();
    plugin.icon_path = plugin_path + "/" + doc["icon"].as<String>();
    plugin.executable_path = plugin_path + "/" + doc["executable"].as<String>();
    plugin.config_path = plugin_path + "/config.json";
    plugin.data_path = data_directory + "/" + plugin.name;
    plugin.enabled = doc["enabled"].as<bool>();
    plugin.autostart = doc["autostart"].as<bool>();
    plugin.priority = doc["priority"].as<int>();
    plugin.permissions = doc["permissions"].as<String>();
    plugin.dependencies = doc["dependencies"].as<String>();
    plugin.category = doc["category"].as<String>();
    plugin.mqtt_topics = doc["mqtt_topics"].as<String>();
    plugin.system_app = doc["system_app"].as<bool>();
    
    return plugin;
}

bool PluginManager::validate_plugin_manifest(const DynamicJsonDocument& manifest) {
    // Check required fields
    if (!manifest.containsKey("name") || 
        !manifest.containsKey("version") || 
        !manifest.containsKey("executable")) {
        return false;
    }
    
    // Validate name format
    String name = manifest["name"].as<String>();
    if (name.isEmpty() || name.indexOf(' ') >= 0) {
        return false;
    }
    
    return true;
}

bool PluginManager::execute_plugin(const PluginInfo& plugin, const String& parameters) {
    // In a real implementation, this would execute the plugin binary
    // For ESP32, this might involve loading and running code from SD card
    
    Serial.printf("[Plugin] Executing: %s\n", plugin.executable_path.c_str());
    
    // Check if executable exists
    if (!SD.exists(plugin.executable_path)) {
        Serial.printf("[Plugin] Executable not found: %s\n", plugin.executable_path.c_str());
        return false;
    }
    
    // Check permissions
    if (!check_plugin_permissions(plugin, "execute")) {
        Serial.printf("[Plugin] Permission denied for: %s\n", plugin.name.c_str());
        return false;
    }
    
    // Simulate plugin execution
    // In real implementation, this would load and execute the plugin code
    
    return true;
}

void PluginManager::monitor_plugins() {
    for (auto& instance : running_plugins) {
        // Check plugin health
        if (instance.state == PluginState::RUNNING) {
            // Simulate plugin monitoring
            // In real implementation, this would check if the plugin process is still alive
            
            // Random crash simulation for testing
            if (random(0, 10000) < 5) { // 0.05% chance
                handle_plugin_crash(instance.info.name, "Simulated crash");
            }
        }
        
        // Update memory usage (simulated)
        instance.memory_usage = random(1000, 5000);
    }
    
    // Update statistics
    stats.running_plugins = running_plugins.size();
    stats.total_memory_usage = 0;
    for (const auto& instance : running_plugins) {
        stats.total_memory_usage += instance.memory_usage;
    }
}

void PluginManager::handle_plugin_crash(const String& plugin_name, const String& error_message) {
    Serial.printf("[Plugin] Plugin crashed: %s - %s\n", plugin_name.c_str(), error_message.c_str());
    
    for (auto& instance : running_plugins) {
        if (instance.info.name == plugin_name) {
            instance.state = PluginState::CRASHED;
            instance.error_message = error_message;
            
            stats.crashed_plugins++;
            
            // Auto-restart if enabled
            if (instance.auto_restart) {
                Serial.printf("[Plugin] Auto-restarting plugin: %s\n", plugin_name.c_str());
                restart_plugin(plugin_name);
            }
            
            break;
        }
    }
}

bool PluginManager::create_plugin_sandbox(const PluginInfo& plugin) {
    // Create isolated environment for plugin
    String sandbox_path = data_directory + "/sandbox/" + plugin.name;
    
    if (!SD.exists(sandbox_path)) {
        SD.mkdir(sandbox_path);
    }
    
    return true;
}

void PluginManager::cleanup_plugin_sandbox(const String& plugin_name) {
    // Cleanup plugin sandbox
    String sandbox_path = data_directory + "/sandbox/" + plugin_name;
    
    // In real implementation, would recursively delete directory
    Serial.printf("[Plugin] Cleaning up sandbox: %s\n", sandbox_path.c_str());
}

bool PluginManager::check_plugin_permissions(const PluginInfo& plugin, const String& operation) {
    // Simple permission checking
    // In real implementation, would have more sophisticated permission system
    
    if (plugin.system_app) {
        return true; // System apps have all permissions
    }
    
    // Check specific permissions
    if (operation == "execute" && plugin.permissions.indexOf("execute") >= 0) {
        return true;
    }
    
    return false;
}

String PluginManager::get_plugin_data_dir(const String& plugin_name) const {
    return data_directory + "/" + plugin_name;
}

String PluginManager::get_plugin_log_path(const String& plugin_name) const {
    return data_directory + "/" + plugin_name + "/plugin.log";
}

void PluginManager::save_plugin_registry() {
    if (!SPIFFS.begin()) {
        return;
    }
    
    DynamicJsonDocument doc(4096);
    JsonArray plugins = doc.createNestedArray("plugins");
    
    for (const auto& plugin : available_plugins) {
        JsonObject p = plugins.createNestedObject();
        p["name"] = plugin.name;
        p["enabled"] = plugin.enabled;
        p["install_time"] = plugin.install_time;
        p["last_run"] = plugin.last_run;
    }
    
    File file = SPIFFS.open(REGISTRY_FILE, "w");
    if (file) {
        serializeJson(doc, file);
        file.close();
    }
}

void PluginManager::load_plugin_registry() {
    if (!SPIFFS.begin()) {
        return;
    }
    
    File file = SPIFFS.open(REGISTRY_FILE, "r");
    if (!file) {
        return;
    }
    
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        return;
    }
    
    // Registry will be merged with scanned plugins
    Serial.println("[Plugin] Plugin registry loaded");
}

void PluginManager::update_plugin_registry(const PluginInfo& plugin) {
    // Update plugin in available_plugins list
    for (auto& p : available_plugins) {
        if (p.name == plugin.name) {
            p = plugin;
            break;
        }
    }
    
    save_plugin_registry();
}

void PluginManager::remove_from_registry(const String& plugin_name) {
    available_plugins.erase(
        std::remove_if(available_plugins.begin(), available_plugins.end(),
            [&plugin_name](const PluginInfo& p) {
                return p.name == plugin_name;
            }),
        available_plugins.end());
    
    save_plugin_registry();
}

void PluginManager::cleanup_old_data() {
    // Remove old log files, temporary data, etc.
    Serial.println("[Plugin] Cleaning up old plugin data");
    
    // In real implementation, would clean up old logs, temporary files, etc.
}