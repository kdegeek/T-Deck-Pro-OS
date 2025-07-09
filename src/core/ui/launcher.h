#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include "core/utils/logger.h"

// Define necessary symbols if not provided by LVGL config
#ifndef LV_SYMBOL_WIFI
#define LV_SYMBOL_WIFI "\xef\x87\xab"
#endif
#ifndef LV_SYMBOL_CALL
#define LV_SYMBOL_CALL "\xef\x82\x95"
#endif
#ifndef LV_SYMBOL_SETTINGS
#define LV_SYMBOL_SETTINGS "\xef\x80\x93"
#endif
#ifndef LV_SYMBOL_BELL
#define LV_SYMBOL_BELL "\xef\x83\xb3"
#endif
#ifndef LV_SYMBOL_DIRECTORY
#define LV_SYMBOL_DIRECTORY "\xef\x81\xbb"
#endif
#ifndef LV_SYMBOL_PLUS
#define LV_SYMBOL_PLUS "\xef\x81\xa7"
#endif
#ifndef LV_SYMBOL_KEYBOARD
#define LV_SYMBOL_KEYBOARD "\xef\x84\x9c"
#endif
#ifndef LV_SYMBOL_GPS
#define LV_SYMBOL_GPS "\xef\x84\xa4"
#endif
#ifndef LV_SYMBOL_FILE
#define LV_SYMBOL_FILE "\xef\x85\x9b"
#endif
#ifndef LV_SYMBOL_CLOUD
#define LV_SYMBOL_CLOUD "\xef\x83\x82"
#endif

#define STATUS_BAR_HEIGHT 30
#define QUICK_SETTINGS_HEIGHT 50

struct Notification {
    String title;
    String message;
    uint32_t timestamp;
};

class Launcher {
public:
    static Launcher* getInstance();
    bool init();
    void update();
    void addNotification(const String& title, const String& message);

private:
    Launcher();
    static Launcher* instance;

    bool initialized;

    // UI Objects
    lv_obj_t* currentScreen;
    lv_obj_t* statusBar;
    lv_obj_t* appGrid;
    lv_obj_t* quickSettings;
    lv_obj_t* notificationPanel;
    lv_obj_t* appContainer;
    lv_obj_t* notificationList;

    // Status Bar Elements
    lv_obj_t* timeLabel;
    lv_obj_t* batteryLabel;
    lv_obj_t* wifiIcon;
    lv_obj_t* cellularIcon;
    lv_obj_t* loraIcon;
    lv_obj_t* notificationIcon;

    // Quick Settings Elements
    lv_obj_t* wifiToggle;
    lv_obj_t* cellularToggle;
    lv_obj_t* loraToggle;
    lv_obj_t* settingsBtn;
    
    std::vector<lv_obj_t*> appButtons;
    std::vector<Notification> notifications;

    lv_timer_t* statusUpdateTimer;

    // Internal UI creation
    void createStatusBar();
    void createAppGrid();
    void createQuickSettings();
    void createNotificationPanel();

    // Core functionality
    void refreshAppGrid();
    void updateStatus();
    void showNotifications();
    void hideNotifications();
    void clearNotifications();
    const char* getAppIcon(const String& appName);
    bool lvgl_initialized();

    // Static callbacks for LVGL
    static void statusUpdateCallback(lv_timer_t* timer);
    static void appLaunchCallback(lv_event_t* e);
    static void wifiToggleCallback(lv_event_t* e);
    static void cellularToggleCallback(lv_event_t* e);
    static void loraToggleCallback(lv_event_t* e);
    static void settingsCallback(lv_event_t* e);
    static void notificationClickCallback(lv_event_t* e);
};