// storage_manager.h
// T-Deck-Pro OS Storage Management System
#pragma once

#include <Arduino.h>
#include <SPIFFS.h>
#include <SD.h>
#include <FS.h>
#include <vector>
#include "core/utils/logger.h"

// Storage types
typedef enum {
    STORAGE_FLASH,      // Internal flash (SPIFFS)
    STORAGE_SD_CARD,    // External SD card
    STORAGE_AUTO        // Automatically choose best storage
} storage_type_t;

// Storage priority for different content types
typedef enum {
    STORAGE_PRIORITY_FLASH,     // Prefer flash storage
    STORAGE_PRIORITY_SD,        // Prefer SD card storage
    STORAGE_PRIORITY_BALANCED   // Balance between flash and SD
} storage_priority_t;

// File metadata
struct FileInfo {
    String path;
    size_t size;
    storage_type_t location;
    bool isDirectory;
    time_t lastModified;
    bool canMove;  // Can be moved between storage types
};

// Storage statistics
struct StorageStats {
    // Flash storage
    size_t flashTotal;
    size_t flashUsed;
    size_t flashFree;
    
    // SD card storage
    size_t sdTotal;
    size_t sdUsed;
    size_t sdFree;
    
    // File counts
    uint32_t flashFiles;
    uint32_t sdFiles;
    uint32_t totalFiles;
};

class StorageManager {
private:
    static StorageManager* instance;
    
    bool flashInitialized;
    bool sdInitialized;
    storage_priority_t defaultPriority;
    
    // Storage thresholds
    float flashWarningThreshold;  // Warn when flash usage exceeds this %
    float flashCriticalThreshold; // Critical when flash usage exceeds this %
    
    // Internal methods
    bool initializeFlash();
    bool initializeSD();
    storage_type_t selectBestStorage(size_t fileSize, storage_priority_t priority);
    bool moveFileInternal(const String& srcPath, storage_type_t srcType, 
                         const String& dstPath, storage_type_t dstType);
    
public:
    StorageManager();
    ~StorageManager();
    
    // Singleton access
    static StorageManager& getInstance();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Storage availability
    bool isFlashAvailable() const;
    bool isSDAvailable() const;
    
    // File operations
    bool writeFile(const String& path, const uint8_t* data, size_t size, 
                   storage_priority_t priority = STORAGE_PRIORITY_BALANCED);
    bool readFile(const String& path, uint8_t* buffer, size_t bufferSize, size_t* bytesRead);
    bool deleteFile(const String& path);
    bool fileExists(const String& path);
    bool createDirectory(const String& path, storage_type_t storage = STORAGE_AUTO);
    
    // File management
    bool moveFile(const String& srcPath, const String& dstPath, storage_type_t dstStorage = STORAGE_AUTO);
    bool copyFile(const String& srcPath, const String& dstPath, storage_type_t dstStorage = STORAGE_AUTO);
    FileInfo getFileInfo(const String& path);
    std::vector<FileInfo> listFiles(const String& directory, bool recursive = false);
    
    // Storage optimization
    bool optimizeStorage();
    bool moveToSD(const String& path);
    bool moveToFlash(const String& path);
    bool autoBalance();
    
    // App-specific operations
    bool installApp(const String& appPath, const uint8_t* appData, size_t appSize);
    bool uninstallApp(const String& appId);
    bool loadAppFromStorage(const String& appId, uint8_t* buffer, size_t bufferSize, size_t* appSize);
    std::vector<String> getInstalledApps();
    
    // Configuration
    void setStoragePriority(storage_priority_t priority);
    void setFlashThresholds(float warning, float critical);
    
    // Statistics and monitoring
    StorageStats getStorageStats();
    float getFlashUsagePercent();
    float getSDUsagePercent();
    bool isFlashCritical();
    bool isFlashWarning();
    
    // Maintenance
    bool cleanupTempFiles();
    bool defragmentStorage();
    bool verifyStorageIntegrity();
    
    // File system access
    fs::FS& getFlashFS();
    fs::FS& getSDFS();
    fs::FS& getStorageFS(storage_type_t storage);
};

// Convenience macros
#define STORAGE_MGR StorageManager::getInstance()

// Add these constants after the existing includes

// Storage paths
#define STORAGE_PATH_APPS     "/apps"
#define STORAGE_PATH_DATA     "/data"
#define STORAGE_PATH_CONFIG   "/config"
#define STORAGE_PATH_LOGS     "/logs"
#define STORAGE_PATH_TEMP     "/temp"
#define STORAGE_PATH_CACHE    "/cache"

// App storage structure
#define APP_MANIFEST_FILE "manifest.json"
#define APP_BINARY_FILE "app.bin"
#define APP_RESOURCES_DIR "resources"
#define APP_DATA_DIR "data"