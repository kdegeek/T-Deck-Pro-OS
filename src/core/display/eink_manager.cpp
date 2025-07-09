/**
 * @file eink_manager.cpp
 * @brief E-ink Display Manager implementation with advanced burn-in prevention
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "eink_manager.h"
#include "../hal/board_config.h"
#include "../utils/logger.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_timer.h>

// Global instance
EinkManager eink_manager;

// LVGL wrapper functions
void eink_flush_wrapper(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    eink_manager.lvglFlushCallback(disp_drv, area, color_p);
}

void eink_render_start_wrapper(struct _lv_disp_drv_t* disp_drv) {
    eink_manager.lvglRenderStartCallback(disp_drv);
}

EinkManager::EinkManager() :
    display(nullptr),
    partial_refresh_limit(50),      // Force full refresh after 50 partial updates
    full_refresh_interval(300000),  // Full refresh every 5 minutes
    clear_interval(1800000),        // Clear cycle every 30 minutes
    dirty_region_count(0),
    current_policy(EINK_POLICY_ADAPTIVE),
    current_buffer(nullptr),
    previous_buffer(nullptr),
    diff_buffer(nullptr),
    last_update_time(0),
    min_update_interval(100),       // Minimum 100ms between updates
    update_pending(false),
    lvgl_buf1(nullptr),
    lvgl_buf2(nullptr)
{
    // Initialize burn-in prevention data
    memset(&burn_in_data, 0, sizeof(burn_in_data));
    burn_in_data.needs_maintenance = false;
    
    // Initialize dirty regions
    memset(dirty_regions, 0, sizeof(dirty_regions));
}

EinkManager::~EinkManager() {
    if (current_buffer) free(current_buffer);
    if (previous_buffer) free(previous_buffer);
    if (diff_buffer) free(diff_buffer);
    if (lvgl_buf1) free(lvgl_buf1);
    if (lvgl_buf2) free(lvgl_buf2);
}

bool EinkManager::initialize() {
    Logger::info("EinkManager", "Initializing E-ink Display Manager");
    
    // Initialize display hardware
    display = new GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>(
        GxEPD2_310_GDEQ031T10(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY)
    );
    
    if (!display) {
        Logger::error("EinkManager", "Failed to create display instance");
        return false;
    }
    
    // Initialize display with conservative settings
    display->init(115200, true, 2, false);
    display->setRotation(0);
    display->setTextColor(GxEPD_BLACK);
    
    // Initialize buffers
    if (!initializeBuffers()) {
        Logger::error("EinkManager", "Failed to initialize display buffers");
        return false;
    }
    
    // Configure LVGL
    configureLVGL();
    
    // Perform initial clear to establish baseline
    performClearCycle();
    
    Logger::info("EinkManager", "E-ink Display Manager initialized successfully");
    return true;
}

bool EinkManager::initializeBuffers() {
    // Allocate display buffers
    current_buffer = (uint8_t*)ps_malloc(EINK_BUFFER_SIZE);
    previous_buffer = (uint8_t*)ps_malloc(EINK_BUFFER_SIZE);
    diff_buffer = (uint8_t*)ps_malloc(EINK_BUFFER_SIZE);
    
    // Allocate LVGL buffers
    lvgl_buf1 = (lv_color_t*)ps_malloc(sizeof(lv_color_t) * EINK_WIDTH * EINK_HEIGHT);
    lvgl_buf2 = (lv_color_t*)ps_malloc(sizeof(lv_color_t) * EINK_WIDTH * EINK_HEIGHT);
    
    if (!current_buffer || !previous_buffer || !diff_buffer || !lvgl_buf1 || !lvgl_buf2) {
        Logger::error("EinkManager", "Failed to allocate display buffers");
        return false;
    }
    
    // Initialize buffers to white (0xFF for e-ink)
    memset(current_buffer, 0xFF, EINK_BUFFER_SIZE);
    memset(previous_buffer, 0xFF, EINK_BUFFER_SIZE);
    memset(diff_buffer, 0x00, EINK_BUFFER_SIZE);
    
    return true;
}

void EinkManager::configureLVGL() {
    Logger::info("EinkManager", "Configuring LVGL for E-ink display");
    
    // Initialize LVGL draw buffer
    lv_disp_draw_buf_init(&lvgl_draw_buf, lvgl_buf1, lvgl_buf2, EINK_WIDTH * EINK_HEIGHT);
    
    // Initialize display driver
    lv_disp_drv_init(&lvgl_driver);
    lvgl_driver.hor_res = EINK_WIDTH;
    lvgl_driver.ver_res = EINK_HEIGHT;
    lvgl_driver.flush_cb = eink_flush_wrapper;
    lvgl_driver.render_start_cb = eink_render_start_wrapper;
    lvgl_driver.user_data = this;
    lvgl_driver.draw_buf = &lvgl_draw_buf;
    lvgl_driver.full_refresh = 0;  // We'll manage refresh strategy ourselves
    
    // Register the driver
    lv_disp_t* disp = lv_disp_drv_register(&lvgl_driver);
    if (!disp) {
        Logger::error("EinkManager", "Failed to register LVGL display driver");
        return;
    }
    
    // Set as default display
    lv_disp_set_default(disp);
    
    Logger::info("EinkManager", "LVGL configured for E-ink display");
}

void EinkManager::lvglFlushCallback(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    // Convert LVGL color buffer to E-ink format
    convertLvglToEink(color_p, current_buffer, area);
    
    // Update pixel usage tracking
    updatePixelUsageMap(area);
    
    // Determine refresh strategy
    EinkRefreshMode refresh_mode = EINK_REFRESH_PARTIAL;
    
    if (shouldPerformFullRefresh()) {
        refresh_mode = EINK_REFRESH_FULL;
        Logger::debug("EinkManager", "Performing full refresh to prevent burn-in");
    }
    
    // Schedule the display update
    scheduleUpdate(area, refresh_mode);
    
    // Mark flush as ready
    lv_disp_flush_ready(disp_drv);
}

void EinkManager::lvglRenderStartCallback(struct _lv_disp_drv_t* disp_drv) {
    // Called when LVGL starts rendering
    // We can use this to batch updates or prepare for refresh
    update_pending = true;
}

void EinkManager::convertLvglToEink(const lv_color_t* color_p, uint8_t* eink_buf, const lv_area_t* area) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    // Convert LVGL 1-bit color to E-ink format
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x += 8) {
            uint8_t byte_val = 0;
            
            // Pack 8 pixels into one byte
            for (int bit = 0; bit < 8 && (x + bit) < w; bit++) {
                uint32_t pixel_idx = y * w + x + bit;
                if (pixel_idx < w * h) {
                    // LVGL uses 0 for black, 1 for white
                    // E-ink uses 0 for black, 1 for white (same)
                    if ((color_p + pixel_idx)->full) {
                        byte_val |= (1 << (7 - bit));
                    }
                }
            }
            
            // Calculate buffer position
            uint32_t buf_x = area->x1 + x;
            uint32_t buf_y = area->y1 + y;
            uint32_t buf_idx = (buf_y * EINK_WIDTH + buf_x) / 8;
            
            if (buf_idx < EINK_BUFFER_SIZE) {
                eink_buf[buf_idx] = byte_val;
            }
        }
    }
}

bool EinkManager::shouldPerformFullRefresh() {
    uint32_t current_time = esp_timer_get_time() / 1000; // Convert to milliseconds
    
    // Check partial refresh count limit
    if (burn_in_data.partial_refresh_count >= partial_refresh_limit) {
        Logger::debug("EinkManager", "Full refresh triggered by partial count limit");
        return true;
    }
    
    // Check time-based full refresh interval
    if (current_time - burn_in_data.last_full_refresh_time >= full_refresh_interval) {
        Logger::debug("EinkManager", "Full refresh triggered by time interval");
        return true;
    }
    
    // Check if maintenance cycle is needed
    if (burn_in_data.needs_maintenance) {
        Logger::debug("EinkManager", "Full refresh triggered by maintenance requirement");
        return true;
    }
    
    return false;
}

void EinkManager::calculateDirtyRegions(const lv_area_t* area) {
    // Find or create a dirty region for this area
    bool region_found = false;
    
    for (uint8_t i = 0; i < dirty_region_count; i++) {
        EinkRegion* region = &dirty_regions[i];
        
        // Check if this area overlaps with existing dirty region
        if (area->x1 <= region->x + region->width && area->x2 >= region->x &&
            area->y1 <= region->y + region->height && area->y2 >= region->y) {
            
            // Expand the existing region to include the new area
            int16_t new_x1 = min((int16_t)region->x, (int16_t)area->x1);
            int16_t new_y1 = min((int16_t)region->y, (int16_t)area->y1);
            int16_t new_x2 = max((int16_t)(region->x + region->width - 1), (int16_t)area->x2);
            int16_t new_y2 = max((int16_t)(region->y + region->height - 1), (int16_t)area->y2);
            
            region->x = new_x1;
            region->y = new_y1;
            region->width = new_x2 - new_x1 + 1;
            region->height = new_y2 - new_y1 + 1;
            region->dirty = true;
            region->update_count++;
            
            region_found = true;
            break;
        }
    }
    
    // If no overlapping region found and we have space, create a new one
    if (!region_found && dirty_region_count < 16) {
        EinkRegion* new_region = &dirty_regions[dirty_region_count];
        new_region->x = area->x1;
        new_region->y = area->y1;
        new_region->width = area->x2 - area->x1 + 1;
        new_region->height = area->y2 - area->y1 + 1;
        new_region->dirty = true;
        new_region->last_update = esp_timer_get_time() / 1000;
        new_region->update_count = 1;
        
        dirty_region_count++;
    }
}

void EinkManager::optimizeRefreshRegion(lv_area_t* area) {
    // Ensure the area is within display bounds
    if (area->x1 < 0) area->x1 = 0;
    if (area->y1 < 0) area->y1 = 0;
    if (area->x2 >= EINK_WIDTH) area->x2 = EINK_WIDTH - 1;
    if (area->y2 >= EINK_HEIGHT) area->y2 = EINK_HEIGHT - 1;
    
    // Align to byte boundaries for better performance
    area->x1 = (area->x1 / 8) * 8;
    area->x2 = ((area->x2 / 8) + 1) * 8 - 1;
    if (area->x2 >= EINK_WIDTH) area->x2 = EINK_WIDTH - 1;
    
    // Ensure minimum update size to avoid too many small updates
    int16_t min_width = 16;
    int16_t min_height = 16;
    
    if ((area->x2 - area->x1 + 1) < min_width) {
        int16_t expand = (min_width - (area->x2 - area->x1 + 1)) / 2;
        area->x1 = max(0, (int)(area->x1 - expand));
        area->x2 = min(EINK_WIDTH - 1, (int)(area->x2 + expand));
    }
    
    if ((area->y2 - area->y1 + 1) < min_height) {
        int16_t expand = (min_height - (area->y2 - area->y1 + 1)) / 2;
        area->y1 = max(0, (int)(area->y1 - expand));
        area->y2 = min(EINK_HEIGHT - 1, (int)(area->y2 + expand));
    }
}

void EinkManager::scheduleUpdate(const lv_area_t* area, EinkRefreshMode mode) {
    // Optimize the refresh area to minimize unnecessary updates
    lv_area_t optimized_area = *area;
    optimizeRefreshRegion(&optimized_area);
    
    // Check minimum update interval
    uint32_t current_time = esp_timer_get_time() / 1000;
    if (current_time - last_update_time < min_update_interval) {
        // Too soon, batch this update
        calculateDirtyRegions(&optimized_area);
        return;
    }
    
    // Perform the update
    flushDisplay(&optimized_area, current_buffer, mode);
    last_update_time = current_time;
}

void EinkManager::flushDisplay(const lv_area_t* area, const uint8_t* buffer, EinkRefreshMode mode) {
    if (!display) return;
    
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    
    switch (mode) {
        case EINK_REFRESH_PARTIAL:
            display->setPartialWindow(area->x1, area->y1, w, h);
            display->firstPage();
            do {
                display->drawInvertedBitmap(area->x1, area->y1, buffer, w, h, GxEPD_BLACK);
            } while (display->nextPage());
            burn_in_data.partial_refresh_count++;
            Logger::debug("EinkManager", "Partial refresh completed");
            break;
            
        case EINK_REFRESH_FULL:
            display->setFullWindow();
            display->firstPage();
            do {
                display->drawInvertedBitmap(0, 0, buffer, EINK_WIDTH, EINK_HEIGHT, GxEPD_BLACK);
            } while (display->nextPage());
            burn_in_data.partial_refresh_count = 0;
            burn_in_data.last_full_refresh_time = esp_timer_get_time() / 1000;
            burn_in_data.needs_maintenance = false;
            Logger::debug("EinkManager", "Full refresh completed");
            break;
            
        case EINK_REFRESH_CLEAR:
            performClearCycle();
            break;
            
        case EINK_REFRESH_DEEP_CLEAN:
            performDeepClean();
            break;
    }
    
    display->hibernate();
}

void EinkManager::processScheduledUpdates() {
    // Process any pending dirty regions
    if (dirty_region_count == 0) {
        return;
    }
    
    uint32_t current_time = esp_timer_get_time() / 1000;
    
    for (uint8_t i = 0; i < dirty_region_count; i++) {
        EinkRegion* region = &dirty_regions[i];
        
        if (region->dirty && (current_time - region->last_update) >= min_update_interval) {
            // Create area from region
            lv_area_t area;
            area.x1 = region->x;
            area.y1 = region->y;
            area.x2 = region->x + region->width - 1;
            area.y2 = region->y + region->height - 1;
            
            // Determine refresh mode based on update count
            EinkRefreshMode mode = EINK_REFRESH_PARTIAL;
            if (region->update_count > 10 || shouldPerformFullRefresh()) {
                mode = EINK_REFRESH_FULL;
            }
            
            // Perform the update
            flushDisplay(&area, current_buffer, mode);
            
            // Mark as processed
            region->dirty = false;
            region->last_update = current_time;
            
            // Remove processed region by shifting array
            for (uint8_t j = i; j < dirty_region_count - 1; j++) {
                dirty_regions[j] = dirty_regions[j + 1];
            }
            dirty_region_count--;
            i--; // Adjust index after removal
        }
    }
}

void EinkManager::performClearCycle() {
    Logger::info("EinkManager", "Performing E-ink clear cycle");
    
    display->setFullWindow();
    
    // Clear to white
    display->firstPage();
    do {
        display->fillScreen(GxEPD_WHITE);
    } while (display->nextPage());
    
    delay(100);
    
    // Clear to black
    display->firstPage();
    do {
        display->fillScreen(GxEPD_BLACK);
    } while (display->nextPage());
    
    delay(100);
    
    // Final clear to white
    display->firstPage();
    do {
        display->fillScreen(GxEPD_WHITE);
    } while (display->nextPage());
    
    // Reset burn-in tracking
    burn_in_data.last_clear_time = esp_timer_get_time() / 1000;
    burn_in_data.partial_refresh_count = 0;
    memset(burn_in_data.pixel_usage_map, 0, sizeof(burn_in_data.pixel_usage_map));
    
    display->hibernate();
    Logger::info("EinkManager", "Clear cycle completed");
}

void EinkManager::performDeepClean() {
    Logger::info("EinkManager", "Performing E-ink deep clean cycle");
    
    // Perform multiple clear cycles for deep cleaning
    for (int i = 0; i < 3; i++) {
        performClearCycle();
        delay(500);
    }
    
    Logger::info("EinkManager", "Deep clean cycle completed");
}

void EinkManager::updatePixelUsageMap(const lv_area_t* area) {
    // Track which pixels are being used to detect potential burn-in areas
    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x += 8) {
            if (y < EINK_HEIGHT && x < EINK_WIDTH) {
                burn_in_data.pixel_usage_map[y][x/8]++;
                
                // Check for excessive usage
                if (burn_in_data.pixel_usage_map[y][x/8] > 1000) {
                    burn_in_data.needs_maintenance = true;
                }
            }
        }
    }
}

void EinkManager::checkBurnInPrevention() {
    uint32_t current_time = esp_timer_get_time() / 1000;
    
    // Check if clear cycle is needed
    if (current_time - burn_in_data.last_clear_time >= clear_interval) {
        Logger::info("EinkManager", "Scheduling clear cycle for burn-in prevention");
        performClearCycle();
    }
    
    // Check pixel usage patterns
    float usage_percentage = getPixelUsagePercentage();
    if (usage_percentage > 80.0f) {
        Logger::warning("EinkManager", "High pixel usage detected, scheduling maintenance");
        burn_in_data.needs_maintenance = true;
    }
}

float EinkManager::getPixelUsagePercentage() {
    uint32_t total_usage = 0;
    uint32_t max_possible = EINK_HEIGHT * (EINK_WIDTH / 8) * 1000; // Max usage per pixel
    
    for (int y = 0; y < EINK_HEIGHT; y++) {
        for (int x = 0; x < EINK_WIDTH / 8; x++) {
            total_usage += burn_in_data.pixel_usage_map[y][x];
        }
    }
    
    return (float)total_usage / max_possible * 100.0f;
}

void EinkManager::enterSleepMode() {
    if (display) {
        display->hibernate();
    }
    Logger::debug("EinkManager", "E-ink display entered sleep mode");
}

void EinkManager::exitSleepMode() {
    // Display will wake up automatically on next update
    Logger::debug("EinkManager", "E-ink display exiting sleep mode");
}

// Global initialization function
void eink_init() {
    if (!eink_manager.initialize()) {
        Logger::error("EinkManager", "Failed to initialize E-ink manager");
    }
}

// Task handler for periodic maintenance
void eink_task_handler() {
    eink_manager.checkBurnInPrevention();
    eink_manager.processScheduledUpdates();
}

// FreeRTOS task for maintenance
void eink_maintenance_task(void* parameter) {
    const TickType_t xDelay = pdMS_TO_TICKS(60000); // Run every minute
    
    while (1) {
        eink_task_handler();
        vTaskDelay(xDelay);
    }
}