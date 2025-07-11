/**
 * @file      lvgl_integration.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     LVGL integration implementation for T-Deck-Pro e-paper display
 */

#include "lvgl_integration.h"
#include "simple_logger.h"

// Static instance
LVGLIntegration* LVGLIntegration::instance = nullptr;
LVGLIntegration* LVGL = nullptr;

LVGLIntegration::LVGLIntegration() {
    display = nullptr;
    touch_indev = nullptr;
    buf1 = nullptr;
    buf2 = nullptr;
    epd_display = nullptr;
    touch_controller = nullptr;
    initialized = false;
    display_needs_refresh = false;
    last_refresh_time = 0;
    refresh_interval_ms = 5000; // 5 seconds default for e-paper
}

LVGLIntegration* LVGLIntegration::getInstance() {
    if (instance == nullptr) {
        instance = new LVGLIntegration();
        LVGL = instance; // Set global pointer
    }
    return instance;
}

bool LVGLIntegration::init(GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>* display_ptr,
                          TouchDrvCSTXXX* touch_ptr) {
    LOG_INFO("LVGL", "Initializing LVGL integration...");
    
    if (!display_ptr || !touch_ptr) {
        LOG_ERROR("LVGL", "Invalid display or touch pointers");
        return false;
    }
    
    epd_display = display_ptr;
    touch_controller = touch_ptr;
    
    // Initialize LVGL
    lv_init();
    
    // Initialize display
    if (!initDisplay()) {
        LOG_ERROR("LVGL", "Failed to initialize display");
        return false;
    }
    
    // Initialize touch
    if (!initTouch()) {
        LOG_ERROR("LVGL", "Failed to initialize touch");
        return false;
    }
    
    // Setup monochrome theme
    setupMonochromeTheme();
    
    initialized = true;
    LOG_INFO("LVGL", "LVGL integration initialized successfully");
    return true;
}

bool LVGLIntegration::initDisplay() {
    LOG_INFO("LVGL", "Initializing LVGL display driver...");
    
    // Allocate display buffers
    size_t buffer_size = LVGL_BUFFER_SIZE;
    buf1 = (lv_color_t*)ps_malloc(buffer_size * sizeof(lv_color_t));
    buf2 = (lv_color_t*)ps_malloc(buffer_size * sizeof(lv_color_t));
    
    if (!buf1 || !buf2) {
        LOG_ERROR("LVGL", "Failed to allocate display buffers");
        return false;
    }
    
    // Initialize display buffer
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_size);
    
    // Initialize display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    
    disp_drv.hor_res = LVGL_DISPLAY_WIDTH;
    disp_drv.ver_res = LVGL_DISPLAY_HEIGHT;
    disp_drv.flush_cb = display_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1; // E-paper needs full refresh for proper rendering
    
    // Register display driver
    display = lv_disp_drv_register(&disp_drv);
    if (!display) {
        LOG_ERROR("LVGL", "Failed to register display driver");
        return false;
    }
    
    LOG_INFO("LVGL", "LVGL display driver initialized");
    return true;
}

bool LVGLIntegration::initTouch() {
    LOG_INFO("LVGL", "Initializing LVGL touch driver...");

    // Initialize touch input driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);

    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;

    // Register touch input driver
    touch_indev = lv_indev_drv_register(&indev_drv);
    if (!touch_indev) {
        LOG_ERROR("LVGL", "Failed to register touch driver");
        return false;
    }

    // Configure for faster, more responsive touch detection
    if (touch_indev && touch_indev->driver) {
        // Set faster read period for more responsive touch (10ms instead of default 30ms)
        touch_indev->driver->read_timer->period = 10;
        LOG_INFO("LVGL", "Touch read period set to 10ms for better responsiveness");
    }

    LOG_INFO("LVGL", "LVGL touch driver initialized");
    return true;
}

void LVGLIntegration::display_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    LVGLIntegration* lvgl = getInstance();
    
    if (!lvgl->epd_display) {
        lv_disp_flush_ready(disp_drv);
        return;
    }
    
    // For e-paper display, we need to convert LVGL buffer to display format
    // LVGL uses 1-bit color, which should map directly to e-paper
    
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    
    // Set full window for reliable e-paper rendering
    lvgl->epd_display->setFullWindow();
    lvgl->epd_display->firstPage();

    do {
        // Clear the entire screen for proper e-paper rendering
        lvgl->epd_display->fillScreen(GxEPD_WHITE);

        // Draw pixels from LVGL buffer
        for (int32_t y = 0; y < h; y++) {
            for (int32_t x = 0; x < w; x++) {
                lv_color_t pixel = color_p[y * w + x];
                // Convert LVGL 1-bit color to e-paper color
                uint16_t color = (pixel.full == 0) ? GxEPD_BLACK : GxEPD_WHITE;
                lvgl->epd_display->drawPixel(area->x1 + x, area->y1 + y, color);
            }
        }
    } while (lvgl->epd_display->nextPage());
    
    // Mark that display needs refresh
    lvgl->display_needs_refresh = true;
    
    // Tell LVGL that flushing is done
    lv_disp_flush_ready(disp_drv);
}

void LVGLIntegration::touch_read_cb(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    LVGLIntegration* lvgl = getInstance();
    
    if (!lvgl->touch_controller) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    
    if (lvgl->touch_controller->isPressed()) {
        int16_t x, y;
        if (lvgl->touch_controller->getPoint(&x, &y, 1)) {
            data->point.x = x;
            data->point.y = y;
            data->state = LV_INDEV_STATE_PR;
        } else {
            data->state = LV_INDEV_STATE_REL;
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void LVGLIntegration::setupMonochromeTheme() {
    LOG_INFO("LVGL", "Setting up monochrome theme for e-paper display");
    
    // Create and apply monochrome theme
    lv_theme_t* theme = createMonochromeTheme();
    if (theme) {
        lv_disp_set_theme(display, theme);
        LOG_INFO("LVGL", "Monochrome theme applied");
    }
}

lv_theme_t* LVGLIntegration::createMonochromeTheme() {
    // Use the basic theme as a base and modify for monochrome
    lv_theme_t* theme = lv_theme_basic_init(display);
    
    if (theme) {
        // The basic theme should work well for monochrome displays
        // Additional customizations can be added here if needed
        LOG_INFO("LVGL", "Monochrome theme created successfully");
    } else {
        LOG_ERROR("LVGL", "Failed to create monochrome theme");
    }
    
    return theme;
}

void LVGLIntegration::update() {
    if (!initialized) {
        return;
    }
    
    // Handle LVGL tasks
    lv_timer_handler();
    
    // Check if display needs refresh and enough time has passed
    uint32_t current_time = millis();
    if (display_needs_refresh && 
        (current_time - last_refresh_time) >= refresh_interval_ms) {
        
        forceRefresh();
    }
}

void LVGLIntegration::forceRefresh() {
    if (!initialized || !epd_display) {
        return;
    }
    
    // Force a full display refresh for e-paper
    epd_display->display();
    display_needs_refresh = false;
    last_refresh_time = millis();
    
    LOG_DEBUG("LVGL", "Display refreshed");
}

void LVGLIntegration::setRefreshInterval(uint32_t interval_ms) {
    refresh_interval_ms = interval_ms;
    LOG_INFOF("LVGL", "Refresh interval set to %lums", interval_ms);
}

bool LVGLIntegration::isDisplayReady() {
    return initialized && display != nullptr;
}

lv_obj_t* LVGLIntegration::createScreen() {
    if (!initialized) {
        return nullptr;
    }
    
    lv_obj_t* screen = lv_obj_create(NULL);
    if (screen) {
        // Set screen background to white for e-paper
        lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    }
    
    return screen;
}

void LVGLIntegration::setActiveScreen(lv_obj_t* screen) {
    if (initialized && screen) {
        lv_scr_load(screen);
        display_needs_refresh = true;
    }
}

void LVGLIntegration::deinit() {
    if (!initialized) {
        return;
    }
    
    LOG_INFO("LVGL", "Deinitializing LVGL integration...");
    
    // Free buffers
    if (buf1) {
        free(buf1);
        buf1 = nullptr;
    }
    if (buf2) {
        free(buf2);
        buf2 = nullptr;
    }
    
    // Reset state
    display = nullptr;
    touch_indev = nullptr;
    epd_display = nullptr;
    touch_controller = nullptr;
    initialized = false;
    
    LOG_INFO("LVGL", "LVGL integration deinitialized");
}
