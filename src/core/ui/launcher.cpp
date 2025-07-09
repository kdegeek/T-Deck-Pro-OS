#include "launcher.h"
#include "core/storage/storage_manager.h"
#include "core/apps/app_manager.h"
#include "core/communication/communication_manager.h"

UILauncher* UILauncher::instance = nullptr;

UILauncher::UILauncher() : 
    initialized(false),
    currentScreen(nullptr),
    statusBar(nullptr),
    appGrid(nullptr),
    quickSettings(nullptr),
    notificationPanel(nullptr),
    appContainer(nullptr),
    notificationList(nullptr),
    timeLabel(nullptr),
    batteryLabel(nullptr),
    wifiIcon(nullptr),
    cellularIcon(nullptr),
    loraIcon(nullptr),
    notificationIcon(nullptr),
    wifiToggle(nullptr),
    cellularToggle(nullptr),
    loraToggle(nullptr),
    settingsBtn(nullptr),
    statusUpdateTimer(nullptr) {
}

UILauncher* UILauncher::getInstance() {
    if (!instance) {
        instance = new UILauncher();
    }
    return instance;
}

bool UILauncher::init() {
    Logger::info("Launcher", "Initializing launcher UI...");
    
    if (!lvgl_initialized()) {
        Logger::error("Launcher", "LVGL not initialized");
        return false;
    }
    
    currentScreen = lv_scr_act();
    lv_obj_set_style_bg_color(currentScreen, lv_color_white(), 0);
    
    createStatusBar();
    createAppGrid();
    createQuickSettings();
    createNotificationPanel();
    
    refreshAppGrid();
    
    statusUpdateTimer = lv_timer_create(statusUpdateCallback, 1000, this);
    
    initialized = true;
    Logger::info("Launcher", "Launcher initialized successfully");
    return true;
}

void UILauncher::update() {
    // This function can be used for periodic updates if needed
}

void UILauncher::createStatusBar() {
    statusBar = lv_obj_create(currentScreen);
    lv_obj_set_size(statusBar, LV_HOR_RES, STATUS_BAR_HEIGHT);
    lv_obj_align(statusBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(statusBar, lv_color_black(), 0);
    lv_obj_set_style_border_width(statusBar, 0, 0);
    lv_obj_set_style_radius(statusBar, 0, 0);
    
    timeLabel = lv_label_create(statusBar);
    lv_obj_align(timeLabel, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_text_color(timeLabel, lv_color_white(), 0);
    lv_label_set_text(timeLabel, "00:00");
    
    batteryLabel = lv_label_create(statusBar);
    lv_obj_align(batteryLabel, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(batteryLabel, lv_color_white(), 0);
    lv_label_set_text(batteryLabel, "100%");
    
    wifiIcon = lv_label_create(statusBar);
    lv_obj_align_to(wifiIcon, batteryLabel, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_set_style_text_color(wifiIcon, lv_color_white(), 0);
    lv_label_set_text(wifiIcon, LV_SYMBOL_WIFI);
    
    cellularIcon = lv_label_create(statusBar);
    lv_obj_align_to(cellularIcon, wifiIcon, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_set_style_text_color(cellularIcon, lv_color_white(), 0);
    lv_label_set_text(cellularIcon, LV_SYMBOL_CALL);
    
    loraIcon = lv_label_create(statusBar);
    lv_obj_align_to(loraIcon, cellularIcon, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_set_style_text_color(loraIcon, lv_color_white(), 0);
    lv_label_set_text(loraIcon, "LoRa");

    notificationIcon = lv_label_create(statusBar);
    lv_obj_align(notificationIcon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(notificationIcon, lv_color_white(), 0);
    lv_label_set_text(notificationIcon, "");
}

void UILauncher::createAppGrid() {
    appGrid = lv_obj_create(currentScreen);
    lv_obj_set_size(appGrid, LV_HOR_RES, LV_VER_RES - STATUS_BAR_HEIGHT - QUICK_SETTINGS_HEIGHT);
    lv_obj_align(appGrid, LV_ALIGN_DEFAULT, 0, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(appGrid, lv_color_white(), 0);
    lv_obj_set_style_border_width(appGrid, 0, 0);
    lv_obj_set_style_pad_all(appGrid, 10, 0);
    
    appContainer = lv_obj_create(appGrid);
    lv_obj_set_size(appContainer, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(appContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(appContainer, 0, 0);
    lv_obj_set_scroll_dir(appContainer, LV_DIR_VER);
}

void UILauncher::createQuickSettings() {
    quickSettings = lv_obj_create(currentScreen);
    lv_obj_set_size(quickSettings, LV_HOR_RES, QUICK_SETTINGS_HEIGHT);
    lv_obj_align(quickSettings, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(quickSettings, lv_color_hex(0xF0F0F0), 0);
    
    wifiToggle = lv_btn_create(quickSettings);
    lv_obj_align(wifiToggle, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_add_event_cb(wifiToggle, wifiToggleCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* wifiLabel = lv_label_create(wifiToggle);
    lv_label_set_text(wifiLabel, "WiFi");
    lv_obj_center(wifiLabel);

    cellularToggle = lv_btn_create(quickSettings);
    lv_obj_align_to(cellularToggle, wifiToggle, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(cellularToggle, cellularToggleCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* cellularLabel = lv_label_create(cellularToggle);
    lv_label_set_text(cellularLabel, "4G");
    lv_obj_center(cellularLabel);

    loraToggle = lv_btn_create(quickSettings);
    lv_obj_align_to(loraToggle, cellularToggle, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(loraToggle, loraToggleCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* loraLabel = lv_label_create(loraToggle);
    lv_label_set_text(loraLabel, "LoRa");
    lv_obj_center(loraLabel);

    settingsBtn = lv_btn_create(quickSettings);
    lv_obj_align(settingsBtn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(settingsBtn, settingsCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* settingsLabel = lv_label_create(settingsBtn);
    lv_label_set_text(settingsLabel, LV_SYMBOL_SETTINGS);
    lv_obj_center(settingsLabel);
}

void UILauncher::createNotificationPanel() {
    notificationPanel = lv_obj_create(currentScreen);
    lv_obj_set_size(notificationPanel, LV_HOR_RES, LV_VER_RES / 2);
    lv_obj_align(notificationPanel, LV_ALIGN_TOP_MID, 0, -LV_VER_RES); // Start hidden
    lv_obj_set_style_bg_color(notificationPanel, lv_color_hex(0xF8F8F8), 0);
    
    lv_obj_t* notifTitle = lv_label_create(notificationPanel);
    lv_label_set_text(notifTitle, "Notifications");
    lv_obj_align(notifTitle, LV_ALIGN_TOP_LEFT, 10, 10);
    
    notificationList = lv_list_create(notificationPanel);
    lv_obj_set_size(notificationList, lv_pct(95), lv_pct(80));
    lv_obj_align(notificationList, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void UILauncher::refreshAppGrid() {
    if (!initialized || !appContainer) return;

    Logger::info("Launcher", "Refreshing app grid...");
    lv_obj_clean(appContainer);
    appButtons.clear();
    
    StorageManager* storage = &StorageManager::getInstance();
    std::vector<String> apps = storage->getInstalledApps();
    
    apps.insert(apps.begin(), "File Manager");
    apps.insert(apps.begin(), "Settings");
    apps.insert(apps.begin(), "Meshtastic");

    int appsPerRow = 4;
    int appWidth = (lv_obj_get_width(appContainer) - (appsPerRow + 1) * 10) / appsPerRow;
    int appHeight = 80;
    
    for (size_t i = 0; i < apps.size(); ++i) {
        const String& appName = apps[i];
        int row = i / appsPerRow;
        int col = i % appsPerRow;

        lv_obj_t* appBtn = lv_btn_create(appContainer);
        lv_obj_set_size(appBtn, appWidth, appHeight);
        lv_obj_align(appBtn, LV_ALIGN_TOP_LEFT, col * (appWidth + 10) + 10, row * (appHeight + 10) + 10);
        
        // Use lv_obj_add_event_cb instead of direct user data for simplicity with statics
        lv_obj_add_event_cb(appBtn, appLaunchCallback, LV_EVENT_CLICKED, (void*)apps[i].c_str());

        lv_obj_t* appIcon = lv_label_create(appBtn);
        lv_label_set_text(appIcon, getAppIcon(appName));
        lv_obj_align(appIcon, LV_ALIGN_TOP_MID, 0, 10);
        
        lv_obj_t* appLabel = lv_label_create(appBtn);
        lv_label_set_text(appLabel, appName.c_str());
        lv_obj_set_style_text_align(appLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(appLabel, lv_pct(100));
        lv_obj_align(appLabel, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_label_set_long_mode(appLabel, LV_LABEL_LONG_DOT);
        
        appButtons.push_back(appBtn);
    }
    Logger::info("Launcher", "App grid refreshed with " + String(apps.size()) + " apps");
}

void UILauncher::updateStatus() {
    if (!initialized) return;

    static int hours = 12, minutes = 0, seconds = 0;
    seconds++;
    if(seconds >= 60) { seconds = 0; minutes++; }
    if(minutes >= 60) { minutes = 0; hours++; }
    if(hours >= 24) { hours = 0; }
    lv_label_set_text_fmt(timeLabel, "%02d:%02d", hours, minutes);

    static int battery = 100;
    lv_label_set_text_fmt(batteryLabel, "%d%%", battery);

    using TDeckOS::Communication::CommunicationManager;
    CommunicationManager* comm = CommunicationManager::getInstance();

    lv_obj_set_style_text_color(wifiIcon, comm->isWiFiConnected() ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_text_color(cellularIcon, comm->isCellularConnected() ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_text_color(loraIcon, comm->isLoRaActive() ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);

    if (!notifications.empty()) {
        lv_label_set_text_fmt(notificationIcon, "%d", notifications.size());
        lv_obj_set_style_text_color(notificationIcon, lv_color_hex(0xFF6600), 0);
    } else {
        lv_label_set_text(notificationIcon, "");
    }
}

void UILauncher::showNotifications() {
    if (!notificationPanel) return;
    Logger::info("Launcher", "Showing notifications panel");
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, notificationPanel);
    lv_anim_set_values(&anim, lv_obj_get_y(notificationPanel), STATUS_BAR_HEIGHT);
    lv_anim_set_time(&anim, 300);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_start(&anim);
}

void UILauncher::hideNotifications() {
    if (!notificationPanel) return;
    Logger::info("Launcher", "Hiding notifications panel");
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, notificationPanel);
    lv_anim_set_values(&anim, lv_obj_get_y(notificationPanel), -lv_obj_get_height(notificationPanel));
    lv_anim_set_time(&anim, 300);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_start(&anim);
}

void UILauncher::addNotification(const String& title, const String& message) {
    Logger::info("Launcher", "Adding notification: " + title);
    notifications.push_back({title, message, (uint32_t)millis()});
    if(notificationList) {
        lv_obj_t* listItem = lv_list_add_text(notificationList, (title + ": " + message).c_str());
        lv_obj_add_event_cb(listItem, notificationClickCallback, LV_EVENT_CLICKED, this);
    }
    if (notifications.size() > 20) {
        notifications.erase(notifications.begin());
        if(notificationList) {
           lv_obj_del(lv_obj_get_child(notificationList, 0));
        }
    }
}

void UILauncher::clearNotifications() {
    Logger::info("Launcher", "Clearing all notifications");
    notifications.clear();
    if (notificationList) {
        lv_obj_clean(notificationList);
    }
}

const char* UILauncher::getAppIcon(const String& appName) {
    if (appName == "Meshtastic") return LV_SYMBOL_WIFI;
    if (appName == "File Manager") return LV_SYMBOL_DIRECTORY;
    if (appName == "Settings") return LV_SYMBOL_SETTINGS;
    if (appName == "Weather") return LV_SYMBOL_CLOUD;
    return LV_SYMBOL_FILE;
}

bool UILauncher::lvgl_initialized() {
    return lv_is_initialized();
}

// --- Static Callback Implementations ---

void UILauncher::statusUpdateCallback(lv_timer_t* timer) {
    UILauncher* launcher = (UILauncher*)timer->user_data;
    if(launcher) launcher->updateStatus();
}

void UILauncher::appLaunchCallback(lv_event_t* e) {
    const char* appName = (const char*)lv_event_get_user_data(e);
    if (appName) {
        Logger::info("Launcher", "Launching app: " + String(appName));
        AppManager::getInstance().launchApp(String(appName));
    }
}

void UILauncher::wifiToggleCallback(lv_event_t* e) {
    using TDeckOS::Communication::CommunicationManager;
    CommunicationManager* comm = CommunicationManager::getInstance();
    bool enabled = comm->isInterfaceEnabled(TDeckOS::Communication::CommInterface::WIFI);
    comm->enableInterface(TDeckOS::Communication::CommInterface::WIFI, !enabled);
    Logger::info("Launcher", "WiFi " + String(enabled ? "disabled" : "enabled"));
}

void UILauncher::cellularToggleCallback(lv_event_t* e) {
    using TDeckOS::Communication::CommunicationManager;
    CommunicationManager* comm = CommunicationManager::getInstance();
    bool enabled = comm->isInterfaceEnabled(TDeckOS::Communication::CommInterface::CELLULAR);
    comm->enableInterface(TDeckOS::Communication::CommInterface::CELLULAR, !enabled);
    Logger::info("Launcher", "Cellular " + String(enabled ? "disabled" : "enabled"));
}

void UILauncher::loraToggleCallback(lv_event_t* e) {
    using TDeckOS::Communication::CommunicationManager;
    CommunicationManager* comm = CommunicationManager::getInstance();
    bool enabled = comm->isInterfaceEnabled(TDeckOS::Communication::CommInterface::LORA);
    comm->enableInterface(TDeckOS::Communication::CommInterface::LORA, !enabled);
    Logger::info("Launcher", "LoRa " + String(enabled ? "disabled" : "enabled"));
}

void UILauncher::settingsCallback(lv_event_t* e) {
    Logger::info("Launcher", "Opening settings app");
    AppManager::getInstance().launchApp("Settings");
}

void UILauncher::notificationClickCallback(lv_event_t* e) {
    Logger::info("Launcher", "Notification clicked");
    UILauncher* launcher = (UILauncher*)lv_event_get_user_data(e);
    if(launcher) launcher->hideNotifications();
}