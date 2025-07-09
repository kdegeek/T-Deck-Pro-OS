#include "file_manager_app.h"
#include "core/utils/logger.h"

FileManagerApp::FileManagerApp(const AppInfo& info) : AppBase(info) {
    Logger::info("FileManagerApp", "File manager app created");
}

FileManagerApp::~FileManagerApp() {
    // Destructor - cleanup is handled by base class
}

bool FileManagerApp::initialize() {
    Logger::info("FileManagerApp", "Initializing file manager app");
    // Initialize file manager components
    return true;
}

bool FileManagerApp::start() {
    Logger::info("FileManagerApp", "Starting file manager app");
    // Start file manager UI
    return true;
}

bool FileManagerApp::pauseApp() {
    Logger::info("FileManagerApp", "Pausing file manager app");
    // Pause file operations
    return true;
}

bool FileManagerApp::resumeApp() {
    Logger::info("FileManagerApp", "Resuming file manager app");
    // Resume file operations
    return true;
}

bool FileManagerApp::stop() {
    Logger::info("FileManagerApp", "Stopping file manager app");
    // Stop file manager services
    return true;
}

void FileManagerApp::cleanup() {
    Logger::info("FileManagerApp", "Cleaning up file manager app");
    // Cleanup file manager resources
}

AppBase::AppInfo FileManagerApp::getAppInfo() {
    AppBase::AppInfo info;
    info.name = "File Manager";
    info.version = "1.0.0";
    info.description = "Browse and manage files";
    info.author = "T-Deck-Pro OS";
    info.memoryUsage = 32 * 1024; // 32KB
    info.priority = AppBase::AppPriority::NORMAL_PRIORITY;
    info.canRunInBackground = false;
    info.requiresNetwork = false;
    info.requiresSD = true;
    info.iconPath = "📁";
    return info;
}