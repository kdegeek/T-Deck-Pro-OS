/**
 * @file file_manager_app.cpp
 * @brief T-Deck-Pro File Manager App - SD Card and System File Browser
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Provides file browsing, management, and application handling
 */

#include "file_manager_app.h"
#include "../core/utils/logger.h"
#include "../core/storage/storage_manager.h"
#include "../core/launcher.h"
#include <ArduinoJson.h>

// ===== FILE MANAGER APP IMPLEMENTATION =====

FileManagerApp::FileManagerApp() : initialized_(false), current_path_("/"), 
                                   selected_index_(0), view_mode_(ViewMode::LIST),
                                   sort_mode_(SortMode::NAME), sort_reverse_(false) {
    
    // Initialize file list
    files_.reserve(100);
    
    // Initialize app info
    app_info_.name = "File Manager";
    app_info_.description = "Browse and manage files on SD card and system";
    app_info_.type = AppType::SYSTEM;
    app_info_.enabled = true;
    app_info_.memory_usage = 0;
    app_info_.last_used = 0;
}

FileManagerApp::~FileManagerApp() {
    if (initialized_) {
        cleanup();
    }
}

bool FileManagerApp::init() {
    if (initialized_) {
        return true;
    }
    
    logInfo("FILE_MANAGER", "Initializing file manager app");
    
    // Initialize storage manager if not already done
    if (!getStorageManager()) {
        if (!initializeStorageManager()) {
            logError("FILE_MANAGER", "Failed to initialize storage manager");
            return false;
        }
    }
    
    // Load configuration
    loadConfig();
    
    // Initialize UI
    if (!initUI()) {
        logError("FILE_MANAGER", "Failed to initialize UI");
        return false;
    }
    
    initialized_ = true;
    app_info_.last_used = millis();
    
    logInfo("FILE_MANAGER", "File manager app initialized successfully");
    return true;
}

void FileManagerApp::run() {
    if (!initialized_) {
        if (!init()) {
            logError("FILE_MANAGER", "Failed to initialize file manager app");
            return;
        }
    }
    
    // Update app usage time
    app_info_.last_used = millis();
    
    // Refresh file list if needed
    if (shouldRefresh()) {
        refreshFileList();
    }
    
    // Process user input
    processInput();
    
    // Update display
    updateDisplay();
}

void FileManagerApp::cleanup() {
    if (!initialized_) {
        return;
    }
    
    logInfo("FILE_MANAGER", "Cleaning up file manager app");
    
    // Save configuration
    saveConfig();
    
    // Clear file list
    files_.clear();
    
    initialized_ = false;
}

const char* FileManagerApp::getName() {
    return "FileManager";
}

bool FileManagerApp::initUI() {
    // Create main UI elements
    // This will be implemented when LVGL is available
    
    logInfo("FILE_MANAGER", "UI initialized");
    return true;
}

void FileManagerApp::loadConfig() {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return;
    }
    
    DynamicJsonDocument doc(1024);
    if (storage->loadJSON("/config/file_manager.json", doc, StorageType::SPIFFS)) {
        // Load configuration from JSON
        if (doc.containsKey("current_path")) {
            current_path_ = doc["current_path"].as<String>();
        }
        if (doc.containsKey("view_mode")) {
            view_mode_ = (ViewMode)doc["view_mode"].as<int>();
        }
        if (doc.containsKey("sort_mode")) {
            sort_mode_ = (SortMode)doc["sort_mode"].as<int>();
        }
        if (doc.containsKey("sort_reverse")) {
            sort_reverse_ = doc["sort_reverse"].as<bool>();
        }
        
        logInfo("FILE_MANAGER", "Configuration loaded");
    } else {
        logInfo("FILE_MANAGER", "No configuration found, using defaults");
    }
}

void FileManagerApp::saveConfig() {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return;
    }
    
    DynamicJsonDocument doc(1024);
    doc["current_path"] = current_path_;
    doc["view_mode"] = (int)view_mode_;
    doc["sort_mode"] = (int)sort_mode_;
    doc["sort_reverse"] = sort_reverse_;
    
    if (storage->saveJSON("/config/file_manager.json", doc, StorageType::SPIFFS)) {
        logInfo("FILE_MANAGER", "Configuration saved");
    } else {
        logError("FILE_MANAGER", "Failed to save configuration");
    }
}

bool FileManagerApp::shouldRefresh() {
    static uint32_t last_refresh = 0;
    uint32_t now = millis();
    
    // Refresh every 5 seconds
    if (now - last_refresh > 5000) {
        last_refresh = now;
        return true;
    }
    
    return false;
}

void FileManagerApp::refreshFileList() {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        logError("FILE_MANAGER", "Storage manager not available");
        return;
    }
    
    logDebug("FILE_MANAGER", "Refreshing file list for path: " + current_path_);
    
    // Clear current file list
    files_.clear();
    
    // Get files from both SPIFFS and SD card
    std::vector<FileInfo> spiffs_files = storage->listFiles(current_path_, StorageType::SPIFFS);
    std::vector<FileInfo> sd_files = storage->listFiles(current_path_, StorageType::SD_CARD);
    
    // Combine files
    for (const auto& file : spiffs_files) {
        FileEntry entry;
        entry.info = file;
        entry.storage_type = StorageType::SPIFFS;
        files_.push_back(entry);
    }
    
    for (const auto& file : sd_files) {
        FileEntry entry;
        entry.info = file;
        entry.storage_type = StorageType::SD_CARD;
        files_.push_back(entry);
    }
    
    // Sort files
    sortFiles();
    
    logInfo("FILE_MANAGER", "File list refreshed: " + String(files_.size()) + " files");
}

void FileManagerApp::sortFiles() {
    std::sort(files_.begin(), files_.end(), [this](const FileEntry& a, const FileEntry& b) {
        bool result = false;
        
        switch (sort_mode_) {
            case SortMode::NAME:
                result = a.info.name.compareTo(b.info.name) < 0;
                break;
            case SortMode::SIZE:
                result = a.info.size < b.info.size;
                break;
            case SortMode::DATE:
                result = a.info.last_modified < b.info.last_modified;
                break;
            case SortMode::TYPE:
                result = a.info.extension.compareTo(b.info.extension) < 0;
                break;
        }
        
        return sort_reverse_ ? !result : result;
    });
}

void FileManagerApp::processInput() {
    // Handle keyboard input
    // This will be implemented when keyboard input is available
    
    // Handle touch input
    // This will be implemented when touch input is available
}

void FileManagerApp::updateDisplay() {
    // Update display with current file list
    // This will be implemented when display manager is available
    
    // For now, just log the current state
    logDebug("FILE_MANAGER", "Display updated - " + String(files_.size()) + " files in " + current_path_);
}

bool FileManagerApp::navigateTo(const String& path) {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return false;
    }
    
    String normalized_path = normalizePath(path);
    
    // Check if path exists in either storage
    if (!storage->pathExists(normalized_path, StorageType::SPIFFS) && 
        !storage->pathExists(normalized_path, StorageType::SD_CARD)) {
        logError("FILE_MANAGER", "Path does not exist: " + normalized_path);
        return false;
    }
    
    current_path_ = normalized_path;
    selected_index_ = 0;
    
    logInfo("FILE_MANAGER", "Navigated to: " + current_path_);
    return true;
}

bool FileManagerApp::navigateUp() {
    if (current_path_ == "/") {
        return false;
    }
    
    String parent_path = getDirectoryPath(current_path_);
    if (parent_path.length() == 0) {
        parent_path = "/";
    }
    
    return navigateTo(parent_path);
}

bool FileManagerApp::openFile(const String& file_path) {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return false;
    }
    
    // Determine storage type
    StorageType storage_type = StorageType::BOTH;
    if (storage->fileExists(file_path, StorageType::SPIFFS)) {
        storage_type = StorageType::SPIFFS;
    } else if (storage->fileExists(file_path, StorageType::SD_CARD)) {
        storage_type = StorageType::SD_CARD;
    } else {
        logError("FILE_MANAGER", "File not found: " + file_path);
        return false;
    }
    
    // Check file extension to determine action
    String extension = getFileExtension(file_path);
    extension.toLowerCase();
    
    if (extension == "json" || extension == "txt" || extension == "log") {
        // Open as text file
        return openTextFile(file_path, storage_type);
    } else if (extension == "app" || extension == "plugin") {
        // Open as application
        return openApplication(file_path, storage_type);
    } else {
        // Show file info
        return showFileInfo(file_path, storage_type);
    }
}

bool FileManagerApp::openTextFile(const String& file_path, StorageType storage_type) {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return false;
    }
    
    String content = storage->readFile(file_path, storage_type);
    if (content.length() == 0) {
        logError("FILE_MANAGER", "Failed to read file: " + file_path);
        return false;
    }
    
    logInfo("FILE_MANAGER", "Opened text file: " + file_path + " (" + String(content.length()) + " bytes)");
    
    // Show content (will be implemented with display manager)
    return true;
}

bool FileManagerApp::openApplication(const String& file_path, StorageType storage_type) {
    logInfo("FILE_MANAGER", "Opening application: " + file_path);
    
    // This will be implemented when plugin manager is available
    return true;
}

bool FileManagerApp::showFileInfo(const String& file_path, StorageType storage_type) {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return false;
    }
    
    FileInfo info = storage->getFileInfo(file_path, storage_type);
    
    logInfo("FILE_MANAGER", "File info: " + info.name + " (" + formatFileSize(info.size) + ")");
    
    // Show file info (will be implemented with display manager)
    return true;
}

bool FileManagerApp::deleteFile(const String& file_path) {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return false;
    }
    
    // Determine storage type
    StorageType storage_type = StorageType::BOTH;
    if (storage->fileExists(file_path, StorageType::SPIFFS)) {
        storage_type = StorageType::SPIFFS;
    } else if (storage->fileExists(file_path, StorageType::SD_CARD)) {
        storage_type = StorageType::SD_CARD;
    } else {
        logError("FILE_MANAGER", "File not found: " + file_path);
        return false;
    }
    
    if (storage->deleteFile(file_path, storage_type)) {
        logInfo("FILE_MANAGER", "Deleted file: " + file_path);
        return true;
    } else {
        logError("FILE_MANAGER", "Failed to delete file: " + file_path);
        return false;
    }
}

bool FileManagerApp::copyFile(const String& source, const String& destination) {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return false;
    }
    
    // Determine source storage type
    StorageType source_type = StorageType::BOTH;
    if (storage->fileExists(source, StorageType::SPIFFS)) {
        source_type = StorageType::SPIFFS;
    } else if (storage->fileExists(source, StorageType::SD_CARD)) {
        source_type = StorageType::SD_CARD;
    } else {
        logError("FILE_MANAGER", "Source file not found: " + source);
        return false;
    }
    
    // Determine destination storage type
    StorageType dest_type = StorageType::SD_CARD; // Default to SD card
    
    if (storage->copyFile(source, destination, source_type, dest_type)) {
        logInfo("FILE_MANAGER", "Copied file: " + source + " -> " + destination);
        return true;
    } else {
        logError("FILE_MANAGER", "Failed to copy file: " + source + " -> " + destination);
        return false;
    }
}

bool FileManagerApp::moveFile(const String& source, const String& destination) {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return false;
    }
    
    // Determine source storage type
    StorageType source_type = StorageType::BOTH;
    if (storage->fileExists(source, StorageType::SPIFFS)) {
        source_type = StorageType::SPIFFS;
    } else if (storage->fileExists(source, StorageType::SD_CARD)) {
        source_type = StorageType::SD_CARD;
    } else {
        logError("FILE_MANAGER", "Source file not found: " + source);
        return false;
    }
    
    // Determine destination storage type
    StorageType dest_type = StorageType::SD_CARD; // Default to SD card
    
    if (storage->moveFile(source, destination, source_type, dest_type)) {
        logInfo("FILE_MANAGER", "Moved file: " + source + " -> " + destination);
        return true;
    } else {
        logError("FILE_MANAGER", "Failed to move file: " + source + " -> " + destination);
        return false;
    }
}

String FileManagerApp::getCurrentPath() const {
    return current_path_;
}

std::vector<FileEntry> FileManagerApp::getFiles() const {
    return files_;
}

uint8_t FileManagerApp::getSelectedIndex() const {
    return selected_index_;
}

void FileManagerApp::setSelectedIndex(uint8_t index) {
    if (index < files_.size()) {
        selected_index_ = index;
    }
}

ViewMode FileManagerApp::getViewMode() const {
    return view_mode_;
}

void FileManagerApp::setViewMode(ViewMode mode) {
    view_mode_ = mode;
}

SortMode FileManagerApp::getSortMode() const {
    return sort_mode_;
}

void FileManagerApp::setSortMode(SortMode mode) {
    sort_mode_ = mode;
    sortFiles();
}

bool FileManagerApp::isSortReversed() const {
    return sort_reverse_;
}

void FileManagerApp::setSortReversed(bool reversed) {
    sort_reverse_ = reversed;
    sortFiles();
}

// ===== GLOBAL FILE MANAGER APP INSTANCE =====
FileManagerApp* g_file_manager_app = nullptr;

// ===== FILE MANAGER APP UTILITY FUNCTIONS =====

bool initializeFileManagerApp() {
    if (g_file_manager_app) {
        return true;
    }
    
    g_file_manager_app = new FileManagerApp();
    if (!g_file_manager_app) {
        return false;
    }
    
    return g_file_manager_app->init();
}

FileManagerApp* getFileManagerApp() {
    return g_file_manager_app;
} 