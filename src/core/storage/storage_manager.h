/**
 * @file storage_manager.h
 * @brief T-Deck-Pro Storage Manager - SD Card and SPIFFS Management
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Handles SD card and SPIFFS file systems with error recovery
 */

#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "../hal/board_config_corrected.h"

// ===== STORAGE STATES =====
enum class StorageState {
    INIT,
    READY,
    ERROR,
    MOUNTING,
    FORMATTING
};

// ===== STORAGE TYPES =====
enum class StorageType {
    SPIFFS,
    SD_CARD,
    BOTH
};

// ===== STORAGE CONFIGURATION =====
#define STORAGE_MAX_PATH_LENGTH 128
#define STORAGE_MAX_FILE_SIZE 1048576  // 1MB
#define STORAGE_BUFFER_SIZE 1024
#define STORAGE_MOUNT_TIMEOUT_MS 5000

/**
 * @brief File information structure
 */
struct FileInfo {
    String name;
    String path;
    size_t size;
    bool is_directory;
    uint32_t last_modified;
    String extension;
};

/**
 * @brief Storage statistics
 */
struct StorageStats {
    size_t total_space;
    size_t used_space;
    size_t free_space;
    uint32_t file_count;
    uint32_t directory_count;
    uint32_t read_operations;
    uint32_t write_operations;
    uint32_t error_count;
};

/**
 * @brief Storage manager for T-Deck-Pro
 * @note Handles SD card and SPIFFS with error recovery
 */
class StorageManager {
public:
    StorageManager();
    ~StorageManager();
    
    /**
     * @brief Initialize storage manager
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Check if storage is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Get storage state
     * @return Current storage state
     */
    StorageState getState() const { return current_state_; }
    
    /**
     * @brief Mount SD card
     * @return true if successful
     */
    bool mountSDCard();
    
    /**
     * @brief Mount SPIFFS
     * @return true if successful
     */
    bool mountSPIFFS();
    
    /**
     * @brief Unmount SD card
     * @return true if successful
     */
    bool unmountSDCard();
    
    /**
     * @brief Unmount SPIFFS
     * @return true if successful
     */
    bool unmountSPIFFS();
    
    /**
     * @brief Check if SD card is mounted
     * @return true if mounted
     */
    bool isSDCardMounted() const { return sd_card_mounted_; }
    
    /**
     * @brief Check if SPIFFS is mounted
     * @return true if mounted
     */
    bool isSPIFFSMounted() const { return spiffs_mounted_; }
    
    /**
     * @brief List files in directory
     * @param path Directory path
     * @param type Storage type
     * @return Vector of file information
     */
    std::vector<FileInfo> listFiles(const String& path, StorageType type = StorageType::BOTH);
    
    /**
     * @brief Create directory
     * @param path Directory path
     * @param type Storage type
     * @return true if successful
     */
    bool createDirectory(const String& path, StorageType type = StorageType::SD_CARD);
    
    /**
     * @brief Delete file or directory
     * @param path File/directory path
     * @param type Storage type
     * @return true if successful
     */
    bool deleteFile(const String& path, StorageType type = StorageType::BOTH);
    
    /**
     * @brief Read file content
     * @param path File path
     * @param type Storage type
     * @return File content string
     */
    String readFile(const String& path, StorageType type = StorageType::BOTH);
    
    /**
     * @brief Write file content
     * @param path File path
     * @param content File content
     * @param type Storage type
     * @return true if successful
     */
    bool writeFile(const String& path, const String& content, StorageType type = StorageType::BOTH);
    
    /**
     * @brief Append to file
     * @param path File path
     * @param content Content to append
     * @param type Storage type
     * @return true if successful
     */
    bool appendFile(const String& path, const String& content, StorageType type = StorageType::BOTH);
    
    /**
     * @brief Check if file exists
     * @param path File path
     * @param type Storage type
     * @return true if exists
     */
    bool fileExists(const String& path, StorageType type = StorageType::BOTH);
    
    /**
     * @brief Get file size
     * @param path File path
     * @param type Storage type
     * @return File size in bytes
     */
    size_t getFileSize(const String& path, StorageType type = StorageType::BOTH);
    
    /**
     * @brief Copy file
     * @param source Source path
     * @param destination Destination path
     * @param source_type Source storage type
     * @param dest_type Destination storage type
     * @return true if successful
     */
    bool copyFile(const String& source, const String& destination, 
                  StorageType source_type = StorageType::SD_CARD,
                  StorageType dest_type = StorageType::SPIFFS);
    
    /**
     * @brief Move file
     * @param source Source path
     * @param destination Destination path
     * @param source_type Source storage type
     * @param dest_type Destination storage type
     * @return true if successful
     */
    bool moveFile(const String& source, const String& destination,
                  StorageType source_type = StorageType::SD_CARD,
                  StorageType dest_type = StorageType::SPIFFS);
    
    /**
     * @brief Get storage statistics
     * @param type Storage type
     * @return Storage statistics
     */
    StorageStats getStats(StorageType type = StorageType::BOTH);
    
    /**
     * @brief Format storage
     * @param type Storage type
     * @return true if successful
     */
    bool format(StorageType type = StorageType::SPIFFS);
    
    /**
     * @brief Check storage health
     * @param type Storage type
     * @return true if healthy
     */
    bool checkHealth(StorageType type = StorageType::BOTH);
    
    /**
     * @brief Load JSON configuration
     * @param path File path
     * @param doc JSON document
     * @param type Storage type
     * @return true if successful
     */
    bool loadJSON(const String& path, JsonDocument& doc, StorageType type = StorageType::BOTH);
    
    /**
     * @brief Save JSON configuration
     * @param path File path
     * @param doc JSON document
     * @param type Storage type
     * @return true if successful
     */
    bool saveJSON(const String& path, const JsonDocument& doc, StorageType type = StorageType::BOTH);
    
    /**
     * @brief Get available storage space
     * @param type Storage type
     * @return Available space in bytes
     */
    size_t getFreeSpace(StorageType type = StorageType::BOTH);
    
    /**
     * @brief Get total storage space
     * @param type Storage type
     * @return Total space in bytes
     */
    size_t getTotalSpace(StorageType type = StorageType::BOTH);
    
    /**
     * @brief Get storage usage percentage
     * @param type Storage type
     * @return Usage percentage (0-100)
     */
    uint8_t getUsagePercentage(StorageType type = StorageType::BOTH);
    
    /**
     * @brief Process storage events
     */
    void process();

private:
    // ===== STORAGE OPERATIONS =====
    
    /**
     * @brief Initialize storage hardware
     * @return true if successful
     */
    bool initHardware();
    
    /**
     * @brief Initialize file systems
     * @return true if successful
     */
    bool initFileSystems();
    
    /**
     * @brief Configure storage settings
     * @return true if successful
     */
    bool configureStorage();
    
    /**
     * @brief Get file system for type
     * @param type Storage type
     * @return File system pointer
     */
    fs::FS* getFileSystem(StorageType type);
    
    /**
     * @brief Check if path exists in storage type
     * @param path File path
     * @param type Storage type
     * @return true if exists
     */
    bool pathExists(const String& path, StorageType type);
    
    /**
     * @brief Get file information
     * @param path File path
     * @param type Storage type
     * @return File information
     */
    FileInfo getFileInfo(const String& path, StorageType type);
    
    /**
     * @brief Set storage state
     * @param state New state
     */
    void setState(StorageState state);
    
    /**
     * @brief Log storage event
     * @param event Event description
     */
    void logEvent(const String& event);
    
    /**
     * @brief Update storage statistics
     * @param type Storage type
     */
    void updateStats(StorageType type);
    
    /**
     * @brief Handle storage error
     * @param error Error message
     * @param type Storage type
     */
    void handleError(const String& error, StorageType type);

private:
    // ===== MEMBER VARIABLES =====
    bool initialized_;
    StorageState current_state_;
    bool sd_card_mounted_;
    bool spiffs_mounted_;
    
    // Storage statistics
    StorageStats sd_stats_;
    StorageStats spiffs_stats_;
    
    // Error tracking
    uint32_t error_count_;
    String last_error_;
    
    // Timing
    uint32_t last_check_time_;
    uint32_t last_stats_update_;
    
    // File system objects
    fs::FS* sd_fs_;
    fs::FS* spiffs_fs_;
};

// ===== GLOBAL STORAGE MANAGER INSTANCE =====
extern StorageManager* g_storage_manager;

// ===== STORAGE UTILITY FUNCTIONS =====

/**
 * @brief Initialize global storage manager
 * @return true if successful
 */
bool initializeStorageManager();

/**
 * @brief Get global storage manager instance
 * @return Storage manager pointer
 */
StorageManager* getStorageManager();

/**
 * @brief Get file extension from path
 * @param path File path
 * @return File extension
 */
String getFileExtension(const String& path);

/**
 * @brief Get file name from path
 * @param path File path
 * @return File name
 */
String getFileName(const String& path);

/**
 * @brief Get directory path from full path
 * @param path Full path
 * @return Directory path
 */
String getDirectoryPath(const String& path);

/**
 * @brief Format file size for display
 * @param size File size in bytes
 * @return Formatted size string
 */
String formatFileSize(size_t size);

/**
 * @brief Check if path is absolute
 * @param path File path
 * @return true if absolute
 */
bool isAbsolutePath(const String& path);

/**
 * @brief Normalize file path
 * @param path File path
 * @return Normalized path
 */
String normalizePath(const String& path);

#endif // STORAGE_MANAGER_H 