#include "launcher.h"
#include "../utils/logger.h"
#include "../storage/storage_manager.h"
#include "../apps/app_manager.h"
#include "../communication/communication_manager.h"
#include <lvgl.h>

Launcher* Launcher::instance = nullptr;

Launcher::Launcher() : 
    currentScreen(nullptr),
    statusBar(nullptr),
    appGrid(nullptr),
    quickSettings(nullptr),
    notificationPanel(nullptr),
    currentPage(0),
    totalPages(0),
    initialized(false) {
}

Launcher& Launcher::getInstance() {
    if (!instance) {
        instance = new Launcher();
    }
    return *instance;
}

bool Launcher::init() {
    Logger::info("Launcher", "Initializing launcher UI...");
    
    if (!lvgl_initialized()) {
        Logger::error("Launcher", "LVGL not initialized");
        return false;
    }
    
    // Create main screen
    currentScreen = lv_scr_act();
    lv_obj_set_style_bg_color(currentScreen, lv_color_white(), 0);
    
    // Initialize UI components
    if (!createStatusBar()) {
        Logger::error("Launcher", "Failed to create status bar");
        return false;
    }
    
    if (!createAppGrid()) {
        Logger::error("Launcher", "Failed to create app grid");
        return false;
    }
    
    if (!createQuickSettings()) {
        Logger::error("Launcher", "Failed to create quick settings");
        return false;
    }
    
    if (!createNotificationPanel()) {
        Logger::error("Launcher", "Failed to create notification panel");
        return false;
    }
    
    // Load installed apps
    refreshAppGrid();
    
    // Start status update timer
    statusUpdateTimer = lv_timer_create(statusUpdateCallback, 1000, this);
    
    initialized = true;
    Logger::info("Launcher", "Launcher initialized successfully");
    return true;
}

bool Launcher::createStatusBar() {
    // Create status bar container
    statusBar = lv_obj_create(currentScreen);
    lv_obj_set_size(statusBar, LV_HOR_RES, STATUS_BAR_HEIGHT);
    lv_obj_set_pos(statusBar, 0, 0);
    lv_obj_set_style_bg_color(statusBar, lv_color_black(), 0);
    lv_obj_set_style_border_width(statusBar, 0, 0);
    lv_obj_set_style_radius(statusBar, 0, 0);
    
    // Time label
    timeLabel = lv_label_create(statusBar);
    lv_obj_set_pos(timeLabel, 10, 5);
    lv_obj_set_style_text_color(timeLabel, lv_color_white(), 0);
    lv_label_set_text(timeLabel, "00:00");
    
    // Battery indicator
    batteryLabel = lv_label_create(statusBar);
    lv_obj_align(batteryLabel, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_obj_set_style_text_color(batteryLabel, lv_color_white(), 0);
    lv_label_set_text(batteryLabel, "100%");
    
    // WiFi indicator
    wifiIcon = lv_label_create(statusBar);
    lv_obj_align_to(wifiIcon, batteryLabel, LV_ALIGN_OUT_LEFT_MID, -20, 0);
    lv_obj_set_style_text_color(wifiIcon, lv_color_white(), 0);
    lv_label_set_text(wifiIcon, LV_SYMBOL_WIFI);
    
    // Cellular indicator
    cellularIcon = lv_label_create(statusBar);
    lv_obj_align_to(cellularIcon, wifiIcon, LV_ALIGN_OUT_LEFT_MID, -20, 0);
    lv_obj_set_style_text_color(cellularIcon, lv_color_white(), 0);
    lv_label_set_text(cellularIcon, LV_SYMBOL_CALL);
    
    // LoRa indicator
    loraIcon = lv_label_create(statusBar);
    lv_obj_align_to(loraIcon, cellularIcon, LV_ALIGN_OUT_LEFT_MID, -20, 0);
    lv_obj_set_style_text_color(loraIcon, lv_color_white(), 0);
    lv_label_set_text(loraIcon, "LoRa");
    
    // Notification indicator
    notificationIcon = lv_label_create(statusBar);
    lv_obj_align_to(notificationIcon, loraIcon, LV_ALIGN_OUT_LEFT_MID, -20, 0);
    lv_obj_set_style_text_color(notificationIcon, lv_color_white(), 0);
    lv_label_set_text(notificationIcon, "");
    
    return true;
}

bool Launcher::createAppGrid() {
    // Create app grid container
    appGrid = lv_obj_create(currentScreen);
    lv_obj_set_size(appGrid, LV_HOR_RES, LV_VER_RES - STATUS_BAR_HEIGHT - QUICK_SETTINGS_HEIGHT);
    lv_obj_set_pos(appGrid, 0, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(appGrid, lv_color_white(), 0);
    lv_obj_set_style_border_width(appGrid, 0, 0);
    lv_obj_set_style_radius(appGrid, 0, 0);
    lv_obj_set_style_pad_all(appGrid, 10, 0);
    
    // Create scrollable container for apps
    appContainer = lv_obj_create(appGrid);
    lv_obj_set_size(appContainer, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(appContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(appContainer, 0, 0);
    lv_obj_set_scroll_dir(appContainer, LV_DIR_VER);
    
    return true;
}

bool Launcher::createQuickSettings() {
    // Create quick settings panel
    quickSettings = lv_obj_create(currentScreen);
    lv_obj_set_size(quickSettings, LV_HOR_RES, QUICK_SETTINGS_HEIGHT);
    lv_obj_set_pos(quickSettings, 0, LV_VER_RES - QUICK_SETTINGS_HEIGHT);
    lv_obj_set_style_bg_color(quickSettings, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_border_width(quickSettings, 1, 0);
    lv_obj_set_style_border_color(quickSettings, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_radius(quickSettings, 0, 0);
    
    // WiFi toggle button
    wifiToggle = lv_btn_create(quickSettings);
    lv_obj_set_size(wifiToggle, 60, 30);
    lv_obj_set_pos(wifiToggle, 10, 10);
    lv_obj_add_event_cb(wifiToggle, wifiToggleCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* wifiLabel = lv_label_create(wifiToggle);
    lv_label_set_text(wifiLabel, "WiFi");
    lv_obj_center(wifiLabel);
    
    // Cellular toggle button
    cellularToggle = lv_btn_create(quickSettings);
    lv_obj_set_size(cellularToggle, 60, 30);
    lv_obj_set_pos(cellularToggle, 80, 10);
    lv_obj_add_event_cb(cellularToggle, cellularToggleCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* cellularLabel = lv_label_create(cellularToggle);
    lv_label_set_text(cellularLabel, "4G");
    lv_obj_center(cellularLabel);
    
    // LoRa toggle button
    loraToggle = lv_btn_create(quickSettings);
    lv_obj_set_size(loraToggle, 60, 30);
    lv_obj_set_pos(loraToggle, 150, 10);
    lv_obj_add_event_cb(loraToggle, loraToggleCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* loraLabel = lv_label_create(loraToggle);
    lv_label_set_text(loraLabel, "LoRa");
    lv_obj_center(loraLabel);
    
    // Settings button
    settingsBtn = lv_btn_create(quickSettings);
    lv_obj_set_size(settingsBtn, 60, 30);
    lv_obj_align(settingsBtn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_event_cb(settingsBtn, settingsCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* settingsLabel = lv_label_create(settingsBtn);
    lv_label_set_text(settingsLabel, LV_SYMBOL_SETTINGS);
    lv_obj_center(settingsLabel);
    
    return true;
}

bool Launcher::createNotificationPanel() {
    // Create notification panel (initially hidden)
    notificationPanel = lv_obj_create(currentScreen);
    lv_obj_set_size(notificationPanel, LV_HOR_RES, LV_VER_RES / 2);
    lv_obj_set_pos(notificationPanel, 0, -LV_VER_RES / 2); // Hidden above screen
    lv_obj_set_style_bg_color(notificationPanel, lv_color_hex(0xF8F8F8), 0);
    lv_obj_set_style_border_width(notificationPanel, 1, 0);
    lv_obj_set_style_border_color(notificationPanel, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_radius(notificationPanel, 10, 0);
    
    // Notification title
    lv_obj_t* notifTitle = lv_label_create(notificationPanel);
    lv_label_set_text(notifTitle, "Notifications");
    lv_obj_set_pos(notifTitle, 10, 10);
    lv_obj_set_style_text_font(notifTitle, &lv_font_montserrat_16, 0);
    
    // Notification list
    notificationList = lv_list_create(notificationPanel);
    lv_obj_set_size(notificationList, lv_pct(95), lv_pct(80));
    lv_obj_set_pos(notificationList, 10, 40);
    
    return true;
}

void Launcher::refreshAppGrid() {
    if (!initialized || !appContainer) {
        return;
    }
    
    Logger::info("Launcher", "Refreshing app grid...");
    
    // Clear existing app buttons
    lv_obj_clean(appContainer);
    appButtons.clear();
    
    // Get installed apps from storage manager
    StorageManager& storage = StorageManager::getInstance();
    std::vector<String> apps = storage.getInstalledApps();
    
    // Add system apps
    apps.insert(apps.begin(), "File Manager");
    apps.insert(apps.begin(), "Settings");
    apps.insert(apps.begin(), "Meshtastic");
    
    // Calculate grid layout
    int appsPerRow = 4;
    int appWidth = (LV_HOR_RES - 60) / appsPerRow; // 60px total padding
    int appHeight = 80;
    int row = 0, col = 0;
    
    for (const String& appName : apps) {
        // Create app button
        lv_obj_t* appBtn = lv_btn_create(appContainer);
        lv_obj_set_size(appBtn, appWidth - 10, appHeight);
        lv_obj_set_pos(appBtn, col * appWidth + 5, row * (appHeight + 10) + 5);
        lv_obj_add_event_cb(appBtn, appLaunchCallback, LV_EVENT_CLICKED, this);
        
        // Store app name in user data
        char* appNameCopy = (char*)malloc(appName.length() + 1);
        strcpy(appNameCopy, appName.c_str());
        lv_obj_set_user_data(appBtn, appNameCopy);
        
        // Create app icon (placeholder)
        lv_obj_t* appIcon = lv_label_create(appBtn);
        lv_label_set_text(appIcon, getAppIcon(appName));
        lv_obj_set_style_text_font(appIcon, &lv_font_montserrat_20, 0);
        lv_obj_align(appIcon, LV_ALIGN_CENTER, 0, -10);
        
        // Create app label
        lv_obj_t* appLabel = lv_label_create(appBtn);
        lv_label_set_text(appLabel, appName.c_str());
        lv_obj_set_style_text_font(appLabel, &lv_font_montserrat_10, 0);
        lv_obj_align(appLabel, LV_ALIGN_CENTER, 0, 15);
        lv_label_set_long_mode(appLabel, LV_LABEL_LONG_DOT);
        lv_obj_set_width(appLabel, appWidth - 20);
        
        appButtons.push_back(appBtn);
        
        // Move to next position
        col++;
        if (col >= appsPerRow) {
            col = 0;
            row++;
        }
    }
    
    // Update container height
    int totalHeight = (row + (col > 0 ? 1 : 0)) * (appHeight + 10) + 10;
    lv_obj_set_height(appContainer, totalHeight);
    
    Logger::info("Launcher", "App grid refreshed with %d apps", apps.size());
}

void Launcher::updateStatus() {
    if (!initialized) {
        return;
    }
    
    // Update time
    // Note: In real implementation, get actual time from RTC
    static int hours = 12, minutes = 0;
    minutes++;
    if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) hours = 0;
    }
    lv_label_set_text_fmt(timeLabel, "%02d:%02d", hours, minutes);
    
    // Update battery level
    // Note: In real implementation, read from battery ADC
    static int battery = 100;
    if (battery > 0) battery--;
    lv_label_set_text_fmt(batteryLabel, "%d%%", battery);
    
    // Update connectivity status
    CommunicationManager& comm = CommunicationManager::getInstance();
    
    // WiFi status
    if (comm.isWiFiConnected()) {
        lv_obj_set_style_text_color(wifiIcon, lv_color_hex(0x00FF00), 0);
    } else {
        lv_obj_set_style_text_color(wifiIcon, lv_color_hex(0xFF0000), 0);
    }
    
    // Cellular status
    if (comm.isCellularConnected()) {
        lv_obj_set_style_text_color(cellularIcon, lv_color_hex(0x00FF00), 0);
    } else {
        lv_obj_set_style_text_color(cellularIcon, lv_color_hex(0xFF0000), 0);
    }
    
    // LoRa status
    if (comm.isLoRaActive()) {
        lv_obj_set_style_text_color(loraIcon, lv_color_hex(0x00FF00), 0);
    } else {
        lv_obj_set_style_text_color(loraIcon, lv_color_hex(0xFF0000), 0);
    }
    
    // Update notification indicator
    if (notifications.size() > 0) {
        lv_label_set_text_fmt(notificationIcon, "%d", notifications.size());
        lv_obj_set_style_text_color(notificationIcon, lv_color_hex(0xFF6600), 0);
    } else {
        lv_label_set_text(notificationIcon, "");
    }
}

void Launcher::showNotifications() {
    if (!notificationPanel) {
        return;
    }
    
    Logger::info("Launcher", "Showing notifications panel");
    
    // Animate notification panel sliding down
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_obj(&anim, notificationPanel);
    lv_anim_set_values(&anim, -LV_VER_RES / 2, STATUS_BAR_HEIGHT);
    lv_anim_set_time(&anim, 300);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_start(&anim);
}

void Launcher::hideNotifications() {
    if (!notificationPanel) {
        return;
    }
    
    Logger::info("Launcher", "Hiding notifications panel");
    
    // Animate notification panel sliding up
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_obj(&anim, notificationPanel);
    lv_anim_set_values(&anim, STATUS_BAR_HEIGHT, -LV_VER_RES / 2);
    lv_anim_set_time(&anim, 300);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_start(&anim);
}

void Launcher::addNotification(const String& title, const String& message) {
    Logger::info("Launcher", "Adding notification: %s", title.c_str());
    
    Notification notif;
    notif.title = title;
    notif.message = message;
    notif.timestamp = millis();
    
    notifications.push_back(notif);
    
    // Add to notification list UI
    if (notificationList) {
        lv_obj_t* listBtn = lv_list_add_btn(notificationList, LV_SYMBOL_BELL, title.c_str());
        lv_obj_add_event_cb(listBtn, notificationClickCallback, LV_EVENT_CLICKED, this);
    }
    
    // Limit notifications to prevent memory issues
    if (notifications.size() > 10) {
        notifications.erase(notifications.begin());
        // Also remove from UI list
        if (notificationList) {
            lv_obj_t* firstChild = lv_obj_get_child(notificationList, 0);
            if (firstChild) {
                lv_obj_del(firstChild);
            }
        }
    }
}

void Launcher::clearNotifications() {
    Logger::info("Launcher", "Clearing all notifications");
    
    notifications.clear();
    
    if (notificationList) {
        lv_obj_clean(notificationList);
    }
}

const char* Launcher::getAppIcon(const String& appName) {
    if (appName == "Meshtastic") return LV_SYMBOL_WIFI;
    if (appName == "File Manager") return LV_SYMBOL_DIRECTORY;
    if (appName == "Settings") return LV_SYMBOL_SETTINGS;
    if (appName == "Calculator") return LV_SYMBOL_PLUS;
    if (appName == "Terminal") return LV_SYMBOL_KEYBOARD;
    if (appName == "GPS") return LV_SYMBOL_GPS;
    if (appName == "Weather") return LV_SYMBOL_CLOUD;
    return LV_SYMBOL_FILE; // Default icon
}

// Static callback functions
void Launcher::statusUpdateCallback(lv_timer_t* timer) {
    Launcher* launcher = (Launcher*)timer->user_data;
    launcher->updateStatus();
}

void Launcher::appLaunchCallback(lv_event_t* e) {
    Launcher* launcher = (Launcher*)lv_event_get_user_data(e);
    lv_obj_t* btn = lv_event_get_target(e);
    char* appName = (char*)lv_obj_get_user_data(btn);
    
    if (appName) {
        Logger::info("Launcher", "Launching app: %s", appName);
        
        // Launch app through app manager
        AppManager& appManager = AppManager::getInstance();
        appManager.launchApp(String(appName));
    }
}

void Launcher::wifiToggleCallback(lv_event_t* e) {
    Launcher* launcher = (Launcher*)lv_event_get_user_data(e);
    CommunicationManager& comm = CommunicationManager::getInstance();
    
    if (comm.isWiFiConnected()) {
        comm.disconnectWiFi();
        Logger::info("Launcher", "WiFi disabled");
    } else {
        comm.connectWiFi();
        Logger::info("Launcher", "WiFi enabled");
    }
}

void Launcher::cellularToggleCallback(lv_event_t* e) {
    Launcher* launcher = (Launcher*)lv_event_get_user_data(e);
    CommunicationManager& comm = CommunicationManager::getInstance();
    
    if (comm.isCellularConnected()) {
        comm.disconnectCellular();
        Logger::info("Launcher", "Cellular disabled");
    } else {
        comm.connectCellular();
        Logger::info("Launcher", "Cellular enabled");
    }
}

void Launcher::loraToggleCallback(lv_event_t* e) {
    Launcher* launcher = (Launcher*)lv_event_get_user_data(e);
    CommunicationManager& comm = CommunicationManager::getInstance();
    
    if (comm.isLoRaActive()) {
        comm.stopLoRa();
        Logger::info("Launcher", "LoRa disabled");
    } else {
        comm.startLoRa();
        Logger::info("Launcher", "LoRa enabled");
    }
}

void Launcher::settingsCallback(lv_event_t* e) {
    Launcher* launcher = (Launcher*)lv_event_get_user_data(e);
    
    Logger::info("Launcher", "Opening settings");
    
    // Launch settings app
    AppManager& appManager = AppManager::getInstance();
    appManager.launchApp("Settings");
}

void Launcher::notificationClickCallback(lv_event_t* e) {
    Launcher* launcher = (Launcher*)lv_event_get_user_data(e);
    
    // Handle notification click
    Logger::info("Launcher", "Notification clicked");
    launcher->hideNotifications();
}

bool Launcher::lvgl_initialized() {
    // Check if LVGL is properly initialized
    return lv_is_initialized();
}