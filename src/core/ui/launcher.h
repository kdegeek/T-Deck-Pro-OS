// launcher.h
// T-Deck-Pro OS Main Launcher/Home Screen
#pragma once

#include <Arduino.h>
#include "lvgl.h"
#include "core/utils/logger.h"
#include "core/apps/app_manager.h"
#include "core/storage/storage_manager.h"

// Launcher configuration
#define LAUNCHER_APPS_PER_PAGE 6
#define LAUNCHER_MAX_PAGES 10
#define LAUNCHER_ICON_SIZE 64
#define LAUNCHER_GRID_COLS 3
#define LAUNCHER_GRID_ROWS 2

// Launcher states
typedef enum {
    LAUNCHER_STATE_HOME,        // Main home screen
    LAUNCHER_STATE_APP_GRID,    // Application grid view
    LAUNCHER_STATE_SETTINGS,    // Quick settings
    LAUNCHER_STATE_NOTIFICATIONS, // Notification panel
    LAUNCHER_STATE_SEARCH       // App search
} launcher_state_t;

// App icon information
struct AppIcon {
    String appId;
    String displayName;
    String iconPath;
    bool isInstalled;
    bool isRunning;
    bool canUninstall;
    lv_obj_t* iconObj;
    lv_obj_t* labelObj;
};

// Status bar information
struct StatusInfo {
    bool wifiConnected;
    int wifiSignal;
    bool cellularConnected;
    int cellularSignal;
    bool loraActive;
    int batteryPercent;
    bool usbConnected;
    bool serverConnected;
    String currentTime;
    int runningApps;
    int notifications;
};

class Launcher {
private:
    static Launcher* instance;
    
    // LVGL objects
    lv_obj_t* mainScreen;
    lv_obj_t* statusBar;
    lv_obj_t* homeContainer;
    lv_obj_t* appGridContainer;
    lv_obj_t* settingsContainer;
    lv_obj_t* notificationContainer;
    
    // Status bar elements
    lv_obj_t* timeLabel;
    lv_obj_t* batteryIcon;
    lv_obj_t* wifiIcon;
    lv_obj_t* cellularIcon;
    lv_obj_t* loraIcon;
    lv_obj_t* serverIcon;
    lv_obj_t* notificationIcon;
    
    // App grid
    lv_obj_t* appGrid;
    lv_obj_t* pageIndicator;
    std::vector<AppIcon> appIcons;
    int currentPage;
    int totalPages;
    
    // Quick actions
    lv_obj_t* quickActionsContainer;
    lv_obj_t* searchButton;
    lv_obj_t* settingsButton;
    lv_obj_t* powerButton;
    
    // State management
    launcher_state_t currentState;
    bool initialized;
    uint32_t lastUpdate;
    
    // Internal methods
    void createStatusBar();
    void createHomeScreen();
    void createAppGrid();
    void createQuickSettings();
    void createNotificationPanel();
    void updateStatusBar();
    void updateAppGrid();
    void loadAppIcons();
    void refreshAppList();
    
    // Event handlers
    static void appIconEventHandler(lv_event_t* e);
    static void quickActionEventHandler(lv_event_t* e);
    static void settingsEventHandler(lv_event_t* e);
    static void gestureEventHandler(lv_event_t* e);
    
    // UI helpers
    void showState(launcher_state_t state);
    void hideAllContainers();
    void updatePageIndicator();
    lv_obj_t* createAppIcon(const AppIcon& appInfo, int gridPos);
    void showAppContextMenu(const String& appId);
    
public:
    Launcher();
    ~Launcher();
    
    // Singleton access
    static Launcher& getInstance();
    
    // Lifecycle
    bool initialize();
    void shutdown();
    void update();
    
    // State management
    void showHome();
    void showAppGrid();
    void showSettings();
    void showNotifications();
    void showSearch();
    launcher_state_t getCurrentState() const;
    
    // App management
    void launchApp(const String& appId);
    void closeApp(const String& appId);
    void uninstallApp(const String& appId);
    void refreshApps();
    
    // Navigation
    void nextPage();
    void previousPage();
    void goToPage(int page);
    
    // Status updates
    void updateStatus(const StatusInfo& status);
    void showNotification(const String& title, const String& message, int duration = 3000);
    void clearNotifications();
    
    // Settings
    void showQuickSettings();
    void toggleWifi();
    void toggleCellular();
    void toggleLora();
    void adjustBrightness(int level);
    void showPowerMenu();
    
    // Search
    void showSearchDialog();
    void searchApps(const String& query);
    
    // Customization
    void setWallpaper(const String& imagePath);
    void setTheme(const String& themeName);
    void setGridLayout(int cols, int rows);
    
    // LVGL integration
    lv_obj_t* getMainScreen() const;
    void handleInput(lv_event_t* e);
    
    // Utility
    bool isVisible() const;
    void forceRefresh();
};

// Convenience macros
#define LAUNCHER Launcher::getInstance()

// Launcher events
typedef enum {
    LAUNCHER_EVENT_APP_LAUNCHED,
    LAUNCHER_EVENT_APP_CLOSED,
    LAUNCHER_EVENT_STATE_CHANGED,
    LAUNCHER_EVENT_NOTIFICATION_ADDED,
    LAUNCHER_EVENT_SETTINGS_CHANGED
} launcher_event_t;

// Event callback type
typedef void (*launcher_event_callback_t)(launcher_event_t event, void* data);

// Launcher configuration
struct LauncherConfig {
    bool showStatusBar;
    bool enableGestures;
    bool autoHideStatusBar;
    int statusBarHeight;
    int iconSize;
    int gridCols;
    int gridRows;
    String wallpaperPath;
    String themeName;
    bool enableAnimations;
    int animationDuration;
};