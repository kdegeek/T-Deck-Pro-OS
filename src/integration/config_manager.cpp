/**
 * @file      config_manager.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Configuration management system implementation
 */

#include "config_manager.h"

// Global configuration manager instance
ConfigManager* GlobalConfigManager = nullptr;

// ConfigValue implementation
ConfigValue::ConfigValue() : type(ConfigValueType::STRING) {
    string_value = "";
    int_value = 0;
    float_value = 0.0f;
    bool_value = false;
}

ConfigValue::ConfigValue(const String& value) : type(ConfigValueType::STRING), string_value(value) {
    int_value = 0;
    float_value = 0.0f;
    bool_value = false;
}

ConfigValue::ConfigValue(const char* value) : type(ConfigValueType::STRING), string_value(String(value)) {
    int_value = 0;
    float_value = 0.0f;
    bool_value = false;
}

ConfigValue::ConfigValue(int32_t value) : type(ConfigValueType::INTEGER), int_value(value) {
    string_value = String(value);
    float_value = static_cast<float>(value);
    bool_value = (value != 0);
}

ConfigValue::ConfigValue(float value) : type(ConfigValueType::FLOAT), float_value(value) {
    string_value = String(value);
    int_value = static_cast<int32_t>(value);
    bool_value = (value != 0.0f);
}

ConfigValue::ConfigValue(bool value) : type(ConfigValueType::BOOLEAN), bool_value(value) {
    string_value = value ? "true" : "false";
    int_value = value ? 1 : 0;
    float_value = value ? 1.0f : 0.0f;
}

ConfigValue::ConfigValue(const JsonObject& value) : type(ConfigValueType::JSON_OBJECT) {
    json_value.set(value);
    string_value = "";
    int_value = 0;
    float_value = 0.0f;
    bool_value = false;
}

ConfigValue::ConfigValue(const JsonArray& value) : type(ConfigValueType::JSON_ARRAY) {
    json_value.set(value);
    string_value = "";
    int_value = 0;
    float_value = 0.0f;
    bool_value = false;
}

String ConfigValue::asString() const {
    switch (type) {
        case ConfigValueType::STRING:
            return string_value;
        case ConfigValueType::INTEGER:
            return String(int_value);
        case ConfigValueType::FLOAT:
            return String(float_value);
        case ConfigValueType::BOOLEAN:
            return bool_value ? "true" : "false";
        case ConfigValueType::JSON_OBJECT:
        case ConfigValueType::JSON_ARRAY:
            String result;
            serializeJson(json_value, result);
            return result;
    }
    return "";
}

int32_t ConfigValue::asInteger() const {
    switch (type) {
        case ConfigValueType::INTEGER:
            return int_value;
        case ConfigValueType::FLOAT:
            return static_cast<int32_t>(float_value);
        case ConfigValueType::BOOLEAN:
            return bool_value ? 1 : 0;
        case ConfigValueType::STRING:
            return string_value.toInt();
        default:
            return 0;
    }
}

float ConfigValue::asFloat() const {
    switch (type) {
        case ConfigValueType::FLOAT:
            return float_value;
        case ConfigValueType::INTEGER:
            return static_cast<float>(int_value);
        case ConfigValueType::BOOLEAN:
            return bool_value ? 1.0f : 0.0f;
        case ConfigValueType::STRING:
            return string_value.toFloat();
        default:
            return 0.0f;
    }
}

bool ConfigValue::asBoolean() const {
    switch (type) {
        case ConfigValueType::BOOLEAN:
            return bool_value;
        case ConfigValueType::INTEGER:
            return int_value != 0;
        case ConfigValueType::FLOAT:
            return float_value != 0.0f;
        case ConfigValueType::STRING:
            return string_value.equalsIgnoreCase("true") || string_value == "1";
        default:
            return false;
    }
}

JsonObject ConfigValue::asJsonObject() const {
    // TODO: Implement proper JsonObject support for ArduinoJson v7
    // For now, create a minimal empty object
    JsonDocument doc;
    return doc.to<JsonObject>();
}

JsonArray ConfigValue::asJsonArray() const {
    // TODO: Implement proper JsonArray support for ArduinoJson v7
    // For now, create a minimal empty array
    JsonDocument doc;
    return doc.to<JsonArray>();
}

String ConfigValue::toString() const {
    String result = "ConfigValue{type=";
    switch (type) {
        case ConfigValueType::STRING: result += "STRING"; break;
        case ConfigValueType::INTEGER: result += "INTEGER"; break;
        case ConfigValueType::FLOAT: result += "FLOAT"; break;
        case ConfigValueType::BOOLEAN: result += "BOOLEAN"; break;
        case ConfigValueType::JSON_OBJECT: result += "JSON_OBJECT"; break;
        case ConfigValueType::JSON_ARRAY: result += "JSON_ARRAY"; break;
    }
    result += ", value=" + asString() + "}";
    return result;
}

// ConfigSection implementation
ConfigSection::ConfigSection(const String& name) : section_name(name), modified(false) {
}

void ConfigSection::setValue(const String& key, const ConfigValue& value) {
    values[key] = value;
    modified = true;
}

ConfigValue ConfigSection::getValue(const String& key, const ConfigValue& default_value) const {
    auto it = values.find(key);
    return it != values.end() ? it->second : default_value;
}

bool ConfigSection::hasValue(const String& key) const {
    return values.find(key) != values.end();
}

bool ConfigSection::removeValue(const String& key) {
    auto it = values.find(key);
    if (it != values.end()) {
        values.erase(it);
        modified = true;
        return true;
    }
    return false;
}

void ConfigSection::clear() {
    if (!values.empty()) {
        values.clear();
        modified = true;
    }
}

void ConfigSection::setString(const String& key, const String& value) {
    setValue(key, ConfigValue(value));
}

void ConfigSection::setInteger(const String& key, int32_t value) {
    setValue(key, ConfigValue(value));
}

void ConfigSection::setFloat(const String& key, float value) {
    setValue(key, ConfigValue(value));
}

void ConfigSection::setBoolean(const String& key, bool value) {
    setValue(key, ConfigValue(value));
}

String ConfigSection::getString(const String& key, const String& default_value) const {
    return getValue(key, ConfigValue(default_value)).asString();
}

int32_t ConfigSection::getInteger(const String& key, int32_t default_value) const {
    return getValue(key, ConfigValue(default_value)).asInteger();
}

float ConfigSection::getFloat(const String& key, float default_value) const {
    return getValue(key, ConfigValue(default_value)).asFloat();
}

bool ConfigSection::getBoolean(const String& key, bool default_value) const {
    return getValue(key, ConfigValue(default_value)).asBoolean();
}

std::vector<String> ConfigSection::getKeys() const {
    std::vector<String> keys;
    for (const auto& pair : values) {
        keys.push_back(pair.first);
    }
    return keys;
}

bool ConfigSection::toJson(JsonObject& obj) const {
    for (const auto& pair : values) {
        const String& key = pair.first;
        const ConfigValue& value = pair.second;
        
        switch (value.getType()) {
            case ConfigValueType::STRING:
                obj[key] = value.asString();
                break;
            case ConfigValueType::INTEGER:
                obj[key] = value.asInteger();
                break;
            case ConfigValueType::FLOAT:
                obj[key] = value.asFloat();
                break;
            case ConfigValueType::BOOLEAN:
                obj[key] = value.asBoolean();
                break;
            case ConfigValueType::JSON_OBJECT:
                obj[key] = value.asJsonObject();
                break;
            case ConfigValueType::JSON_ARRAY:
                obj[key] = value.asJsonArray();
                break;
        }
    }
    return true;
}

bool ConfigSection::fromJson(const JsonObject& obj) {
    values.clear();
    
    for (JsonPair pair : obj) {
        String key = pair.key().c_str();
        JsonVariant value = pair.value();
        
        if (value.is<String>()) {
            setValue(key, ConfigValue(value.as<String>()));
        } else if (value.is<int>()) {
            setValue(key, ConfigValue(value.as<int32_t>()));
        } else if (value.is<float>()) {
            setValue(key, ConfigValue(value.as<float>()));
        } else if (value.is<bool>()) {
            setValue(key, ConfigValue(value.as<bool>()));
        } else if (value.is<JsonObject>()) {
            setValue(key, ConfigValue(value.as<JsonObject>()));
        } else if (value.is<JsonArray>()) {
            setValue(key, ConfigValue(value.as<JsonArray>()));
        }
    }
    
    modified = false;
    return true;
}

// ConfigManager implementation
ConfigManager::ConfigManager(ConfigStorage backend)
    : storage_backend(backend), auto_save_enabled(true), auto_save_interval_ms(30000),
      last_save_time(0), initialized(false), config_loaded(false), file_system(nullptr) {

    config_file_path = getDefaultConfigPath();
    // Don't log during static initialization - logger may not be ready
}

ConfigManager::~ConfigManager() {
    shutdown();
    LOG_INFO("ConfigManager", "Configuration manager destroyed");
}

bool ConfigManager::initialize() {
    LOG_INFO("ConfigManager", "Initializing configuration manager...");

    if (initialized) {
        LOG_WARN("ConfigManager", "Configuration manager already initialized");
        return true;
    }

    // Initialize file system
    if (!initializeFileSystem()) {
        LOG_ERROR("ConfigManager", "Failed to initialize file system");
        return false;
    }

    // Set global instance
    if (!GlobalConfigManager) {
        GlobalConfigManager = this;
    }

    // Load configuration
    if (!loadConfig()) {
        LOG_WARN("ConfigManager", "Failed to load configuration, creating default");
        createDefaultConfig();
    }

    initialized = true;
    LOG_INFO("ConfigManager", "Configuration manager initialized successfully");
    return true;
}

void ConfigManager::shutdown() {
    if (!initialized) {
        return;
    }

    LOG_INFO("ConfigManager", "Shutting down configuration manager...");

    // Save configuration if auto-save is enabled
    if (auto_save_enabled) {
        saveConfig();
    }

    // Clear all sections
    sections.clear();

    initialized = false;
    config_loaded = false;

    // Clear global instance if it's this manager
    if (GlobalConfigManager == this) {
        GlobalConfigManager = nullptr;
    }

    LOG_INFO("ConfigManager", "Configuration manager shut down");
}

bool ConfigManager::initializeFileSystem() {
    switch (storage_backend) {
        case ConfigStorage::LITTLEFS:
            if (!LittleFS.begin()) {
                LOG_WARN("ConfigManager", "LittleFS mount failed, attempting to format...");
                if (!LittleFS.begin(true)) {  // true = format if mount fails
                    LOG_ERROR("ConfigManager", "Failed to initialize and format LittleFS");
                    return false;
                }
                LOG_INFO("ConfigManager", "LittleFS formatted and initialized successfully");
            } else {
                LOG_INFO("ConfigManager", "LittleFS mounted successfully");
            }
            file_system = &LittleFS;
            LOG_INFO("ConfigManager", "LittleFS initialized for configuration storage");
            break;

        case ConfigStorage::SD_CARD:
            if (!SD.begin()) {
                LOG_ERROR("ConfigManager", "Failed to initialize SD card");
                return false;
            }
            file_system = &SD;
            LOG_INFO("ConfigManager", "SD card initialized for configuration storage");
            break;

        case ConfigStorage::EEPROM:
            // EEPROM initialization would go here
            LOG_WARN("ConfigManager", "EEPROM storage not yet implemented");
            return false;

        default:
            LOG_ERROR("ConfigManager", "Unknown storage backend");
            return false;
    }

    return true;
}

bool ConfigManager::loadConfig() {
    LOG_INFOF("ConfigManager", "Loading configuration from: %s", config_file_path.c_str());

    if (!file_system || !fileExists(config_file_path)) {
        LOG_WARNF("ConfigManager", "Configuration file not found: %s", config_file_path.c_str());
        return false;
    }

    String config_content = readFile(config_file_path);
    if (config_content.isEmpty()) {
        LOG_ERROR("ConfigManager", "Failed to read configuration file");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, config_content);

    if (error) {
        LOG_ERRORF("ConfigManager", "Failed to parse configuration JSON: %s", error.c_str());
        return false;
    }

    // Clear existing sections
    sections.clear();

    // Load sections from JSON
    if (!doc.is<JsonObject>()) {
        LOG_ERROR("ConfigManager", "Configuration root is not a JSON object");
        return false;
    }
    JsonObject root = doc.as<JsonObject>();
    for (JsonPair section_pair : root) {
        String section_name = section_pair.key().c_str();
        JsonObject section_obj = section_pair.value().as<JsonObject>();

        auto section = createSection(section_name);
        if (section && section->fromJson(section_obj)) {
            section->setModified(false);
        }
    }

    config_loaded = true;
    LOG_INFOF("ConfigManager", "Configuration loaded successfully (%d sections)", sections.size());
    return true;
}

bool ConfigManager::saveConfig() {
    LOG_INFOF("ConfigManager", "Saving configuration to: %s", config_file_path.c_str());

    if (!file_system) {
        LOG_ERROR("ConfigManager", "File system not initialized");
        return false;
    }

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();

    // Serialize all sections
    for (const auto& pair : sections) {
        const String& section_name = pair.first;
        const auto& section = pair.second;

        JsonObject section_obj = root[section_name].to<JsonObject>();
        section->toJson(section_obj);
        section->setModified(false);
    }

    String config_content;
    if (serializeJsonPretty(doc, config_content) == 0) {
        LOG_ERROR("ConfigManager", "Failed to serialize configuration JSON");
        return false;
    }

    if (!writeFile(config_file_path, config_content)) {
        LOG_ERROR("ConfigManager", "Failed to write configuration file");
        return false;
    }

    last_save_time = millis();
    LOG_INFO("ConfigManager", "Configuration saved successfully");
    return true;
}

void ConfigManager::update() {
    if (!initialized || !auto_save_enabled) {
        return;
    }

    uint32_t current_time = millis();
    if (current_time - last_save_time >= auto_save_interval_ms) {
        // Check if any section has been modified
        bool needs_save = false;
        for (const auto& pair : sections) {
            if (pair.second->isModified()) {
                needs_save = true;
                break;
            }
        }

        if (needs_save) {
            saveConfig();
        }
    }
}

std::shared_ptr<ConfigSection> ConfigManager::getSection(const String& section_name) {
    auto it = sections.find(section_name);
    return it != sections.end() ? it->second : nullptr;
}

std::shared_ptr<ConfigSection> ConfigManager::createSection(const String& section_name) {
    auto section = std::make_shared<ConfigSection>(section_name);
    sections[section_name] = section;
    return section;
}

bool ConfigManager::hasSection(const String& section_name) const {
    return sections.find(section_name) != sections.end();
}

String ConfigManager::getDefaultConfigPath() const {
    switch (storage_backend) {
        case ConfigStorage::LITTLEFS:
            return "/config.json";
        case ConfigStorage::SD_CARD:
            return "/config/config.json";
        default:
            return "/config.json";
    }
}

void ConfigManager::createDefaultConfig() {
    LOG_INFO("ConfigManager", "Creating default configuration");

    // Create system section with default values
    auto system_section = createSection("system");
    system_section->setString("device_name", "T-Deck-Pro");
    system_section->setString("version", "1.0.0");
    system_section->setBoolean("debug_enabled", true);
    system_section->setInteger("log_level", 2);

    // Create display section
    auto display_section = createSection("display");
    display_section->setInteger("refresh_interval", 5000);
    display_section->setBoolean("auto_refresh", true);
    display_section->setInteger("brightness", 100);

    // Create wifi section
    auto wifi_section = createSection("wifi");
    wifi_section->setString("ssid", "");
    wifi_section->setString("password", "");
    wifi_section->setBoolean("auto_connect", true);

    // Save default configuration
    saveConfig();
}

bool ConfigManager::fileExists(const String& path) const {
    return file_system && file_system->exists(path);
}

String ConfigManager::readFile(const String& path) const {
    if (!file_system) {
        return "";
    }

    File file = file_system->open(path, "r");
    if (!file) {
        return "";
    }

    String content = file.readString();
    file.close();
    return content;
}

bool ConfigManager::writeFile(const String& path, const String& content) {
    if (!file_system) {
        return false;
    }

    File file = file_system->open(path, "w");
    if (!file) {
        return false;
    }

    size_t bytes_written = file.print(content);
    file.close();
    return bytes_written == content.length();
}

void ConfigManager::setValue(const String& section, const String& key, const ConfigValue& value) {
    auto section_ptr = getSection(section);
    if (!section_ptr) {
        section_ptr = createSection(section);
    }
    section_ptr->setValue(key, value);
}

ConfigValue ConfigManager::getValue(const String& section, const String& key, const ConfigValue& default_value) const {
    auto it = sections.find(section);
    if (it != sections.end()) {
        return it->second->getValue(key, default_value);
    }
    return default_value;
}

void ConfigManager::setString(const String& section, const String& key, const String& value) {
    setValue(section, key, ConfigValue(value));
}

void ConfigManager::setInteger(const String& section, const String& key, int32_t value) {
    setValue(section, key, ConfigValue(value));
}

void ConfigManager::setFloat(const String& section, const String& key, float value) {
    setValue(section, key, ConfigValue(value));
}

void ConfigManager::setBoolean(const String& section, const String& key, bool value) {
    setValue(section, key, ConfigValue(value));
}

String ConfigManager::getString(const String& section, const String& key, const String& default_value) const {
    return getValue(section, key, ConfigValue(default_value)).asString();
}

int32_t ConfigManager::getInteger(const String& section, const String& key, int32_t default_value) const {
    return getValue(section, key, ConfigValue(default_value)).asInteger();
}

float ConfigManager::getFloat(const String& section, const String& key, float default_value) const {
    return getValue(section, key, ConfigValue(default_value)).asFloat();
}

bool ConfigManager::getBoolean(const String& section, const String& key, bool default_value) const {
    return getValue(section, key, ConfigValue(default_value)).asBoolean();
}

void ConfigManager::printStatus() const {
    LOG_INFO("ConfigManager", "=== Configuration Manager Status ===");
    LOG_INFOF("ConfigManager", "Initialized: %s", initialized ? "true" : "false");
    LOG_INFOF("ConfigManager", "Config loaded: %s", config_loaded ? "true" : "false");
    LOG_INFOF("ConfigManager", "Storage backend: %s",
              storage_backend == ConfigStorage::LITTLEFS ? "LittleFS" :
              storage_backend == ConfigStorage::SD_CARD ? "SD_CARD" : "EEPROM");
    LOG_INFOF("ConfigManager", "Config file: %s", config_file_path.c_str());
    LOG_INFOF("ConfigManager", "Auto-save: %s (interval: %lums)",
              auto_save_enabled ? "enabled" : "disabled", auto_save_interval_ms);
    LOG_INFOF("ConfigManager", "Sections: %d", sections.size());
}
