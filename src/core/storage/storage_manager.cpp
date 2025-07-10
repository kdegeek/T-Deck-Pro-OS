/**
 * @file storage_manager.cpp
 * @brief T-Deck-Pro Storage Manager - SD Card and SPIFFS Management Implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Handles SD card and SPIFFS file systems with error recovery
 */

#include "storage_manager.h"
#include "../utils/logger.h"
#include <ArduinoJson.h>

// ===== GLOBAL STORAGE MANAGER INSTANCE =====
StorageManager* g_storage_manager = nullptr;

// ===== STORAGE MANAGER IMPLEMENTATION =====

StorageManager::StorageManager() : initialized_(false), current_state_(StorageState::INIT),
                                  sd_card_mounted_(false), spiffs_mounted_(false),
                                  error_count_(0), last_check_time_(0), last_stats_update_(0) {
    
    // Initialize file system pointers
    sd_fs_ = &SD;
    spiffs_fs_ = &SPIFFS;
    
    // Initialize statistics
    memset(&sd_stats_, 0, sizeof(StorageStats));
    memset(&spiffs_stats_, 0, sizeof(StorageStats));
}

StorageManager::~StorageManager() {
    if (initialized_) {
        unmountSDCard();
        unmountSPIFFS();
    }
}

bool StorageManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    logInfo("STORAGE", "Initializing storage manager");
    
    // Initialize hardware
    if (!initHardware()) {
        logError("STORAGE", "Failed to initialize storage hardware");
        return false;
    }
    
    // Initialize file systems
    if (!initFileSystems()) {
        logError("STORAGE", "Failed to initialize file systems");
        return false;
    }
    
    // Configure storage settings
    if (!configureStorage()) {
        logError("STORAGE", "Failed to configure storage");
        return false;
    }
    
    initialized_ = true;
    setState(StorageState::READY);
    
    logInfo("STORAGE", "Storage manager initialized successfully");
    return true;
}

bool StorageManager::initHardware() {
    // Initialize SPI for SD card
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
    
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        logError("STORAGE", "Failed to initialize SPIFFS");
        return false;
    }
    
    return true;
}

bool StorageManager::initFileSystems() {
    // SPIFFS is already initialized in initHardware()
    spiffs_mounted_ = true;
    
    // SD card will be mounted when needed
    sd_card_mounted_ = false;
    
    return true;
}

bool StorageManager::configureStorage() {
    // Set up default directories
    if (spiffs_mounted_) {
        // Create default directories in SPIFFS
        if (!SPIFFS.exists("/config")) {
            SPIFFS.mkdir("/config");
        }
        if (!SPIFFS.exists("/cache")) {
            SPIFFS.mkdir("/cache");
        }
    }
    
    return true;
}

bool StorageManager::mountSDCard() {
    if (sd_card_mounted_) {
        return true;
    }
    
    logInfo("STORAGE", "Mounting SD card");
    
    if (!SD.begin(BOARD_SD_CS, *sd_fs_)) {
        logError("STORAGE", "Failed to mount SD card");
        return false;
    }
    
    sd_card_mounted_ = true;
    logInfo("STORAGE", "SD card mounted successfully");
    
    // Update statistics
    updateStats(StorageType::SD_CARD);
    
    return true;
}

bool StorageManager::mountSPIFFS() {
    if (spiffs_mounted_) {
        return true;
    }
    
    logInfo("STORAGE", "Mounting SPIFFS");
    
    if (!SPIFFS.begin(true)) {
        logError("STORAGE", "Failed to mount SPIFFS");
        return false;
    }
    
    spiffs_mounted_ = true;
    logInfo("STORAGE", "SPIFFS mounted successfully");
    
    // Update statistics
    updateStats(StorageType::SPIFFS);
    
    return true;
}

bool StorageManager::unmountSDCard() {
    if (!sd_card_mounted_) {
        return true;
    }
    
    logInfo("STORAGE", "Unmounting SD card");
    SD.end();
    sd_card_mounted_ = false;
    
    return true;
}

bool StorageManager::unmountSPIFFS() {
    if (!spiffs_mounted_) {
        return true;
    }
    
    logInfo("STORAGE", "Unmounting SPIFFS");
    SPIFFS.end();
    spiffs_mounted_ = false;
    
    return true;
}

std::vector<FileInfo> StorageManager::listFiles(const String& path, StorageType type) {
    std::vector<FileInfo> files;
    
    fs::FS* fs = getFileSystem(type);
    if (!fs) {
        logError("STORAGE", "Invalid storage type for listing files");
        return files;
    }
    
    File root = fs->open(path);
    if (!root) {
        logError("STORAGE", "Failed to open directory: " + path);
        return files;
    }
    
    if (!root.isDirectory()) {
        logError("STORAGE", "Path is not a directory: " + path);
        root.close();
        return files;
    }
    
    File file = root.openNextFile();
    while (file) {
        FileInfo info;
        info.name = file.name();
        info.path = path + "/" + file.name();
        info.size = file.size();
        info.is_directory = file.isDirectory();
        info.last_modified = file.getLastWrite();
        info.extension = getFileExtension(file.name());
        
        files.push_back(info);
        file = root.openNextFile();
    }
    
    root.close();
    return files;
}

bool StorageManager::createDirectory(const String& path, StorageType type) {
    fs::FS* fs = getFileSystem(type);
    if (!fs) {
        logError("STORAGE", "Invalid storage type for creating directory");
        return false;
    }
    
    if (fs->mkdir(path)) {
        logInfo("STORAGE", "Created directory: " + path);
        return true;
    } else {
        logError("STORAGE", "Failed to create directory: " + path);
        return false;
    }
}

bool StorageManager::deleteFile(const String& path, StorageType type) {
    fs::FS* fs = getFileSystem(type);
    if (!fs) {
        logError("STORAGE", "Invalid storage type for deleting file");
        return false;
    }
    
    if (fs->remove(path)) {
        logInfo("STORAGE", "Deleted file: " + path);
        return true;
    } else {
        logError("STORAGE", "Failed to delete file: " + path);
        return false;
    }
}

String StorageManager::readFile(const String& path, StorageType type) {
    fs::FS* fs = getFileSystem(type);
    if (!fs) {
        logError("STORAGE", "Invalid storage type for reading file");
        return "";
    }
    
    File file = fs->open(path, FILE_READ);
    if (!file) {
        logError("STORAGE", "Failed to open file for reading: " + path);
        return "";
    }
    
    String content = file.readString();
    file.close();
    
    logDebug("STORAGE", "Read file: " + path + " (" + String(content.length()) + " bytes)");
    return content;
}

bool StorageManager::writeFile(const String& path, const String& content, StorageType type) {
    fs::FS* fs = getFileSystem(type);
    if (!fs) {
        logError("STORAGE", "Invalid storage type for writing file");
        return false;
    }
    
    File file = fs->open(path, FILE_WRITE);
    if (!file) {
        logError("STORAGE", "Failed to open file for writing: " + path);
        return false;
    }
    
    size_t bytes_written = file.print(content);
    file.close();
    
    if (bytes_written == content.length()) {
        logInfo("STORAGE", "Wrote file: " + path + " (" + String(bytes_written) + " bytes)");
        return true;
    } else {
        logError("STORAGE", "Failed to write file: " + path);
        return false;
    }
}

bool StorageManager::appendFile(const String& path, const String& content, StorageType type) {
    fs::FS* fs = getFileSystem(type);
    if (!fs) {
        logError("STORAGE", "Invalid storage type for appending file");
        return false;
    }
    
    File file = fs->open(path, FILE_APPEND);
    if (!file) {
        logError("STORAGE", "Failed to open file for appending: " + path);
        return false;
    }
    
    size_t bytes_written = file.print(content);
    file.close();
    
    if (bytes_written == content.length()) {
        logInfo("STORAGE", "Appended to file: " + path + " (" + String(bytes_written) + " bytes)");
        return true;
    } else {
        logError("STORAGE", "Failed to append to file: " + path);
        return false;
    }
}

bool StorageManager::fileExists(const String& path, StorageType type) {
    fs::FS* fs = getFileSystem(type);
    if (!fs) {
        return false;
    }
    
    return fs->exists(path);
}

size_t StorageManager::getFileSize(const String& path, StorageType type) {
    fs::FS* fs = getFileSystem(type);
    if (!fs) {
        return 0;
    }
    
    File file = fs->open(path);
    if (!file) {
        return 0;
    }
    
    size_t size = file.size();
    file.close();
    return size;
}

bool StorageManager::copyFile(const String& source, const String& destination,
                             StorageType source_type, StorageType dest_type) {
    String content = readFile(source, source_type);
    if (content.length() == 0) {
        return false;
    }
    
    return writeFile(destination, content, dest_type);
}

bool StorageManager::moveFile(const String& source, const String& destination,
                             StorageType source_type, StorageType dest_type) {
    if (!copyFile(source, destination, source_type, dest_type)) {
        return false;
    }
    
    return deleteFile(source, source_type);
}

StorageStats StorageManager::getStats(StorageType type) {
    if (type == StorageType::SD_CARD) {
        return sd_stats_;
    } else if (type == StorageType::SPIFFS) {
        return spiffs_stats_;
    } else {
        // Return combined stats
        StorageStats combined;
        combined.total_space = sd_stats_.total_space + spiffs_stats_.total_space;
        combined.used_space = sd_stats_.used_space + spiffs_stats_.used_space;
        combined.free_space = sd_stats_.free_space + spiffs_stats_.free_space;
        combined.file_count = sd_stats_.file_count + spiffs_stats_.file_count;
        combined.directory_count = sd_stats_.directory_count + spiffs_stats_.directory_count;
        combined.read_operations = sd_stats_.read_operations + spiffs_stats_.read_operations;
        combined.write_operations = sd_stats_.write_operations + spiffs_stats_.write_operations;
        combined.error_count = sd_stats_.error_count + spiffs_stats_.error_count;
        return combined;
    }
}

bool StorageManager::format(StorageType type) {
    if (type == StorageType::SPIFFS) {
        logInfo("STORAGE", "Formatting SPIFFS");
        if (SPIFFS.format()) {
            logInfo("STORAGE", "SPIFFS formatted successfully");
            return true;
        } else {
            logError("STORAGE", "Failed to format SPIFFS");
            return false;
        }
    } else if (type == StorageType::SD_CARD) {
        logWarn("STORAGE", "SD card formatting not supported");
        return false;
    }
    
    return false;
}

bool StorageManager::checkHealth(StorageType type) {
    if (type == StorageType::SPIFFS) {
        return spiffs_mounted_;
    } else if (type == StorageType::SD_CARD) {
        return sd_card_mounted_;
    } else {
        return spiffs_mounted_ && sd_card_mounted_;
    }
}

bool StorageManager::loadJSON(const String& path, JsonDocument& doc, StorageType type) {
    String content = readFile(path, type);
    if (content.length() == 0) {
        return false;
    }
    
    DeserializationError error = deserializeJson(doc, content);
    if (error) {
        logError("STORAGE", "Failed to parse JSON from file: " + path);
        return false;
    }
    
    return true;
}

bool StorageManager::saveJSON(const String& path, const JsonDocument& doc, StorageType type) {
    String content;
    serializeJson(doc, content);
    
    return writeFile(path, content, type);
}

size_t StorageManager::getFreeSpace(StorageType type) {
    if (type == StorageType::SPIFFS) {
        return SPIFFS.totalBytes() - SPIFFS.usedBytes();
    } else if (type == StorageType::SD_CARD) {
        return SD.totalBytes() - SD.usedBytes();
    } else {
        return getFreeSpace(StorageType::SPIFFS) + getFreeSpace(StorageType::SD_CARD);
    }
}

size_t StorageManager::getTotalSpace(StorageType type) {
    if (type == StorageType::SPIFFS) {
        return SPIFFS.totalBytes();
    } else if (type == StorageType::SD_CARD) {
        return SD.totalBytes();
    } else {
        return getTotalSpace(StorageType::SPIFFS) + getTotalSpace(StorageType::SD_CARD);
    }
}

uint8_t StorageManager::getUsagePercentage(StorageType type) {
    size_t total = getTotalSpace(type);
    size_t used = total - getFreeSpace(type);
    
    if (total == 0) {
        return 0;
    }
    
    return (uint8_t)((used * 100) / total);
}

void StorageManager::process() {
    uint32_t now = millis();
    
    // Update statistics periodically
    if (now - last_stats_update_ > 30000) { // Every 30 seconds
        if (spiffs_mounted_) {
            updateStats(StorageType::SPIFFS);
        }
        if (sd_card_mounted_) {
            updateStats(StorageType::SD_CARD);
        }
        last_stats_update_ = now;
    }
}

fs::FS* StorageManager::getFileSystem(StorageType type) {
    switch (type) {
        case StorageType::SPIFFS:
            return spiffs_mounted_ ? spiffs_fs_ : nullptr;
        case StorageType::SD_CARD:
            return sd_card_mounted_ ? sd_fs_ : nullptr;
        case StorageType::BOTH:
            // For BOTH, try SPIFFS first, then SD card
            if (spiffs_mounted_) {
                return spiffs_fs_;
            } else if (sd_card_mounted_) {
                return sd_fs_;
            }
            return nullptr;
        default:
            return nullptr;
    }
}

bool StorageManager::pathExists(const String& path, StorageType type) {
    fs::FS* fs = getFileSystem(type);
    if (!fs) {
        return false;
    }
    
    return fs->exists(path);
}

FileInfo StorageManager::getFileInfo(const String& path, StorageType type) {
    FileInfo info;
    fs::FS* fs = getFileSystem(type);
    
    if (!fs) {
        return info;
    }
    
    File file = fs->open(path);
    if (!file) {
        return info;
    }
    
    info.name = getFileName(path);
    info.path = path;
    info.size = file.size();
    info.is_directory = file.isDirectory();
    info.last_modified = file.getLastWrite();
    info.extension = getFileExtension(path);
    
    file.close();
    return info;
}

void StorageManager::setState(StorageState state) {
    current_state_ = state;
}

void StorageManager::logEvent(const String& event) {
    logInfo("STORAGE", event);
}

void StorageManager::updateStats(StorageType type) {
    StorageStats* stats = nullptr;
    
    if (type == StorageType::SPIFFS) {
        stats = &spiffs_stats_;
        stats->total_space = SPIFFS.totalBytes();
        stats->used_space = SPIFFS.usedBytes();
        stats->free_space = SPIFFS.totalBytes() - SPIFFS.usedBytes();
    } else if (type == StorageType::SD_CARD) {
        stats = &sd_stats_;
        stats->total_space = SD.totalBytes();
        stats->used_space = SD.usedBytes();
        stats->free_space = SD.totalBytes() - SD.usedBytes();
    }
    
    if (stats) {
        // Count files and directories
        std::vector<FileInfo> files = listFiles("/", type);
        stats->file_count = 0;
        stats->directory_count = 0;
        
        for (const auto& file : files) {
            if (file.is_directory) {
                stats->directory_count++;
            } else {
                stats->file_count++;
            }
        }
    }
}

void StorageManager::handleError(const String& error, StorageType type) {
    error_count_++;
    last_error_ = error;
    logError("STORAGE", "Error: " + error);
}

// ===== GLOBAL STORAGE MANAGER FUNCTIONS =====

bool initializeStorageManager() {
    if (g_storage_manager) {
        return true;
    }
    
    g_storage_manager = new StorageManager();
    if (!g_storage_manager) {
        return false;
    }
    
    return g_storage_manager->initialize();
}

StorageManager* getStorageManager() {
    return g_storage_manager;
}

String getFileExtension(const String& path) {
    int last_dot = path.lastIndexOf('.');
    if (last_dot == -1) {
        return "";
    }
    return path.substring(last_dot + 1);
}

String getFileName(const String& path) {
    int last_slash = path.lastIndexOf('/');
    if (last_slash == -1) {
        return path;
    }
    return path.substring(last_slash + 1);
}

String getDirectoryPath(const String& path) {
    int last_slash = path.lastIndexOf('/');
    if (last_slash == -1) {
        return "";
    }
    return path.substring(0, last_slash);
}

String formatFileSize(size_t size) {
    if (size < 1024) {
        return String(size) + " B";
    } else if (size < 1024 * 1024) {
        return String(size / 1024.0, 1) + " KB";
    } else if (size < 1024 * 1024 * 1024) {
        return String(size / (1024.0 * 1024.0), 1) + " MB";
    } else {
        return String(size / (1024.0 * 1024.0 * 1024.0), 1) + " GB";
    }
}

bool isAbsolutePath(const String& path) {
    return path.startsWith("/");
}

String normalizePath(const String& path) {
    String normalized = path;
    
    // Remove double slashes
    while (normalized.indexOf("//") != -1) {
        normalized.replace("//", "/");
    }
    
    // Remove trailing slash
    if (normalized.endsWith("/") && normalized.length() > 1) {
        normalized = normalized.substring(0, normalized.length() - 1);
    }
    
    return normalized;
} 