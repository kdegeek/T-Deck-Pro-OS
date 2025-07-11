/**
 * @file      power_manager.h
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Power Management System for T-Deck-Pro OS
 */

#pragma once

#include <Arduino.h>
#include <esp_pm.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <soc/rtc.h>

// Forward declarations
class HardwareManager;
class Logger;

/**
 * @brief Power management modes
 */
enum class PowerMode {
    HIGH_PERFORMANCE,   // Maximum CPU frequency, no power saving
    BALANCED,          // Moderate CPU frequency, light sleep enabled
    POWER_SAVE,        // Reduced CPU frequency, aggressive power saving
    ULTRA_LOW_POWER    // Minimum CPU frequency, maximum power saving
};

/**
 * @brief Sleep modes
 */
enum class SleepMode {
    NONE,              // No sleep
    LIGHT_SLEEP,       // Light sleep (WiFi maintained)
    DEEP_SLEEP,        // Deep sleep (complete shutdown)
    HIBERNATION        // Ultra-deep sleep (RTC only)
};

/**
 * @brief Wake-up sources
 */
enum class WakeupSource {
    TIMER,             // Timer-based wakeup
    TOUCH,             // Touch screen wakeup
    BUTTON,            // Button press wakeup
    GPIO,              // GPIO interrupt wakeup
    UART,              // UART activity wakeup
    WIFI,              // WiFi activity wakeup
    EXTERNAL           // External interrupt wakeup
};

/**
 * @brief Power statistics structure
 */
struct PowerStats {
    uint32_t uptime_ms = 0;
    uint32_t sleep_time_ms = 0;
    uint32_t wake_count = 0;
    float average_current_ma = 0.0f;
    float battery_voltage = 0.0f;
    uint8_t battery_percentage = 0;
    uint32_t cpu_frequency_mhz = 0;
    bool charging = false;
    WakeupSource last_wakeup_source = WakeupSource::TIMER;
};

/**
 * @brief Power event callback function type
 */
typedef std::function<void(PowerMode old_mode, PowerMode new_mode)> PowerModeCallback;
typedef std::function<void(SleepMode mode, uint32_t duration_ms)> SleepCallback;
typedef std::function<void(WakeupSource source)> WakeupCallback;

/**
 * @brief Power Manager Class
 * 
 * Manages ESP32-S3 power consumption and sleep modes
 */
class PowerManager {
public:
    /**
     * @brief Get singleton instance
     */
    static PowerManager& getInstance();

    /**
     * @brief Initialize power management system
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Shutdown power management system
     */
    void shutdown();

    /**
     * @brief Update power management (call in main loop)
     */
    void update();

    // === POWER MODE MANAGEMENT ===

    /**
     * @brief Set power mode
     * @param mode Power mode to set
     * @return true if mode set successfully
     */
    bool setPowerMode(PowerMode mode);

    /**
     * @brief Get current power mode
     * @return Current power mode
     */
    PowerMode getPowerMode();

    /**
     * @brief Set CPU frequency
     * @param frequency_mhz CPU frequency in MHz (80, 160, 240)
     * @return true if frequency set successfully
     */
    bool setCPUFrequency(uint32_t frequency_mhz);

    /**
     * @brief Get current CPU frequency
     * @return CPU frequency in MHz
     */
    uint32_t getCPUFrequency();

    // === SLEEP MANAGEMENT ===

    /**
     * @brief Enter light sleep mode
     * @param duration_ms Sleep duration in milliseconds (0 = indefinite)
     * @return true if sleep entered successfully
     */
    bool enterLightSleep(uint32_t duration_ms = 0);

    /**
     * @brief Enter deep sleep mode
     * @param duration_ms Sleep duration in milliseconds (0 = indefinite)
     * @return true if sleep entered successfully
     */
    bool enterDeepSleep(uint32_t duration_ms = 0);

    /**
     * @brief Enter hibernation mode (ultra-deep sleep)
     * @param duration_ms Sleep duration in milliseconds (0 = indefinite)
     * @return true if hibernation entered successfully
     */
    bool enterHibernation(uint32_t duration_ms = 0);

    /**
     * @brief Configure wakeup sources
     * @param sources Bitmask of wakeup sources to enable
     * @return true if wakeup sources configured successfully
     */
    bool configureWakeupSources(uint32_t sources);

    /**
     * @brief Enable timer wakeup
     * @param duration_ms Wakeup timer duration in milliseconds
     */
    void enableTimerWakeup(uint32_t duration_ms);

    /**
     * @brief Enable touch wakeup
     * @param threshold Touch threshold (0-1000)
     */
    void enableTouchWakeup(uint16_t threshold = 500);

    /**
     * @brief Enable GPIO wakeup
     * @param gpio_num GPIO pin number
     * @param level Trigger level (0 = low, 1 = high)
     */
    void enableGPIOWakeup(gpio_num_t gpio_num, int level);

    /**
     * @brief Disable all wakeup sources
     */
    void disableAllWakeupSources();

    // === BATTERY MANAGEMENT ===

    /**
     * @brief Get battery voltage
     * @return Battery voltage in volts
     */
    float getBatteryVoltage();

    /**
     * @brief Get battery percentage
     * @return Battery percentage (0-100)
     */
    uint8_t getBatteryPercentage();

    /**
     * @brief Check if device is charging
     * @return true if charging
     */
    bool isCharging();

    /**
     * @brief Get estimated battery life
     * @param current_ma Current consumption in mA
     * @return Estimated battery life in hours
     */
    float getEstimatedBatteryLife(float current_ma);

    // === POWER MONITORING ===

    /**
     * @brief Get power statistics
     * @return PowerStats structure
     */
    PowerStats getPowerStats();

    /**
     * @brief Reset power statistics
     */
    void resetStats();

    /**
     * @brief Get last wakeup source
     * @return Last wakeup source
     */
    WakeupSource getLastWakeupSource();

    /**
     * @brief Get wakeup reason string
     * @return Human-readable wakeup reason
     */
    String getWakeupReasonString();

    // === CALLBACK MANAGEMENT ===

    /**
     * @brief Set power mode change callback
     * @param callback Callback function
     */
    void setPowerModeCallback(PowerModeCallback callback);

    /**
     * @brief Set sleep callback
     * @param callback Callback function
     */
    void setSleepCallback(SleepCallback callback);

    /**
     * @brief Set wakeup callback
     * @param callback Callback function
     */
    void setWakeupCallback(WakeupCallback callback);

    // === UTILITY METHODS ===

    /**
     * @brief Enable/disable automatic power management
     * @param enabled true to enable automatic power management
     */
    void setAutoPowerManagement(bool enabled);

    /**
     * @brief Set idle timeout for automatic sleep
     * @param timeout_ms Idle timeout in milliseconds
     */
    void setIdleTimeout(uint32_t timeout_ms);

    /**
     * @brief Reset idle timer
     */
    void resetIdleTimer();

    /**
     * @brief Set hardware manager reference
     * @param hw Hardware manager instance
     */
    void setHardwareManager(HardwareManager* hw);

    /**
     * @brief Set logger reference
     * @param logger Logger instance
     */
    void setLogger(Logger* logger);

private:
    // Singleton pattern
    PowerManager() = default;
    ~PowerManager() = default;
    PowerManager(const PowerManager&) = delete;
    PowerManager& operator=(const PowerManager&) = delete;

    // Configuration
    PowerMode current_mode = PowerMode::BALANCED;
    bool initialized = false;
    bool auto_power_management = true;
    uint32_t idle_timeout_ms = 300000; // 5 minutes default

    // State tracking
    uint32_t last_activity_time = 0;
    uint32_t boot_time = 0;
    uint32_t total_sleep_time = 0;
    uint32_t wake_count = 0;
    WakeupSource last_wakeup_source = WakeupSource::TIMER;

    // Hardware references
    HardwareManager* hardware_manager = nullptr;
    Logger* logger = nullptr;

    // Callbacks
    PowerModeCallback power_mode_callback = nullptr;
    SleepCallback sleep_callback = nullptr;
    WakeupCallback wakeup_callback = nullptr;

    // Private methods
    bool configurePowerManagement(PowerMode mode);
    void updatePowerStats();
    void handleIdleTimeout();
    WakeupSource determineWakeupSource();
    void logPowerEvent(const char* event, const char* details = nullptr);
    float calculateAverageCurrent();
    uint8_t voltageToPercentage(float voltage);
};

// Global power manager instance
extern PowerManager& Power;
