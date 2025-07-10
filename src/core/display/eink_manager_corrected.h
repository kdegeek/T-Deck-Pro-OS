/**
 * @file eink_manager_corrected.h
 * @brief T-Deck-Pro E-Ink Display Manager - GDEQ031T10 Optimized
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Optimized for E-paper display with slow refresh and power management
 */

#ifndef EINK_MANAGER_CORRECTED_H
#define EINK_MANAGER_CORRECTED_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <lvgl.h>
#include <SPI.h>

#include "../hal/board_config_corrected.h"

// ===== DISPLAY CONFIGURATION =====
#define EINK_DISPLAY_WIDTH 240
#define EINK_DISPLAY_HEIGHT 320
#define EINK_BUFFER_SIZE (EINK_DISPLAY_WIDTH * EINK_DISPLAY_HEIGHT / 8)

// ===== REFRESH MODES =====
enum class RefreshMode {
    FULL,       // Full screen refresh (slow, high quality)
    PARTIAL,    // Partial refresh (fast, lower quality)
    FAST        // Fast refresh (very fast, lowest quality)
};

// ===== DISPLAY STATES =====
enum class DisplayState {
    INIT,
    READY,
    REFRESHING,
    SLEEPING,
    ERROR
};

/**
 * @brief E-Ink display manager for T-Deck-Pro
 * @note Optimized for GDEQ031T10 with power management
 */
class EinkManager {
public:
    EinkManager();
    ~EinkManager();
    
    /**
     * @brief Initialize E-ink display
     * @return true if successful
     */
    bool init();
    
    /**
     * @brief Check if display is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Get display state
     * @return Current display state
     */
    DisplayState getState() const { return current_state_; }
    
    /**
     * @brief Show boot splash screen
     * @param title Main title
     * @param subtitle Subtitle text
     */
    void showBootSplash(const String& title, const String& subtitle = "");
    
    /**
     * @brief Show system ready screen
     */
    void showSystemReady();
    
    /**
     * @brief Show error screen
     * @param title Error title
     * @param message Error message
     */
    void showError(const String& title, const String& message);
    
    /**
     * @brief Show low battery warning
     */
    void showLowBatteryWarning();
    
    /**
     * @brief Update battery status display
     * @param voltage Battery voltage
     */
    void updateBatteryStatus(float voltage);
    
    /**
     * @brief Refresh display with current content
     * @param mode Refresh mode
     */
    void refresh(RefreshMode mode = RefreshMode::PARTIAL);
    
    /**
     * @brief Clear display
     */
    void clear();
    
    /**
     * @brief Put display to sleep
     */
    void sleep();
    
    /**
     * @brief Wake display from sleep
     */
    void wake();
    
    /**
     * @brief Set display rotation
     * @param rotation Rotation value (0-3)
     */
    void setRotation(uint8_t rotation);
    
    /**
     * @brief Get display width
     * @return Width in pixels
     */
    uint16_t getWidth() const { return EINK_DISPLAY_WIDTH; }
    
    /**
     * @brief Get display height
     * @return Height in pixels
     */
    uint16_t getHeight() const { return EINK_DISPLAY_HEIGHT; }
    
    /**
     * @brief Check if display is busy
     * @return true if busy
     */
    bool isBusy() const;
    
    /**
     * @brief Wait for display to be ready
     * @param timeout_ms Timeout in milliseconds
     * @return true if ready
     */
    bool waitForReady(uint32_t timeout_ms = 5000);
    
    /**
     * @brief Get refresh time for mode
     * @param mode Refresh mode
     * @return Refresh time in milliseconds
     */
    uint32_t getRefreshTime(RefreshMode mode) const;
    
    /**
     * @brief Set display brightness
     * @param brightness Brightness level (0-255)
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * @brief Get current brightness
     * @return Brightness level
     */
    uint8_t getBrightness() const { return brightness_; }
    
    /**
     * @brief Enable/disable power saving
     * @param enabled true to enable
     */
    void setPowerSaving(bool enabled);
    
    /**
     * @brief Check if power saving is enabled
     * @return true if enabled
     */
    bool isPowerSavingEnabled() const { return power_saving_enabled_; }
    
    /**
     * @brief Get display statistics
     * @return Statistics JSON string
     */
    String getStatistics();
    
    /**
     * @brief Reset display statistics
     */
    void resetStatistics();

private:
    // ===== DISPLAY OPERATIONS =====
    
    /**
     * @brief Initialize display hardware
     * @return true if successful
     */
    bool initHardware();
    
    /**
     * @brief Initialize LVGL for display
     * @return true if successful
     */
    bool initLVGL();
    
    /**
     * @brief Initialize display buffers
     * @return true if successful
     */
    bool initBuffers();
    
    /**
     * @brief Configure display settings
     * @return true if successful
     */
    bool configureDisplay();
    
    /**
     * @brief Perform full screen refresh
     * @return true if successful
     */
    bool performFullRefresh();
    
    /**
     * @brief Perform partial refresh
     * @return true if successful
     */
    bool performPartialRefresh();
    
    /**
     * @brief Perform fast refresh
     * @return true if successful
     */
    bool performFastRefresh();
    
    /**
     * @brief Update display content
     * @param mode Refresh mode
     * @return true if successful
     */
    bool updateDisplay(RefreshMode mode);
    
    /**
     * @brief Draw text on display
     * @param text Text to draw
     * @param x X position
     * @param y Y position
     * @param size Font size
     */
    void drawText(const String& text, uint16_t x, uint16_t y, uint8_t size = 2);
    
    /**
     * @brief Draw rectangle on display
     * @param x X position
     * @param y Y position
     * @param width Width
     * @param height Height
     * @param color Color (0=white, 1=black)
     */
    void drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color = 1);
    
    /**
     * @brief Fill rectangle on display
     * @param x X position
     * @param y Y position
     * @param width Width
     * @param height Height
     * @param color Color (0=white, 1=black)
     */
    void fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color = 1);
    
    /**
     * @brief Draw line on display
     * @param x1 Start X
     * @param y1 Start Y
     * @param x2 End X
     * @param y2 End Y
     * @param color Color (0=white, 1=black)
     */
    void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color = 1);
    
    /**
     * @brief Set display state
     * @param state New state
     */
    void setState(DisplayState state);
    
    /**
     * @brief Log display event
     * @param event Event description
     */
    void logEvent(const String& event);
    
    /**
     * @brief Update display statistics
     * @param mode Refresh mode used
     * @param duration_ms Duration in milliseconds
     */
    void updateStatistics(RefreshMode mode, uint32_t duration_ms);

private:
    // ===== MEMBER VARIABLES =====
    bool initialized_;
    DisplayState current_state_;
    uint8_t rotation_;
    uint8_t brightness_;
    bool power_saving_enabled_;
    
    // Display buffers
    uint8_t* display_buffer_;
    uint8_t* previous_buffer_;
    
    // Statistics
    uint32_t total_refreshes_;
    uint32_t full_refreshes_;
    uint32_t partial_refreshes_;
    uint32_t fast_refreshes_;
    uint32_t total_refresh_time_;
    uint32_t last_refresh_time_;
    
    // Hardware objects
    GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>* display_;
    
    // LVGL objects
    lv_disp_draw_buf_t draw_buf_;
    lv_disp_drv_t disp_drv_;
    lv_color_t* lvgl_buf1_;
    lv_color_t* lvgl_buf2_;
    
    // Timing
    uint32_t last_refresh_;
    uint32_t refresh_interval_;
};

// ===== GLOBAL DISPLAY MANAGER INSTANCE =====
extern EinkManager* g_display_manager;

// ===== DISPLAY UTILITY FUNCTIONS =====

/**
 * @brief Initialize global display manager
 * @return true if successful
 */
bool initializeDisplayManager();

/**
 * @brief Get global display manager instance
 * @return Display manager pointer
 */
EinkManager* getDisplayManager();

/**
 * @brief LVGL display flush callback
 * @param disp_drv Display driver
 * @param area Area to flush
 * @param color_p Color data
 */
void lvgl_display_flush(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p);

/**
 * @brief LVGL display render start callback
 * @param disp_drv Display driver
 */
void lvgl_display_render_start(lv_disp_drv_t* disp_drv);

#endif // EINK_MANAGER_CORRECTED_H 