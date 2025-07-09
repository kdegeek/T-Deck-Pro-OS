#include "storage_manager.h"
#include "../utils/logger.h"
#include <SPIFFS.h>
#include <SD.h>
#include <ArduinoJson.h>

StorageManager* StorageManager::instance = nullptr;

StorageManager::StorageManager() : 
    flashInitialized(false),
    sdInitialized(false),
    defaultPriority(STORAGE_PRIORITY_BALANCED),
    flashWarningThreshold(0.8),
    flashCriticalThreshold(0.9) {
}

StorageManager::~StorageManager() {
    shutdown();
}

StorageManager& StorageManager::getInstance() {
    if (!instance) {
        instance = new StorageManager();
    }
    return *instance;
}

bool StorageManager::initialize() {
    Logger::info("StorageManager", "Initializing storage systems...");
    
    // Initialize SPIFFS (Flash storage)
    if (!SPIFFS.begin(true)) {
        Logger::error("StorageManager", "Failed to initialize SPIFFS");
        flashInitialized = false;
    } else {
        flashInitialized = true;
        Logger::info("StorageManager", "SPIFFS initialized: %d/%d bytes used", 
                     SPIFFS.usedBytes(), SPIFFS.totalBytes());
    }
    
    // Initialize SD Card
    if (!SD.begin()) {
        Logger::warning("StorageManager", "SD card not available - running in flash-only mode");
        sdInitialized = false;
    } else {
        sdInitialized = true;
        Logger::info("StorageManager", "SD card initialized: %llu/%llu bytes used", 
                     SD.usedBytes(), SD.totalBytes());
        
        // Create app directories on SD card
        createDirectory("/apps", STORAGE_SD_CARD);
        createDirectory("/data", STORAGE_SD_CARD);
        createDirectory("/ota", STORAGE_SD_CARD);
    }
    
    Logger::info("StorageManager", "Storage manager initialized successfully");
    return flashInitialized || sdInitialized;
}

void StorageManager::shutdown() {
    if (flashInitialized) {
        SPIFFS.end();
        flashInitialized = false;
    }
    if (sdInitialized) {
        SD.end();
        sdInitialized = false;
    }
    Logger::info("StorageManager", "Storage manager shutdown");
}

bool StorageManager::isFlashAvailable() const {
    return flashInitialized;
}

bool StorageManager::isSDAvailable() const {
    return sdInitialized;
}

bool StorageManager::initializeFlash() {
    if (!flashInitialized) {
        flashInitialized = SPIFFS.begin(true);
    }
    return flashInitialized;
}

bool StorageManager::initializeSD() {
    if (!sdInitialized) {
        sdInitialized = SD.begin();
    }
    return sdInitialized;
}

storage_type_t StorageManager::selectBestStorage(size_t fileSize, storage_priority_t priority) {
    StorageStats stats = getStorageStats();
    
    switch (priority) {
        case STORAGE_PRIORITY_FLASH:
            if (flashInitialized && stats.flashFree >= fileSize) {
                return STORAGE_FLASH;
            }
            if (sdInitialized && stats.sdFree >= fileSize) {
                return STORAGE_SD_CARD;
            }
            break;
            
        case STORAGE_PRIORITY_SD:
            if (sdInitialized && stats.sdFree >= fileSize) {
                return STORAGE_SD_CARD;
            }
            if (flashInitialized && stats.flashFree >= fileSize) {
                return STORAGE_FLASH;
            }
            break;
            
        case STORAGE_PRIORITY_BALANCED:
        default:
            // For small files, prefer flash if available and not critical
            if (fileSize < (1024 * 1024) && flashInitialized && 
                stats.flashFree >= fileSize && !isFlashCritical()) {
                return STORAGE_FLASH;
            }
            // Otherwise prefer SD card
            if (sdInitialized && stats.sdFree >= fileSize) {
                return STORAGE_SD_CARD;
            }
            if (flashInitialized && stats.flashFree >= fileSize) {
                return STORAGE_FLASH;
            }
            break;
    }
    
    return STORAGE_AUTO; // No suitable storage found
}

bool StorageManager::writeFile(const String& path, const uint8_t* data, size_t size, 
                              storage_priority_t priority) {
    storage_type_t storage = selectBestStorage(size, priority);
    
    if (storage == STORAGE_AUTO) {
        Logger::error("StorageManager", "No suitable storage for file: %s", path.c_str());
        return false;
    }
    
    fs::FS& fs = getStorageFS(storage);
    File file = fs.open(path, "w");
    if (!file) {
        Logger::error("StorageManager", "Failed to create file: %s", path.c_str());
        return false;
    }
    
    size_t written = file.write(data, size);
    file.close();
    
    if (written != size) {
        Logger::error("StorageManager", "Failed to write complete file data");
        return false;
    }
    
    Logger::info("StorageManager", "File written successfully: %s (%d bytes)", path.c_str(), size);
    return true;
}

bool StorageManager::readFile(const String& path, uint8_t* buffer, size_t bufferSize, size_t* bytesRead) {
    // Try flash first
    if (flashInitialized) {
        File file = SPIFFS.open(path, "r");
        if (file) {
            size_t fileSize = file.size();
            size_t toRead = (fileSize < bufferSize) ? fileSize : bufferSize;
            *bytesRead = file.readBytes((char*)buffer, toRead);
            file.close();
            return (*bytesRead == toRead);
        }
    }
    
    // Try SD card
    if (sdInitialized) {
        File file = SD.open(path, "r");
        if (file) {
            size_t fileSize = file.size();
            size_t toRead = (fileSize < bufferSize) ? fileSize : bufferSize;
            *bytesRead = file.readBytes((char*)buffer, toRead);
            file.close();
            return (*bytesRead == toRead);
        }
    }
    
    Logger::error("StorageManager", "File not found: %s", path.c_str());
    return false;
}

bool StorageManager::deleteFile(const String& path) {
    bool deleted = false;
    
    // Try flash first
    if (flashInitialized && SPIFFS.exists(path)) {
        deleted = SPIFFS.remove(path);
    }
    
    // Try SD card
    if (!deleted && sdInitialized && SD.exists(path)) {
        deleted = SD.remove(path);
    }
    
    if (deleted) {
        Logger::info("StorageManager", "File deleted: %s", path.c_str());
    } else {
        Logger::error("StorageManager", "Failed to delete file: %s", path.c_str());
    }
    
    return deleted;
}

bool StorageManager::fileExists(const String& path) {
    if (flashInitialized && SPIFFS.exists(path)) {
        return true;
    }
    if (sdInitialized && SD.exists(path)) {
        return true;
    }
    return false;
}

bool StorageManager::createDirectory(const String& path, storage_type_t storage) {
    if (storage == STORAGE_FLASH || storage == STORAGE_AUTO) {
        // SPIFFS doesn't support directories
        return true;
    }
    
    if ((storage == STORAGE_SD_CARD || storage == STORAGE_AUTO) && sdInitialized) {
        return SD.mkdir(path);
    }
    
    return false;
}

bool StorageManager::moveFileInternal(const String& srcPath, storage_type_t srcType, 
                                     const String& dstPath, storage_type_t dstType) {
    // Read from source
    fs::FS& srcFS = getStorageFS(srcType);
    File srcFile = srcFS.open(srcPath, "r");
    if (!srcFile) {
        return false;
    }
    
    size_t fileSize = srcFile.size();
    uint8_t* buffer = (uint8_t*)malloc(fileSize);
    if (!buffer) {
        srcFile.close();
        return false;
    }
    
    size_t bytesRead = srcFile.readBytes((char*)buffer, fileSize);
    srcFile.close();
    
    if (bytesRead != fileSize) {
        free(buffer);
        return false;
    }
    
    // Write to destination
    fs::FS& dstFS = getStorageFS(dstType);
    File dstFile = dstFS.open(dstPath, "w");
    if (!dstFile) {
        free(buffer);
        return false;
    }
    
    size_t bytesWritten = dstFile.write(buffer, fileSize);
    dstFile.close();
    free(buffer);
    
    if (bytesWritten != fileSize) {
        return false;
    }
    
    // Remove source file
    srcFS.remove(srcPath);
    return true;
}

bool StorageManager::moveFile(const String& srcPath, const String& dstPath, storage_type_t dstStorage) {
    // Determine source storage
    storage_type_t srcStorage = STORAGE_AUTO;
    if (flashInitialized && SPIFFS.exists(srcPath)) {
        srcStorage = STORAGE_FLASH;
    } else if (sdInitialized && SD.exists(srcPath)) {
        srcStorage = STORAGE_SD_CARD;
    } else {
        return false;
    }
    
    if (dstStorage == STORAGE_AUTO) {
        dstStorage = selectBestStorage(0, defaultPriority);
    }
    
    return moveFileInternal(srcPath, srcStorage, dstPath, dstStorage);
}

bool StorageManager::copyFile(const String& srcPath, const String& dstPath, storage_type_t dstStorage) {
    // Similar to moveFile but don't delete source
    storage_type_t srcStorage = STORAGE_AUTO;
    if (flashInitialized && SPIFFS.exists(srcPath)) {
        srcStorage = STORAGE_FLASH;
    } else if (sdInitialized && SD.exists(srcPath)) {
        srcStorage = STORAGE_SD_CARD;
    } else {
        return false;
    }
    
    if (dstStorage == STORAGE_AUTO) {
        dstStorage = selectBestStorage(0, defaultPriority);
    }
    
    // Read from source
    fs::FS& srcFS = getStorageFS(srcStorage);
    File srcFile = srcFS.open(srcPath, "r");
    if (!srcFile) {
        return false;
    }
    
    size_t fileSize = srcFile.size();
    uint8_t* buffer = (uint8_t*)malloc(fileSize);
    if (!buffer) {
        srcFile.close();
        return false;
    }
    
    size_t bytesRead = srcFile.readBytes((char*)buffer, fileSize);
    srcFile.close();
    
    if (bytesRead != fileSize) {
        free(buffer);
        return false;
    }
    
    // Write to destination
    fs::FS& dstFS = getStorageFS(dstStorage);
    File dstFile = dstFS.open(dstPath, "w");
    if (!dstFile) {
        free(buffer);
        return false;
    }
    
    size_t bytesWritten = dstFile.write(buffer, fileSize);
    dstFile.close();
    free(buffer);
    
    return (bytesWritten == fileSize);
}

FileInfo StorageManager::getFileInfo(const String& path) {
    FileInfo info;
    info.path = path;
    info.size = 0;
    info.location = STORAGE_AUTO;
    info.isDirectory = false;
    info.lastModified = 0;
    info.canMove = true;
    
    // Check flash first
    if (flashInitialized && SPIFFS.exists(path)) {
        File file = SPIFFS.open(path);
        if (file) {
            info.size = file.size();
            info.location = STORAGE_FLASH;
            info.isDirectory = file.isDirectory();
            info.lastModified = file.getLastWrite();
            file.close();
        }
    }
    // Check SD card
    else if (sdInitialized && SD.exists(path)) {
        File file = SD.open(path);
        if (file) {
            info.size = file.size();
            info.location = STORAGE_SD_CARD;
            info.isDirectory = file.isDirectory();
            info.lastModified = file.getLastWrite();
            file.close();
        }
    }
    
    return info;
}

std::vector<FileInfo> StorageManager::listFiles(const String& directory, bool recursive) {
    std::vector<FileInfo> files;
    
    // List files from flash
    if (flashInitialized) {
        File root = SPIFFS.open(directory);
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                FileInfo info;
                info.path = file.path();
                info.size = file.size();
                info.location = STORAGE_FLASH;
                info.isDirectory = file.isDirectory();
                info.lastModified = file.getLastWrite();
                info.canMove = true;
                files.push_back(info);
                
                file.close();
                file = root.openNextFile();
            }
            root.close();
        }
    }
    
    // List files from SD card
    if (sdInitialized) {
        File root = SD.open(directory);
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                FileInfo info;
                info.path = file.path();
                info.size = file.size();
                info.location = STORAGE_SD_CARD;
                info.isDirectory = file.isDirectory();
                info.lastModified = file.getLastWrite();
                info.canMove = true;
                files.push_back(info);
                
                file.close();
                file = root.openNextFile();
            }
            root.close();
        }
    }
    
    return files;
}

bool StorageManager::optimizeStorage() {
    Logger::info("StorageManager", "Optimizing storage...");
    
    // Clean up temporary files
    cleanupTempFiles();
    
    // Auto balance if flash is getting full
    if (isFlashWarning()) {
        autoBalance();
    }
    
    Logger::info("StorageManager", "Storage optimization complete");
    return true;
}

bool StorageManager::moveToSD(const String& path) {
    if (!sdInitialized) {
        return false;
    }
    
    if (flashInitialized && SPIFFS.exists(path)) {
        return moveFileInternal(path, STORAGE_FLASH, path, STORAGE_SD_CARD);
    }
    
    return false;
}

bool StorageManager::moveToFlash(const String& path) {
    if (!flashInitialized) {
        return false;
    }
    
    if (sdInitialized && SD.exists(path)) {
        return moveFileInternal(path, STORAGE_SD_CARD, path, STORAGE_FLASH);
    }
    
    return false;
}

bool StorageManager::autoBalance() {
    if (!sdInitialized || !flashInitialized) {
        return false;
    }
    
    Logger::info("StorageManager", "Auto-balancing storage...");
    
    // Move files from flash to SD if flash is getting full
    if (isFlashCritical()) {
        std::vector<FileInfo> flashFiles = listFiles("/");
        
        // Sort by size (largest first) to free up space quickly
        std::sort(flashFiles.begin(), flashFiles.end(), 
                  [](const FileInfo& a, const FileInfo& b) { return a.size > b.size; });
        
        for (const FileInfo& file : flashFiles) {
            if (file.location == STORAGE_FLASH && file.canMove && !file.isDirectory) {
                if (moveToSD(file.path)) {
                    Logger::info("StorageManager", "Moved file to SD: %s", file.path.c_str());
                    
                    // Check if we've freed enough space
                    if (!isFlashCritical()) {
                        break;
                    }
                }
            }
        }
    }
    
    return true;
}

bool StorageManager::installApp(const String& appPath, const uint8_t* appData, size_t appSize) {
    return writeFile(appPath, appData, appSize, STORAGE_PRIORITY_BALANCED);
}

bool StorageManager::uninstallApp(const String& appId) {
    String appPath = STORAGE_PATH_APPS + String("/") + appId + ".bin";
    return deleteFile(appPath);
}

bool StorageManager::loadAppFromStorage(const String& appId, uint8_t* buffer, size_t bufferSize, size_t* appSize) {
    String appPath = STORAGE_PATH_APPS + String("/") + appId + ".bin";
    return readFile(appPath, buffer, bufferSize, appSize);
}

std::vector<String> StorageManager::getInstalledApps() {
    std::vector<String> apps;
    std::vector<FileInfo> files = listFiles(STORAGE_PATH_APPS);
    
    for (const FileInfo& file : files) {
        if (!file.isDirectory && file.path.endsWith(".bin")) {
            String appName = file.path;
            appName.replace(STORAGE_PATH_APPS + String("/"), "");
            appName.replace(".bin", "");
            apps.push_back(appName);
        }
    }
    
    return apps;
}

void StorageManager::setStoragePriority(storage_priority_t priority) {
    defaultPriority = priority;
}

void StorageManager::setFlashThresholds(float warning, float critical) {
    flashWarningThreshold = warning;
    flashCriticalThreshold = critical;
}

StorageStats StorageManager::getStorageStats() {
    StorageStats stats;
    
    if (flashInitialized) {
        stats.flashTotal = SPIFFS.totalBytes();
        stats.flashUsed = SPIFFS.usedBytes();
        stats.flashFree = stats.flashTotal - stats.flashUsed;
    } else {
        stats.flashTotal = 0;
        stats.flashUsed = 0;
        stats.flashFree = 0;
    }
    
    if (sdInitialized) {
        stats.sdTotal = SD.totalBytes();
        stats.sdUsed = SD.usedBytes();
        stats.sdFree = stats.sdTotal - stats.sdUsed;
    } else {
        stats.sdTotal = 0;
        stats.sdUsed = 0;
        stats.sdFree = 0;
    }
    
    // Count files (simplified)
    stats.flashFiles = 0;
    stats.sdFiles = 0;
    stats.totalFiles = stats.flashFiles + stats.sdFiles;
    
    return stats;
}

float StorageManager::getFlashUsagePercent() {
    if (!flashInitialized) {
        return 0.0;
    }
    
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    
    if (total == 0) {
        return 0.0;
    }
    
    return (float(used) / float(total)) * 100.0;
}

float StorageManager::getSDUsagePercent() {
    if (!sdInitialized) {
        return 0.0;
    }
    
    size_t total = SD.totalBytes();
    size_t used = SD.usedBytes();
    
    if (total == 0) {
        return 0.0;
    }
    
    return (float(used) / float(total)) * 100.0;
}

bool StorageManager::isFlashCritical() {
    return getFlashUsagePercent() > (flashCriticalThreshold * 100.0);
}

bool StorageManager::isFlashWarning() {
    return getFlashUsagePercent() > (flashWarningThreshold * 100.0);
}

bool StorageManager::cleanupTempFiles() {
    bool cleaned = false;
    
    // Clean flash temp files
    if (flashInitialized) {
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while (file) {
            String fileName = file.name();
            if (fileName.endsWith(".tmp") || fileName.startsWith("temp_")) {
                file.close();
                if (SPIFFS.remove("/" + fileName)) {
                    cleaned = true;
                    Logger::debug("StorageManager", "Removed temp file: %s", fileName.c_str());
                }
                file = root.openNextFile();
            } else {
                file.close();
                file = root.openNextFile();
            }
        }
        root.close();
    }
    
    // Clean SD temp files
    if (sdInitialized) {
        File root = SD.open("/");
        File file = root.openNextFile();
        while (file) {
            String fileName = file.name();
            if (fileName.endsWith(".tmp") || fileName.startsWith("temp_")) {
                file.close();
                if (SD.remove("/" + fileName)) {
                    cleaned = true;
                    Logger::debug("StorageManager", "Removed SD temp file: %s", fileName.c_str());
                }
                file = root.openNextFile();
            } else {
                file.close();
                file = root.openNextFile();
            }
        }
        root.close();
    }
    
    return cleaned;
}

bool StorageManager::defragmentStorage() {
    // SPIFFS and SD don't need traditional defragmentation
    Logger::info("StorageManager", "Storage defragmentation not needed for SPIFFS/SD");
    return true;
}

bool StorageManager::verifyStorageIntegrity() {
    bool integrity = true;
    
    if (flashInitialized) {
        // Basic SPIFFS check
        if (!SPIFFS.begin()) {
            Logger::error("StorageManager", "SPIFFS integrity check failed");
            integrity = false;
        }
    }
    
    // SD card integrity is harder to check without specific tools
    Logger::info("StorageManager", "Storage integrity check %s", integrity ? "passed" : "failed");
    return integrity;
}

fs::FS& StorageManager::getFlashFS() {
    return SPIFFS;
}

fs::FS& StorageManager::getSDFS() {
    return SD;
}

fs::FS& StorageManager::getStorageFS(storage_type_t storage) {
    switch (storage) {
        case STORAGE_FLASH:
            return SPIFFS;
        case STORAGE_SD_CARD:
        default:
            return SD;
    }
}