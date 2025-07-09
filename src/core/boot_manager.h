/**
 * @file boot_manager.h
 * @brief T-Deck-Pro Boot Manager - System startup and initialization
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef BOOT_MANAGER_H
#define BOOT_MANAGER_H

#include <Arduino.h>
#include "../config/os_config.h"

/**
 * @brief Boot stages for tracking startup progress
 */
enum class BootStage {
    POWER_ON,
    HARDWARE_INIT,
    STORAGE_INIT,
    DISPLAY_INIT,
    CONNECTIVITY_INIT,
    SERVICES_INIT,
    COMPLETE,
    ERROR
};

/**
 * @brief Boot manager handles system startup sequence
 */
class BootManager {
public:
    BootManager();
    ~BootManager();
    
    /**
     * @brief Initialize the boot manager
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Set current boot stage
     * @param stage Current boot stage
     * @param message Optional status message
     */
    void set_boot_stage(BootStage stage, const String& message = "");
    
    /**
     * @brief Get current boot stage
     * @return Current boot stage
     */
    BootStage get_boot_stage() const;
    
    /**
     * @brief Show error message on display
     * @param error_message Error message to display
     */
    void show_error(const String& error_message);
    
    /**
     * @brief Show boot progress on display
     * @param stage Current stage
     * @param message Status message
     * @param progress Progress percentage (0-100)
     */
    void show_boot_progress(BootStage stage, const String& message, int progress = -1);
    
    /**
     * @brief Check if boot completed successfully
     * @return true if boot completed
     */
    bool is_boot_complete() const;
    
    /**
     * @brief Get boot duration in milliseconds
     * @return Boot duration
     */
    uint32_t get_boot_duration() const;
    
    /**
     * @brief Get boot statistics
     * @return JSON string with boot statistics
     */
    String get_boot_statistics() const;
    
    /**
     * @brief Check system health and decide if restart is needed
     * @return true if system is healthy
     */
    bool check_system_health();
    
    /**
     * @brief Perform emergency restart
     * @param reason Reason for restart
     */
    void emergency_restart(const String& reason);
    
    /**
     * @brief Show splash screen with OS info
     */
    void show_splash_screen();
    
private:
    /**
     * @brief Initialize basic display for boot messages
     * @return true if successful
     */
    bool init_basic_display();
    
    /**
     * @brief Clear display and prepare for text
     */
    void clear_display();
    
    /**
     * @brief Draw text on display
     * @param text Text to draw
     * @param x X position
     * @param y Y position
     * @param size Text size
     */
    void draw_text(const String& text, int x, int y, int size = 1);
    
    /**
     * @brief Draw progress bar
     * @param x X position
     * @param y Y position
     * @param width Width of progress bar
     * @param height Height of progress bar
     * @param progress Progress percentage (0-100)
     */
    void draw_progress_bar(int x, int y, int width, int height, int progress);
    
    /**
     * @brief Update display with current content
     */
    void update_display();
    
    /**
     * @brief Get stage name as string
     * @param stage Boot stage
     * @return Stage name
     */
    String get_stage_name(BootStage stage) const;
    
    // Boot state
    BootStage current_stage;
    String current_message;
    uint32_t boot_start_time;
    uint32_t boot_complete_time;
    bool boot_complete;
    bool display_available;
    
    // Boot statistics
    struct BootStats {
        uint32_t power_on_time;
        uint32_t hardware_init_time;
        uint32_t storage_init_time;
        uint32_t display_init_time;
        uint32_t connectivity_init_time;
        uint32_t services_init_time;
        uint32_t total_boot_time;
        int restart_count;
        String last_error;
    } boot_stats;
    
    // Display state for boot messages
    bool basic_display_initialized;
    
    static const int DISPLAY_WIDTH = LCD_HOR_SIZE;
    static const int DISPLAY_HEIGHT = LCD_VER_SIZE;
    static const int TEXT_SIZE_SMALL = 1;
    static const int TEXT_SIZE_MEDIUM = 2;
    static const int TEXT_SIZE_LARGE = 3;
};

#endif // BOOT_MANAGER_H
