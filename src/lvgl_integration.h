/**
 * @file      lvgl_integration.h
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     LVGL integration for T-Deck-Pro e-paper display
 */

#ifndef LVGL_INTEGRATION_H
#define LVGL_INTEGRATION_H

#include <Arduino.h>
#include <lvgl.h>
#include <GxEPD2_BW.h>
#include <TouchDrvCSTXXX.hpp>
#include "utilities.h"

// Display configuration
#define LVGL_DISPLAY_WIDTH  320
#define LVGL_DISPLAY_HEIGHT 240
#define LVGL_BUFFER_SIZE    (LVGL_DISPLAY_WIDTH * LVGL_DISPLAY_HEIGHT / 8)  // 1-bit display

// Forward declarations
class SimpleHardware;

/**
 * @brief LVGL Integration Class
 * 
 * Handles LVGL initialization, display flushing, and input handling
 * for the T-Deck-Pro e-paper display and touch controller
 */
class LVGLIntegration {
private:
    static LVGLIntegration* instance;
    
    // LVGL objects
    lv_disp_t* display;
    lv_indev_t* touch_indev;
    
    // Display buffers
    lv_disp_draw_buf_t draw_buf;
    lv_color_t* buf1;
    lv_color_t* buf2;
    
    // Hardware references
    GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>* epd_display;
    TouchDrvCSTXXX* touch_controller;
    
    // State tracking
    bool initialized;
    bool display_needs_refresh;
    uint32_t last_refresh_time;
    uint32_t refresh_interval_ms;
    
    // Private constructor for singleton
    LVGLIntegration();
    
    // Static callback functions for LVGL
    static void display_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p);
    static void touch_read_cb(lv_indev_drv_t* indev_drv, lv_indev_data_t* data);
    
    // Internal methods
    bool initDisplay();
    bool initTouch();
    void setupMonochromeTheme();

public:
    // Singleton access
    static LVGLIntegration* getInstance();
    
    // Initialization
    bool init(GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>* display,
              TouchDrvCSTXXX* touch);
    void deinit();
    
    // Main loop functions
    void update();
    void forceRefresh();
    
    // Display management
    void setRefreshInterval(uint32_t interval_ms);
    bool isDisplayReady();
    
    // Theme and styling
    void applyMonochromeTheme();
    lv_theme_t* createMonochromeTheme();
    
    // Utility functions
    lv_obj_t* createScreen();
    void setActiveScreen(lv_obj_t* screen);
    
    // Status
    bool isInitialized() { return initialized; }
    uint32_t getLastRefreshTime() { return last_refresh_time; }
};

// Global LVGL integration instance
extern LVGLIntegration* LVGL;

// Convenience macros for LVGL operations
#define LVGL_INIT(display, touch) LVGL->init(display, touch)
#define LVGL_UPDATE() LVGL->update()
#define LVGL_REFRESH() LVGL->forceRefresh()

#endif // LVGL_INTEGRATION_H
