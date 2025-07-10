/************************************************************************
 * FilePath     : ui_config.h
 * Author       : T-Deck-Pro OS
 * Description  : UI Configuration - Safe to modify for customization
 * Copyright (c): 2024 T-Deck-Pro OS
 ************************************************************************/
#ifndef __UI_CONFIG_H__
#define __UI_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************
 *                              LAYOUT CONFIGURATION
 * *******************************************************************************/
// Screen dimensions and layout
#define UI_TASKBAR_HEIGHT           25
#define UI_STATUS_WIDGET_HEIGHT     55
#define UI_BUTTON_HEIGHT           45
#define UI_BUTTON_WIDTH_PERCENT    48      // 48% for 2-column layout
#define UI_MENU_PADDING            8
#define UI_BUTTON_SPACING          4

// Status widget layout (2x2 grid)
#define UI_STATUS_WIDGET_PADDING   4
#define UI_STATUS_WIDGET_RADIUS    8
#define UI_STATUS_WIDGET_BORDER    2

/*********************************************************************************
 *                              TIMING CONFIGURATION
 * *******************************************************************************/
// Update intervals (in seconds)
#define UI_STATUS_UPDATE_INTERVAL  5       // Status widget update frequency
#define UI_BATTERY_UPDATE_INTERVAL 10      // Battery status check frequency
#define UI_TOUCH_POLL_RATE         30      // Touch polling rate (ms)
#define UI_GESTURE_DISTANCE        15      // Touch gesture sensitivity (px)

/*********************************************************************************
 *                              COLOR CONFIGURATION
 * *******************************************************************************/
// Main interface colors
#define UI_COLOR_BG_NORMAL         0xFFFFFF    // White background
#define UI_COLOR_FG_NORMAL         0x000000    // Black text
#define UI_COLOR_BG_PRESSED        0x000000    // Black when pressed
#define UI_COLOR_FG_PRESSED        0xFFFFFF    // White text when pressed

// Status widget colors
#define UI_COLOR_STATUS_WIDGET_BG  0xF0F0F0    // Light gray background
#define UI_COLOR_STATUS_WIDGET_FG  0x000000    // Black text
#define UI_COLOR_STATUS_BORDER     0x666666    // Gray border

// Taskbar colors
#define UI_COLOR_TASKBAR_BG        0xFFFFFF    // White background
#define UI_COLOR_BREADCRUMB_BG     0xE0E0E0    // Light gray breadcrumb
#define UI_COLOR_BREADCRUMB_BORDER 0x999999    // Gray breadcrumb border

/*********************************************************************************
 *                              FONT CONFIGURATION
 * *******************************************************************************/
// Font assignments for different UI elements
#define UI_FONT_BUTTON             &Font_Mono_Bold_16
#define UI_FONT_STATUS_WIDGET      &Font_Mono_Bold_14
#define UI_FONT_TASKBAR            &Font_Mono_Bold_14
#define UI_FONT_BREADCRUMB         &Font_Mono_Bold_14

/*********************************************************************************
 *                              FEATURE CONFIGURATION
 * *******************************************************************************/
// Enable/disable features
#define UI_ENABLE_STATUS_WIDGET    1           // Enable status widget
#define UI_ENABLE_BREADCRUMB_NAV   1           // Enable breadcrumb navigation
#define UI_ENABLE_WIFI_STATUS      1           // Enable WiFi status indicator
#define UI_ENABLE_BATTERY_STATUS   1           // Enable battery status
#define UI_ENABLE_TIME_DISPLAY     1           // Enable time in status widget

// Status widget components
#define UI_STATUS_SHOW_BATTERY     1           // Show battery in status widget
#define UI_STATUS_SHOW_4G          1           // Show 4G status
#define UI_STATUS_SHOW_WIFI        1           // Show WiFi status  
#define UI_STATUS_SHOW_TIME        1           // Show time

/*********************************************************************************
 *                              MENU CONFIGURATION
 * *******************************************************************************/
// Menu behavior
#define UI_MENU_COLUMNS            2           // Number of button columns
#define UI_MENU_AUTO_SCROLL        0           // Auto-scroll menus
#define UI_MENU_WRAP_NAVIGATION    0           // Wrap around at menu edges

// Folder IDs (safe to modify for organization)
#define UI_SETTINGS_FOLDER_ID      999
#define UI_STATUS_WIDGET_ID        998

/*********************************************************************************
 *                              PERFORMANCE CONFIGURATION
 * *******************************************************************************/
// Performance tuning
#define UI_MAX_MENU_ITEMS          20          // Maximum menu items
#define UI_REFRESH_OPTIMIZATION    1           // Enable refresh optimization
#define UI_CHANGE_DETECTION        1           // Only update when values change
#define UI_MEMORY_SAFETY_CHECKS    1           // Enable null pointer checks
#define UI_FAST_TOUCH_RESPONSE     1           // Enable optimized touch processing
#define UI_REDUCE_WIFI_POLLING     1           // Check WiFi less frequently
#define UI_MENU_REBUILD_CACHE      1           // Cache menu builds to avoid rebuilds

/*********************************************************************************
 *                              DEBUG CONFIGURATION
 * *******************************************************************************/
// Debug and logging
#define UI_DEBUG_ENABLED           0           // Enable debug output
#define UI_LOG_TOUCH_EVENTS        0           // Log touch coordinates
#define UI_LOG_STATUS_UPDATES      0           // Log status widget updates
#define UI_LOG_MENU_NAVIGATION     0           // Log menu navigation

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __UI_CONFIG_H__ */
