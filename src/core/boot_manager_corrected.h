/**
 * @file boot_manager_corrected.h
 * @brief T-Deck-Pro Boot Manager - System startup and initialization (CORRECTED)
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Corrected boot manager with hardware driver integration and display feedback
 */

#ifndef BOOT_MANAGER_CORRECTED_H
#define BOOT_MANAGER_CORRECTED_H

#include <Arduino.h>
#include "config/os_config_corrected.h"
#include "../drivers/modem_4g_driver.h"

// Forward declarations
class HardwareManager;
class EinkManager;
class SensorManager;
class LoRaDriver;
class GPSDriver;

/**
 * @brief Boot stages for tracking startup progress
 */
enum class BootStage {
    POWER_ON,
    CORE_INIT,
    HARDWARE_INIT,
    SENSORS_INIT,
    DISPLAY_INIT,
    CONNECTIVITY_INIT,
    SERVICES_INIT,
    VALIDATION,
    COMPLETE,
    ERROR
};

/**
 * @brief Boot error codes
 */
enum class BootError {
    NONE = 0,
    HARDWARE_INIT_FAILED,
    DISPLAY_INIT_FAILED,
    SENSOR_INIT_FAILED,
    CONNECTIVITY_INIT_FAILED,
    VALIDATION_FAILED,
    CRITICAL_ERROR
};

/**
 * @brief Boot manager handles system startup sequence with hardware integration
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
     * @brief Execute complete boot sequence
     * @return true if boot completed successfully
     */
    bool executeBootSequence();
    
    /**
     * @brief Set current boot stage
     * @param stage Current boot stage
     * @param message Optional status message
     */
    void setBootStage(BootStage stage, const String& message = "");
    
    /**
     * @brief Get current boot stage
     * @return Current boot stage
     */
    BootStage getBootStage() const { return current_stage_; }
    
    /**
     * @brief Show error message on display
     * @param error_code Error code
     * @param error_message Error message to display
     */
    void showError(BootError error_code, const String& error_message);
    
    /**
     * @brief Show boot progress on display
     * @param stage Current stage
     * @param message Status message
     * @param progress Progress percentage (0-100)
     */
    void showBootProgress(BootStage stage, const String& message, int progress = -1);
    
    /**
     * @brief Check if boot completed successfully
     * @return true if boot completed
     */
    bool isBootComplete() const { return boot_complete_; }
    
    /**
     * @brief Get boot duration in milliseconds
     * @return Boot duration
     */
    uint32_t getBootDuration() const;
    
    /**
     * @brief Get boot statistics
     * @return JSON string with boot statistics
     */
    String getBootStatistics() const;
    
    /**
     * @brief Check system health and decide if restart is needed
     * @return true if system is healthy
     */
    bool checkSystemHealth();
    
    /**
     * @brief Perform emergency restart
     * @param reason Reason for restart
     */
    void emergencyRestart(const String& reason);
    
    /**
     * @brief Show splash screen with OS info
     */
    void showSplashScreen();
    
    /**
     * @brief Validate all hardware components
     * @return true if all hardware validated successfully
     */
    bool validateHardware();
    
    /**
     * @brief Get last boot error
     * @return Last boot error code
     */
    BootError getLastError() const { return last_error_; }
    
    /**
     * @brief Set hardware manager reference
     * @param hw_manager Hardware manager instance
     */
    void setHardwareManager(HardwareManager* hw_manager) { hw_manager_ = hw_manager; }
    
    /**
     * @brief Set display manager reference
     * @param display_manager Display manager instance
     */
    void setDisplayManager(EinkManager* display_manager) { display_manager_ = display_manager; }
    
    /**
     * @brief Set sensor manager reference
     * @param sensor_manager Sensor manager instance
     */
    void setSensorManager(SensorManager* sensor_manager) { sensor_manager_ = sensor_manager; }
    
    /**
     * @brief Set LoRa driver reference
     * @param lora_driver LoRa driver instance
     */
    void setLoRaDriver(LoRaDriver* lora_driver) { lora_driver_ = lora_driver; }
    
    /**
     * @brief Set GPS driver reference
     * @param gps_driver GPS driver instance
     */
    void setGPSDriver(GPSDriver* gps_driver) { gps_driver_ = gps_driver; }
    
    /**
     * @brief Set modem driver reference
     * @param modem_driver Modem driver instance
     */
    void setModemDriver(Modem4GDriver* modem_driver) { modem_driver_ = modem_driver; }

private:
    /**
     * @brief Initialize core systems
     * @return true if successful
     */
    bool initializeCoreSystem();
    
    /**
     * @brief Initialize hardware components
     * @return true if successful
     */
    bool initializeHardware();
    
    /**
     * @brief Initialize sensors
     * @return true if successful
     */
    bool initializeSensors();
    
    /**
     * @brief Initialize display system
     * @return true if successful
     */
    bool initializeDisplay();
    
    /**
     * @brief Initialize connectivity components
     * @return true if successful
     */
    bool initializeConnectivity();
    
    /**
     * @brief Initialize services
     * @return true if successful
     */
    bool initializeServices();
    
    /**
     * @brief Initialize basic display for boot messages
     * @return true if successful
     */
    bool initBasicDisplay();
    
    /**
     * @brief Clear display and prepare for text
     */
    void clearDisplay();
    
    /**
     * @brief Draw text on display
     * @param text Text to draw
     * @param x X position
     * @param y Y position
     * @param size Text size
     */
    void drawText(const String& text, int x, int y, int size = 1);
    
    /**
     * @brief Draw progress bar
     * @param x X position
     * @param y Y position
     * @param width Width of progress bar
     * @param height Height of progress bar
     * @param progress Progress percentage (0-100)
     */
    void drawProgressBar(int x, int y, int width, int height, int progress);
    
    /**
     * @brief Update display with current content
     */
    void updateDisplay();
    
    /**
     * @brief Get stage name as string
     * @param stage Boot stage
     * @return Stage name
     */
    String getStageName(BootStage stage) const;
    
    /**
     * @brief Get error name as string
     * @param error Boot error
     * @return Error name
     */
    String getErrorName(BootError error) const;
    
    /**
     * @brief Record stage timing
     * @param stage Stage that completed
     * @param duration_ms Duration in milliseconds
     */
    void recordStageTiming(BootStage stage, uint32_t duration_ms);
    
    // Boot state
    BootStage current_stage_;
    String current_message_;
    uint32_t boot_start_time_;
    uint32_t boot_complete_time_;
    bool boot_complete_;
    bool display_available_;
    BootError last_error_;
    
    // Hardware manager references
    HardwareManager* hw_manager_;
    EinkManager* display_manager_;
    SensorManager* sensor_manager_;
    LoRaDriver* lora_driver_;
    GPSDriver* gps_driver_;
    Modem4GDriver* modem_driver_;
    
    // Boot statistics
    struct BootStats {
        uint32_t power_on_time;
        uint32_t core_init_time;
        uint32_t hardware_init_time;
        uint32_t sensors_init_time;
        uint32_t display_init_time;
        uint32_t connectivity_init_time;
        uint32_t services_init_time;
        uint32_t validation_time;
        uint32_t total_boot_time;
        int restart_count;
        String last_error;
        uint32_t hardware_validation_count;
        uint32_t successful_boots;
        uint32_t failed_boots;
    } boot_stats_;
    
    // Display state for boot messages
    bool basic_display_initialized_;
    
    // Display constants
    static const int DISPLAY_WIDTH = 240;
    static const int DISPLAY_HEIGHT = 320;
    static const int TEXT_SIZE_SMALL = 1;
    static const int TEXT_SIZE_MEDIUM = 2;
    static const int TEXT_SIZE_LARGE = 3;
    static const int PROGRESS_BAR_HEIGHT = 20;
    static const int PROGRESS_BAR_Y = 280;
    static const int STATUS_TEXT_Y = 250;
    static const int ERROR_TEXT_Y = 200;
    
    // Boot sequence validation flags
    struct ValidationFlags {
        bool core_systems;
        bool hardware_manager;
        bool display_system;
        bool sensor_system;
        bool connectivity_system;
        bool all_drivers;
    } validation_flags_;
};

#endif // BOOT_MANAGER_CORRECTED_H