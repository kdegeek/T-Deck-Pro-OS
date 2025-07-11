/**
 * @file      simple_power.h
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Simplified Arduino-compatible power manager for Phase 1
 */

#ifndef SIMPLE_POWER_H
#define SIMPLE_POWER_H

#include <Arduino.h>
#include <esp_sleep.h>
// #include <esp_pm.h> // May not be available in all ESP32 versions

enum PowerMode {
    POWER_HIGH_PERFORMANCE,
    POWER_BALANCED,
    POWER_POWER_SAVE,
    POWER_ULTRA_LOW_POWER
};

enum SleepMode {
    SLEEP_LIGHT,
    SLEEP_DEEP,
    SLEEP_HIBERNATION
};

enum WakeupSource {
    WAKEUP_TIMER,
    WAKEUP_TOUCH,
    WAKEUP_BUTTON,
    WAKEUP_GPIO,
    WAKEUP_UART,
    WAKEUP_WIFI,
    WAKEUP_EXT_INTERRUPT
};

struct PowerStats {
    uint32_t uptime_ms;
    uint32_t cpu_frequency_mhz;
    uint32_t free_heap_kb;
    uint32_t free_psram_kb;
    float battery_voltage;
    int battery_percentage;
    bool charging;
    PowerMode current_mode;
    uint32_t sleep_count;
    uint32_t wakeup_count;
};

class SimplePower {
private:
    static SimplePower* instance;
    
    PowerMode current_mode;
    uint32_t idle_timeout_ms;
    uint32_t last_activity_ms;
    uint32_t sleep_count;
    uint32_t wakeup_count;
    bool auto_power_management;
    
    // Private constructor for singleton
    SimplePower();
    
    bool setCPUFrequency(uint32_t frequency_mhz);
    void configureWiFiPowerSave(bool enabled);

public:
    // Singleton access
    static SimplePower* getInstance();
    
    // Initialization
    bool init();
    void update();
    
    // Power mode management
    bool setPowerMode(PowerMode mode);
    PowerMode getPowerMode() { return current_mode; }
    const char* getPowerModeString();
    
    // Sleep management
    bool enterLightSleep(uint32_t duration_ms);
    bool enterDeepSleep(uint32_t duration_ms);
    bool enterHibernation();
    
    // Wakeup configuration
    bool enableTimerWakeup(uint32_t duration_ms);
    bool enableTouchWakeup();
    bool enableGPIOWakeup(gpio_num_t pin, int level);
    WakeupSource getWakeupSource();
    
    // Activity tracking
    void resetIdleTimer();
    uint32_t getIdleTime();
    void setIdleTimeout(uint32_t timeout_ms);
    void enableAutoPowerManagement(bool enabled);
    
    // Battery management (stubbed for Phase 1)
    float getBatteryVoltage();
    int getBatteryPercentage();
    bool isCharging();
    bool isBatteryPresent();
    
    // Statistics and monitoring
    PowerStats getPowerStats();
    void resetStats();
    void printPowerInfo();
    
    // CPU frequency management
    bool setHighPerformanceMode();
    bool setBalancedMode();
    bool setPowerSaveMode();
    bool setUltraLowPowerMode();
    
    // Utility
    uint32_t getCPUFrequency();
    uint32_t getFreeHeap();
    uint32_t getFreePSRAM();
    uint32_t getUptime();
};

// Global power manager instance
extern SimplePower* Power;

#endif // SIMPLE_POWER_H
