#include "storage_manager.h"
#include "../utils/logger.h"
#include <SPIFFS.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <esp_heap_caps.h>

StorageManager* StorageManager::instance = nullptr;

StorageManager::StorageManager() : 
    flashUsed(0), 
    sdUsed(0), 
    flashTotal(0), 
    sdTotal(0),
    initialized(false) {
}

StorageManager& StorageManager::getInstance() {
    if (!instance) {
        instance = new StorageManager();
    }
    return *instance;
}

bool StorageManager::init() {
    Logger::info("StorageManager", "Initializing storage systems...");
    
    // Initialize SPIFFS (Flash storage)
    if (!SPIFFS.begin(true)) {
        Logger::error("StorageManager", "Failed to initialize SPIFFS");
        return false;
    }
    
    flashTotal = SPIFFS.totalBytes();
    flashUsed = SPIFFS.usedBytes();
    Logger::info("StorageManager", "SPIFFS initialized: %d/%d bytes used", flashUsed, flashTotal);
    
    // Initialize SD Card
    if (!SD.begin()) {
        Logger::warning("StorageManager", "SD card not available - running in flash-only mode");
        sdAvailable = false;
    } else {
        sdAvailable = true;
        sdTotal = SD.totalBytes();
        sdUsed = SD.usedBytes();
        Logger::info("StorageManager", "SD card initialized: %llu/%llu bytes used", sdUsed, sdTotal);
        
        // Create app directories on SD card
        createDirectory("/apps");
        createDirectory("/apps/installed");
        createDirectory("/apps/cache");
        createDirectory("/data");
        createDirectory("/ota");
    }
    
    // Load storage configuration
    loadStorageConfig();
    
    // Perform initial optimization
    optimizeStorage();
    
    initialized = true;
    Logger::info("StorageManager", "Storage manager initialized successfully");
    return true;
}

bool StorageManager::installApp(const String& appName, const uint8_t* appData, size_t appSize) {
    if (!initialized) {
        Logger::error("StorageManager", "Storage manager not initialized");
        return false;
    }
    
    Logger::info("StorageManager", "Installing app: %s (%d bytes)", appName.c_str(), appSize);
    
    // Determine storage location based on size and available space
    StorageLocation location = determineStorageLocation(appSize);
    
    String appPath;
    File appFile;
    
    if (location == STORAGE_FLASH) {
        appPath = "/apps/" + appName + ".bin";
        appFile = SPIFFS.open(appPath, "w");
    } else if (location == STORAGE_SD && sdAvailable) {
        appPath = "/apps/installed/" + appName + ".bin";
        appFile = SD.open(appPath, "w");
    } else {
        Logger::error("StorageManager", "No suitable storage location for app: %s", appName.c_str());
        return false;
    }
    
    if (!appFile) {
        Logger::error("StorageManager", "Failed to create app file: %s", appPath.c_str());
        return false;
    }
    
    // Write app data
    size_t written = appFile.write(appData, appSize);
    appFile.close();
    
    if (written != appSize) {
        Logger::error("StorageManager", "Failed to write complete app data");
        return false;
    }
    
    // Update app registry
    AppInfo appInfo;
    appInfo.name = appName;
    appInfo.size = appSize;
    appInfo.location = location;
    appInfo.path = appPath;
    appInfo.installed = true;
    appInfo.lastUsed = millis();
    
    installedApps[appName] = appInfo;
    saveStorageConfig();
    
    Logger::info("StorageManager", "App installed successfully: %s", appName.c_str());
    return true;
}

bool StorageManager::loadApp(const String& appName, uint8_t** appData, size_t* appSize) {
    if (!initialized) {
        Logger::error("StorageManager", "Storage manager not initialized");
        return false;
    }
    
    auto it = installedApps.find(appName);
    if (it == installedApps.end()) {
        Logger::error("StorageManager", "App not found: %s", appName.c_str());
        return false;
    }
    
    AppInfo& appInfo = it->second;
    Logger::info("StorageManager", "Loading app: %s from %s", appName.c_str(), 
                 appInfo.location == STORAGE_FLASH ? "flash" : "SD");
    
    File appFile;
    if (appInfo.location == STORAGE_FLASH) {
        appFile = SPIFFS.open(appInfo.path, "r");
    } else {
        appFile = SD.open(appInfo.path, "r");
    }
    
    if (!appFile) {
        Logger::error("StorageManager", "Failed to open app file: %s", appInfo.path.c_str());
        return false;
    }
    
    size_t fileSize = appFile.size();
    
    // Allocate memory for app data
    *appData = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!*appData) {
        // Try regular heap if PSRAM allocation fails
        *appData = (uint8_t*)malloc(fileSize);
        if (!*appData) {
            Logger::error("StorageManager", "Failed to allocate memory for app: %s", appName.c_str());
            appFile.close();
            return false;
        }
    }
    
    // Read app data
    size_t bytesRead = appFile.readBytes((char*)*appData, fileSize);
    appFile.close();
    
    if (bytesRead != fileSize) {
        Logger::error("StorageManager", "Failed to read complete app data");
        free(*appData);
        *appData = nullptr;
        return false;
    }
    
    *appSize = fileSize;
    appInfo.lastUsed = millis();
    
    Logger::info("StorageManager", "App loaded successfully: %s (%d bytes)", appName.c_str(), fileSize);
    return true;
}

bool StorageManager::unloadApp(const String& appName) {
    // This is handled by the app manager when it frees the app memory
    // We just update the last used time
    auto it = installedApps.find(appName);
    if (it != installedApps.end()) {
        it->second.lastUsed = millis();
        return true;
    }
    return false;
}

bool StorageManager::removeApp(const String& appName) {
    if (!initialized) {
        Logger::error("StorageManager", "Storage manager not initialized");
        return false;
    }
    
    auto it = installedApps.find(appName);
    if (it == installedApps.end()) {
        Logger::error("StorageManager", "App not found: %s", appName.c_str());
        return false;
    }
    
    AppInfo& appInfo = it->second;
    Logger::info("StorageManager", "Removing app: %s", appName.c_str());
    
    // Delete app file
    bool deleted = false;
    if (appInfo.location == STORAGE_FLASH) {
        deleted = SPIFFS.remove(appInfo.path);
    } else {
        deleted = SD.remove(appInfo.path);
    }
    
    if (!deleted) {
        Logger::warning("StorageManager", "Failed to delete app file: %s", appInfo.path.c_str());
    }
    
    // Remove from registry
    installedApps.erase(it);
    saveStorageConfig();
    
    Logger::info("StorageManager", "App removed: %s", appName.c_str());
    return true;
}

bool StorageManager::moveApp(const String& appName, StorageLocation newLocation) {
    if (!initialized) {
        Logger::error("StorageManager", "Storage manager not initialized");
        return false;
    }
    
    auto it = installedApps.find(appName);
    if (it == installedApps.end()) {
        Logger::error("StorageManager", "App not found: %s", appName.c_str());
        return false;
    }
    
    AppInfo& appInfo = it->second;
    if (appInfo.location == newLocation) {
        Logger::info("StorageManager", "App already in target location: %s", appName.c_str());
        return true;
    }
    
    Logger::info("StorageManager", "Moving app %s from %s to %s", appName.c_str(),
                 appInfo.location == STORAGE_FLASH ? "flash" : "SD",
                 newLocation == STORAGE_FLASH ? "flash" : "SD");
    
    // Load app data
    uint8_t* appData;
    size_t appSize;
    if (!loadApp(appName, &appData, &appSize)) {
        Logger::error("StorageManager", "Failed to load app for moving: %s", appName.c_str());
        return false;
    }
    
    // Remove from current location
    String oldPath = appInfo.path;
    StorageLocation oldLocation = appInfo.location;
    
    // Install in new location
    String newPath;
    File newFile;
    
    if (newLocation == STORAGE_FLASH) {
        newPath = "/apps/" + appName + ".bin";
        newFile = SPIFFS.open(newPath, "w");
    } else {
        newPath = "/apps/installed/" + appName + ".bin";
        newFile = SD.open(newPath, "w");
    }
    
    if (!newFile) {
        Logger::error("StorageManager", "Failed to create new app file: %s", newPath.c_str());
        free(appData);
        return false;
    }
    
    size_t written = newFile.write(appData, appSize);
    newFile.close();
    free(appData);
    
    if (written != appSize) {
        Logger::error("StorageManager", "Failed to write app to new location");
        return false;
    }
    
    // Update app info
    appInfo.location = newLocation;
    appInfo.path = newPath;
    
    // Remove old file
    if (oldLocation == STORAGE_FLASH) {
        SPIFFS.remove(oldPath);
    } else {
        SD.remove(oldPath);
    }
    
    saveStorageConfig();
    Logger::info("StorageManager", "App moved successfully: %s", appName.c_str());
    return true;
}

std::vector<String> StorageManager::getInstalledApps() {
    std::vector<String> apps;
    for (const auto& pair : installedApps) {
        apps.push_back(pair.first);
    }
    return apps;
}

StorageInfo StorageManager::getStorageInfo() {
    StorageInfo info;
    
    // Update current usage
    if (initialized) {
        flashUsed = SPIFFS.usedBytes();
        flashTotal = SPIFFS.totalBytes();
        
        if (sdAvailable) {
            sdUsed = SD.usedBytes();
            sdTotal = SD.totalBytes();
        }
    }
    
    info.flashUsed = flashUsed;
    info.flashTotal = flashTotal;
    info.flashAvailable = flashTotal - flashUsed;
    info.sdUsed = sdUsed;
    info.sdTotal = sdTotal;
    info.sdAvailable = sdTotal - sdUsed;
    info.sdCardPresent = sdAvailable;
    
    return info;
}

bool StorageManager::optimizeStorage() {
    if (!initialized) {
        return false;
    }
    
    Logger::info("StorageManager", "Optimizing storage...");
    
    // Get current storage usage
    StorageInfo info = getStorageInfo();
    
    // If flash is getting full (>80%), move least recently used apps to SD
    if (info.flashUsed > (info.flashTotal * 0.8) && sdAvailable) {
        Logger::info("StorageManager", "Flash storage is %d%% full, moving apps to SD card", 
                     (int)((info.flashUsed * 100) / info.flashTotal));
        
        // Find apps in flash, sorted by last used time
        std::vector<std::pair<String, unsigned long>> flashApps;
        for (const auto& pair : installedApps) {
            if (pair.second.location == STORAGE_FLASH) {
                flashApps.push_back({pair.first, pair.second.lastUsed});
            }
        }
        
        // Sort by last used time (oldest first)
        std::sort(flashApps.begin(), flashApps.end(), 
                  [](const auto& a, const auto& b) { return a.second < b.second; });
        
        // Move oldest apps to SD until flash usage is below 60%
        for (const auto& app : flashApps) {
            if (info.flashUsed < (info.flashTotal * 0.6)) {
                break;
            }
            
            if (moveApp(app.first, STORAGE_SD)) {
                info = getStorageInfo(); // Update info after move
            }
        }
    }
    
    // Clean up temporary files
    cleanupTempFiles();
    
    Logger::info("StorageManager", "Storage optimization complete");
    return true;
}

bool StorageManager::createDirectory(const String& path) {
    if (!sdAvailable) {
        return false;
    }
    
    // Check if directory already exists
    File dir = SD.open(path);
    if (dir && dir.isDirectory()) {
        dir.close();
        return true;
    }
    if (dir) {
        dir.close();
    }
    
    // Create directory
    return SD.mkdir(path);
}

StorageLocation StorageManager::determineStorageLocation(size_t appSize) {
    StorageInfo info = getStorageInfo();
    
    // If no SD card, must use flash
    if (!sdAvailable) {
        if (info.flashAvailable >= appSize) {
            return STORAGE_FLASH;
        } else {
            return STORAGE_NONE; // No space available
        }
    }
    
    // If app is small and flash has space, prefer flash for speed
    if (appSize < (1024 * 1024) && info.flashAvailable >= appSize && 
        info.flashUsed < (info.flashTotal * 0.7)) {
        return STORAGE_FLASH;
    }
    
    // Otherwise use SD card
    if (info.sdAvailable >= appSize) {
        return STORAGE_SD;
    }
    
    // If SD is full but flash has space, use flash
    if (info.flashAvailable >= appSize) {
        return STORAGE_FLASH;
    }
    
    return STORAGE_NONE; // No space available
}

void StorageManager::loadStorageConfig() {
    File configFile = SPIFFS.open("/storage_config.json", "r");
    if (!configFile) {
        Logger::info("StorageManager", "No storage config found, creating new one");
        return;
    }
    
    String configData = configFile.readString();
    configFile.close();
    
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, configData);
    
    if (error) {
        Logger::error("StorageManager", "Failed to parse storage config: %s", error.c_str());
        return;
    }
    
    // Load installed apps
    JsonArray apps = doc["apps"];
    for (JsonObject app : apps) {
        AppInfo appInfo;
        appInfo.name = app["name"].as<String>();
        appInfo.size = app["size"];
        appInfo.location = (StorageLocation)app["location"].as<int>();
        appInfo.path = app["path"].as<String>();
        appInfo.installed = app["installed"];
        appInfo.lastUsed = app["lastUsed"];
        
        installedApps[appInfo.name] = appInfo;
    }
    
    Logger::info("StorageManager", "Loaded storage config with %d apps", installedApps.size());
}

void StorageManager::saveStorageConfig() {
    DynamicJsonDocument doc(4096);
    
    JsonArray apps = doc.createNestedArray("apps");
    for (const auto& pair : installedApps) {
        const AppInfo& appInfo = pair.second;
        JsonObject app = apps.createNestedObject();
        app["name"] = appInfo.name;
        app["size"] = appInfo.size;
        app["location"] = (int)appInfo.location;
        app["path"] = appInfo.path;
        app["installed"] = appInfo.installed;
        app["lastUsed"] = appInfo.lastUsed;
    }
    
    File configFile = SPIFFS.open("/storage_config.json", "w");
    if (!configFile) {
        Logger::error("StorageManager", "Failed to open storage config for writing");
        return;
    }
    
    serializeJson(doc, configFile);
    configFile.close();
    
    Logger::debug("StorageManager", "Storage config saved");
}

void StorageManager::cleanupTempFiles() {
    // Clean up temporary files in flash
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        if (fileName.endsWith(".tmp") || fileName.startsWith("temp_")) {
            String filePath = "/" + fileName;
            file.close();
            SPIFFS.remove(filePath);
            Logger::debug("StorageManager", "Removed temp file: %s", filePath.c_str());
            file = root.openNextFile();
        } else {
            file.close();
            file = root.openNextFile();
        }
    }
    root.close();
    
    // Clean up temporary files on SD card
    if (sdAvailable) {
        File sdRoot = SD.open("/");
        File sdFile = sdRoot.openNextFile();
        while (sdFile) {
            String fileName = sdFile.name();
            if (fileName.endsWith(".tmp") || fileName.startsWith("temp_")) {
                String filePath = "/" + fileName;
                sdFile.close();
                SD.remove(filePath);
                Logger::debug("StorageManager", "Removed SD temp file: %s", filePath.c_str());
                sdFile = sdRoot.openNextFile();
            } else {
                sdFile.close();
                sdFile = sdRoot.openNextFile();
            }
        }
        sdRoot.close();
    }
}