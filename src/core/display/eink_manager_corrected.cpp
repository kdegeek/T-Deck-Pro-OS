/**
 * @file eink_manager_corrected.cpp
 * @brief T-Deck-Pro E-Ink Display Manager - GDEQ031T10 Implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Handles E-paper display with LVGL integration and power optimization
 */

#include "eink_manager_corrected.h"
#include "../utils/logger.h"
#include <GxEPD2_GFX.h>
#include <GxEPD2_310_GDEQ031T10.h>
#include <lvgl.h>

// ===== GLOBAL E-INK MANAGER INSTANCE =====
EinkManager* g_eink_manager = nullptr;

// ===== E-INK MANAGER IMPLEMENTATION =====

EinkManager::EinkManager() : initialized_(false), display_(nullptr), 
                             current_mode_(DisplayMode::FULL), 
                             last_update_time_(0), update_interval_(5000),
                             brightness_(50), contrast_(100), 
                             power_save_enabled_(true), auto_refresh_(false) {
    
    // Initialize display pointer
    display_ = new GxEPD2_310_GDEQ031T10(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY);
    
    // Initialize LVGL display buffer
    display_buffer_ = nullptr;
    display_buffer_size_ = 0;
}

EinkManager::~EinkManager() {
    if (initialized_) {
        cleanup();
    }
    
    if (display_) {
        delete display_;
        display_ = nullptr;
    }
    
    if (display_buffer_) {
        free(display_buffer_);
        display_buffer_ = nullptr;
    }
}

bool EinkManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    logInfo("EINK", "Initializing E-ink display manager");
    
    // Initialize SPI for E-paper display
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
    
    // Initialize display
    if (!initDisplay()) {
        logError("EINK", "Failed to initialize display");
        return false;
    }
    
    // Initialize LVGL
    if (!initLVGL()) {
        logError("EINK", "Failed to initialize LVGL");
        return false;
    }
    
    // Set initial display mode
    setDisplayMode(DisplayMode::FULL);
    
    initialized_ = true;
    
    logInfo("EINK", "E-ink display manager initialized successfully");
    return true;
}

bool EinkManager::initDisplay() {
    if (!display_) {
        logError("EINK", "Display object is null");
        return false;
    }
    
    // Initialize the display
    display_->init(115200);
    
    // Set rotation
    display_->setRotation(1); // Landscape mode
    
    // Set power saving mode
    if (power_save_enabled_) {
        display_->powerOff();
    }
    
    logInfo("EINK", "Display hardware initialized");
    return true;
}

bool EinkManager::initLVGL() {
    // Initialize LVGL
    lv_init();
    
    // Allocate display buffer (320x240 / 8 = 9600 bytes for 1-bit depth)
    display_buffer_size_ = (DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8;
    display_buffer_ = (uint8_t*)ps_malloc(display_buffer_size_);
    
    if (!display_buffer_) {
        logError("EINK", "Failed to allocate display buffer");
        return false;
    }
    
    // Initialize LVGL display driver
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, display_buffer_, nullptr, display_buffer_size_);
    
    // Initialize display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = eink_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1; // Full refresh for E-paper
    lv_disp_drv_register(&disp_drv);
    
    // Set refresh period for E-paper (5 seconds)
    lv_disp_set_refresh_period(lv_disp_get_default(), 5000);
    
    logInfo("EINK", "LVGL initialized with buffer size: " + String(display_buffer_size_));
    return true;
}

void EinkManager::process() {
    if (!initialized_) {
        return;
    }
    
    // Handle LVGL tasks
    lv_timer_handler();
    
    // Auto-refresh if enabled
    if (auto_refresh_ && (millis() - last_update_time_) > update_interval_) {
        refresh();
    }
}

void EinkManager::cleanup() {
    if (!initialized_) {
        return;
    }
    
    logInfo("EINK", "Cleaning up E-ink display manager");
    
    // Power off display
    if (display_) {
        display_->powerOff();
    }
    
    initialized_ = false;
}

void EinkManager::setDisplayMode(DisplayMode mode) {
    current_mode_ = mode;
    
    switch (mode) {
        case DisplayMode::FULL:
            logInfo("EINK", "Display mode set to FULL");
            break;
        case DisplayMode::PARTIAL:
            logInfo("EINK", "Display mode set to PARTIAL");
            break;
        case DisplayMode::FAST:
            logInfo("EINK", "Display mode set to FAST");
            break;
    }
}

void EinkManager::setBrightness(uint8_t brightness) {
    brightness_ = brightness;
    logInfo("EINK", "Brightness set to: " + String(brightness));
}

void EinkManager::setContrast(uint8_t contrast) {
    contrast_ = contrast;
    logInfo("EINK", "Contrast set to: " + String(contrast));
}

void EinkManager::setPowerSave(bool enabled) {
    power_save_enabled_ = enabled;
    
    if (display_) {
        if (enabled) {
            display_->powerOff();
        } else {
            display_->powerOn();
        }
    }
    
    logInfo("EINK", "Power save " + String(enabled ? "enabled" : "disabled"));
}

void EinkManager::setAutoRefresh(bool enabled) {
    auto_refresh_ = enabled;
    logInfo("EINK", "Auto refresh " + String(enabled ? "enabled" : "disabled"));
}

void EinkManager::setUpdateInterval(uint32_t interval) {
    update_interval_ = interval;
    logInfo("EINK", "Update interval set to: " + String(interval) + "ms");
}

void EinkManager::refresh() {
    if (!initialized_ || !display_) {
        return;
    }
    
    logDebug("EINK", "Refreshing display");
    
    // Trigger LVGL refresh
    lv_disp_t* disp = lv_disp_get_default();
    if (disp) {
        lv_disp_flush(disp);
    }
    
    last_update_time_ = millis();
}

void EinkManager::clear() {
    if (!initialized_ || !display_) {
        return;
    }
    
    logInfo("EINK", "Clearing display");
    
    // Clear LVGL screen
    lv_obj_clean(lv_scr_act());
    
    // Clear display buffer
    if (display_buffer_) {
        memset(display_buffer_, 0, display_buffer_size_);
    }
    
    // Refresh display
    refresh();
}

void EinkManager::drawText(uint16_t x, uint16_t y, const String& text, uint16_t color, uint8_t size) {
    if (!initialized_) {
        return;
    }
    
    // Create LVGL label
    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, text.c_str());
    lv_obj_set_pos(label, x, y);
    
    // Set font size
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    
    // Set color
    lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
    
    logDebug("EINK", "Drew text: " + text + " at (" + String(x) + "," + String(y) + ")");
}

void EinkManager::drawImage(uint16_t x, uint16_t y, const uint8_t* image_data, uint16_t width, uint16_t height) {
    if (!initialized_) {
        return;
    }
    
    // Create LVGL image object
    lv_obj_t* img = lv_img_create(lv_scr_act());
    
    // Set image data
    lv_img_set_src(img, image_data);
    lv_obj_set_pos(img, x, y);
    
    logDebug("EINK", "Drew image at (" + String(x) + "," + String(y) + ")");
}

void EinkManager::drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color, bool filled) {
    if (!initialized_) {
        return;
    }
    
    // Create LVGL rectangle
    lv_obj_t* rect = lv_obj_create(lv_scr_act());
    lv_obj_set_size(rect, width, height);
    lv_obj_set_pos(rect, x, y);
    
    // Set color
    lv_obj_set_style_bg_color(rect, lv_color_hex(color), LV_PART_MAIN);
    
    // Set fill
    if (filled) {
        lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_opa(rect, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_color(rect, lv_color_hex(color), LV_PART_MAIN);
        lv_obj_set_style_border_width(rect, 1, LV_PART_MAIN);
    }
    
    logDebug("EINK", "Drew rectangle at (" + String(x) + "," + String(y) + ")");
}

void EinkManager::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color, uint8_t width) {
    if (!initialized_) {
        return;
    }
    
    // Create LVGL line
    lv_obj_t* line = lv_line_create(lv_scr_act());
    
    // Set line points
    static lv_point_t points[2];
    points[0].x = x1;
    points[0].y = y1;
    points[1].x = x2;
    points[1].y = y2;
    lv_line_set_points(line, points, 2);
    
    // Set color and width
    lv_obj_set_style_line_color(line, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_line_width(line, width, LV_PART_MAIN);
    
    logDebug("EINK", "Drew line from (" + String(x1) + "," + String(y1) + ") to (" + String(x2) + "," + String(y2) + ")");
}

void EinkManager::drawCircle(uint16_t x, uint16_t y, uint16_t radius, uint16_t color, bool filled) {
    if (!initialized_) {
        return;
    }
    
    // Create LVGL circle
    lv_obj_t* circle = lv_obj_create(lv_scr_act());
    lv_obj_set_size(circle, radius * 2, radius * 2);
    lv_obj_set_pos(circle, x - radius, y - radius);
    
    // Set color
    lv_obj_set_style_bg_color(circle, lv_color_hex(color), LV_PART_MAIN);
    
    // Set fill
    if (filled) {
        lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_color(circle, lv_color_hex(color), LV_PART_MAIN);
        lv_obj_set_style_border_width(circle, 1, LV_PART_MAIN);
    }
    
    logDebug("EINK", "Drew circle at (" + String(x) + "," + String(y) + ") radius: " + String(radius));
}

DisplayMode EinkManager::getDisplayMode() const {
    return current_mode_;
}

uint8_t EinkManager::getBrightness() const {
    return brightness_;
}

uint8_t EinkManager::getContrast() const {
    return contrast_;
}

bool EinkManager::isPowerSaveEnabled() const {
    return power_save_enabled_;
}

bool EinkManager::isAutoRefreshEnabled() const {
    return auto_refresh_;
}

uint32_t EinkManager::getUpdateInterval() const {
    return update_interval_;
}

String EinkManager::getDisplayStatus() {
    DynamicJsonDocument doc(1024);
    
    doc["initialized"] = initialized_;
    doc["display_mode"] = (int)current_mode_;
    doc["brightness"] = brightness_;
    doc["contrast"] = contrast_;
    doc["power_save"] = power_save_enabled_;
    doc["auto_refresh"] = auto_refresh_;
    doc["update_interval"] = update_interval_;
    doc["buffer_size"] = display_buffer_size_;
    doc["last_update"] = last_update_time_;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ===== LVGL CALLBACK FUNCTIONS =====

void eink_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    EinkManager* manager = getEinkManager();
    if (!manager || !manager->display_) {
        lv_disp_flush_ready(disp_drv);
        return;
    }
    
    // Convert LVGL colors to E-paper format
    uint16_t x = area->x1;
    uint16_t y = area->y1;
    uint16_t w = area->x2 - area->x1 + 1;
    uint16_t h = area->y2 - area->y1 + 1;
    
    // Update E-paper display
    manager->display_->setPartialWindow(x, y, w, h);
    manager->display_->firstPage();
    
    do {
        // Convert color data to E-paper format
        for (uint16_t i = 0; i < w * h; i++) {
            // Convert LVGL color to grayscale
            uint8_t gray = (color_p[i].full >> 8) & 0xFF;
            manager->display_->drawPixel(x + (i % w), y + (i / w), gray);
        }
    } while (manager->display_->nextPage());
    
    lv_disp_flush_ready(disp_drv);
}

// ===== GLOBAL E-INK MANAGER FUNCTIONS =====

bool initializeEinkManager() {
    if (g_eink_manager) {
        return true;
    }
    
    g_eink_manager = new EinkManager();
    if (!g_eink_manager) {
        return false;
    }
    
    return g_eink_manager->initialize();
}

EinkManager* getEinkManager() {
    return g_eink_manager;
} 