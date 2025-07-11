/**
 * @file      config_manager.h
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Configuration management system for T-Deck-Pro OS
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SD.h>
#include <LittleFS.h>
#include <map>
#include <memory>
#include <vector>
#include "simple_logger.h"
#include "service_container.h"

/**
 * @brief Configuration Storage Backend
 */
enum class ConfigStorage {
    LITTLEFS,    // Internal flash storage
    SD_CARD,     // SD card storage
    EEPROM       // EEPROM storage (for small configs)
};

/**
 * @brief Configuration Value Types
 */
enum class ConfigValueType {
    STRING,
    INTEGER,
    FLOAT,
    BOOLEAN,
    JSON_OBJECT,
    JSON_ARRAY
};

/**
 * @brief Configuration Value Class
 * 
 * Wrapper for configuration values with type safety
 */
class ConfigValue {
private:
    ConfigValueType type;
    String string_value;
    int32_t int_value;
    float float_value;
    bool bool_value;
    JsonDocument json_value;
    
public:
    ConfigValue();
    ConfigValue(const String& value);
    ConfigValue(const char* value);
    ConfigValue(int32_t value);
    ConfigValue(float value);
    ConfigValue(bool value);
    ConfigValue(const JsonObject& value);
    ConfigValue(const JsonArray& value);
    
    // Type getters
    ConfigValueType getType() const { return type; }
    bool isString() const { return type == ConfigValueType::STRING; }
    bool isInteger() const { return type == ConfigValueType::INTEGER; }
    bool isFloat() const { return type == ConfigValueType::FLOAT; }
    bool isBoolean() const { return type == ConfigValueType::BOOLEAN; }
    bool isJsonObject() const { return type == ConfigValueType::JSON_OBJECT; }
    bool isJsonArray() const { return type == ConfigValueType::JSON_ARRAY; }
    
    // Value getters
    String asString() const;
    int32_t asInteger() const;
    float asFloat() const;
    bool asBoolean() const;
    JsonObject asJsonObject() const;
    JsonArray asJsonArray() const;
    
    // Conversion operators
    operator String() const { return asString(); }
    operator int32_t() const { return asInteger(); }
    operator float() const { return asFloat(); }
    operator bool() const { return asBoolean(); }
    
    String toString() const;
};

/**
 * @brief Configuration Section
 * 
 * Groups related configuration values
 */
class ConfigSection {
private:
    String section_name;
    std::map<String, ConfigValue> values;
    bool modified;
    
public:
    ConfigSection(const String& name);
    
    // Value management
    void setValue(const String& key, const ConfigValue& value);
    ConfigValue getValue(const String& key, const ConfigValue& default_value = ConfigValue()) const;
    bool hasValue(const String& key) const;
    bool removeValue(const String& key);
    void clear();
    
    // Convenience setters
    void setString(const String& key, const String& value);
    void setInteger(const String& key, int32_t value);
    void setFloat(const String& key, float value);
    void setBoolean(const String& key, bool value);
    
    // Convenience getters
    String getString(const String& key, const String& default_value = "") const;
    int32_t getInteger(const String& key, int32_t default_value = 0) const;
    float getFloat(const String& key, float default_value = 0.0f) const;
    bool getBoolean(const String& key, bool default_value = false) const;
    
    // Section management
    const String& getName() const { return section_name; }
    bool isModified() const { return modified; }
    void setModified(bool mod = true) { modified = mod; }
    size_t getValueCount() const { return values.size(); }
    std::vector<String> getKeys() const;
    
    // Serialization
    bool toJson(JsonObject& obj) const;
    bool fromJson(const JsonObject& obj);
};

/**
 * @brief Configuration Manager Class
 * 
 * Central configuration management for T-Deck-Pro OS
 */
class ConfigManager : public IService {
private:
    std::map<String, std::shared_ptr<ConfigSection>> sections;
    ConfigStorage storage_backend;
    String config_file_path;
    bool auto_save_enabled;
    uint32_t auto_save_interval_ms;
    uint32_t last_save_time;
    bool initialized;
    bool config_loaded;
    
    // File system references
    fs::FS* file_system;
    
public:
    ConfigManager(ConfigStorage backend = ConfigStorage::LITTLEFS);
    ~ConfigManager() override;
    
    // IService implementation
    bool initialize() override;
    void shutdown() override;
    const char* getServiceName() const override { return "ConfigManager"; }
    bool isInitialized() const override { return initialized; }
    
    // Configuration management
    bool loadConfig();
    bool saveConfig();
    bool resetConfig();
    void update(); // Called from main loop for auto-save
    
    // Section management
    std::shared_ptr<ConfigSection> getSection(const String& section_name);
    std::shared_ptr<ConfigSection> createSection(const String& section_name);
    bool hasSection(const String& section_name) const;
    bool removeSection(const String& section_name);
    std::vector<String> getSectionNames() const;
    
    // Direct value access (creates sections as needed)
    void setValue(const String& section, const String& key, const ConfigValue& value);
    ConfigValue getValue(const String& section, const String& key, const ConfigValue& default_value = ConfigValue()) const;
    
    // Convenience methods
    void setString(const String& section, const String& key, const String& value);
    void setInteger(const String& section, const String& key, int32_t value);
    void setFloat(const String& section, const String& key, float value);
    void setBoolean(const String& section, const String& key, bool value);
    
    String getString(const String& section, const String& key, const String& default_value = "") const;
    int32_t getInteger(const String& section, const String& key, int32_t default_value = 0) const;
    float getFloat(const String& section, const String& key, float default_value = 0.0f) const;
    bool getBoolean(const String& section, const String& key, bool default_value = false) const;
    
    // Configuration settings
    void setStorageBackend(ConfigStorage backend);
    void setConfigFilePath(const String& path);
    void setAutoSave(bool enabled, uint32_t interval_ms = 30000);
    
    // Status and diagnostics
    bool isConfigLoaded() const { return config_loaded; }
    size_t getSectionCount() const { return sections.size(); }
    String getConfigFilePath() const { return config_file_path; }
    ConfigStorage getStorageBackend() const { return storage_backend; }
    
    void printStatus() const;
    void printConfiguration() const;
    
private:
    bool initializeFileSystem();
    bool fileExists(const String& path) const;
    String readFile(const String& path) const;
    bool writeFile(const String& path, const String& content);
    void createDefaultConfig();
    String getDefaultConfigPath() const;
};

/**
 * @brief Global Configuration Manager Instance
 */
extern ConfigManager* GlobalConfigManager;

// Convenience macros for configuration access
#define GET_CONFIG_STRING(section, key, default_val) \
    (GlobalConfigManager ? GlobalConfigManager->getString(section, key, default_val) : default_val)

#define GET_CONFIG_INT(section, key, default_val) \
    (GlobalConfigManager ? GlobalConfigManager->getInteger(section, key, default_val) : default_val)

#define GET_CONFIG_FLOAT(section, key, default_val) \
    (GlobalConfigManager ? GlobalConfigManager->getFloat(section, key, default_val) : default_val)

#define GET_CONFIG_BOOL(section, key, default_val) \
    (GlobalConfigManager ? GlobalConfigManager->getBoolean(section, key, default_val) : default_val)

#define SET_CONFIG_STRING(section, key, value) \
    if (GlobalConfigManager) GlobalConfigManager->setString(section, key, value)

#define SET_CONFIG_INT(section, key, value) \
    if (GlobalConfigManager) GlobalConfigManager->setInteger(section, key, value)

#define SET_CONFIG_FLOAT(section, key, value) \
    if (GlobalConfigManager) GlobalConfigManager->setFloat(section, key, value)

#define SET_CONFIG_BOOL(section, key, value) \
    if (GlobalConfigManager) GlobalConfigManager->setBoolean(section, key, value)

#endif // CONFIG_MANAGER_H
