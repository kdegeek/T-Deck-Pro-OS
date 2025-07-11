/**
 * @file      power_manager.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Power Management System Implementation
 */

#include "power_manager.h"
#include "../drivers/hardware_manager.h"
#include "logger.h"
#include <esp_wifi.h>
#include <esp_bt.h>

// Global power manager instance
PowerManager& Power = PowerManager::getInstance();

PowerManager& PowerManager::getInstance() {
    static PowerManager instance;
    return instance;
}

bool PowerManager::init() {
    if (initialized) {
        return true;
    }

    logPowerEvent("PowerManager", "Initializing power management system");

    // Record boot time
    boot_time = millis();
    last_activity_time = boot_time;

    // Determine wakeup source
    last_wakeup_source = determineWakeupSource();
    wake_count++;

    // Configure default power management
    if (!configurePowerManagement(PowerMode::BALANCED)) {
        logPowerEvent("PowerManager", "Failed to configure default power mode");
        return false;
    }

    // Enable automatic light sleep
    esp_pm_config_esp32s3_t pm_config = {
        .max_freq_mhz = 160,
        .min_freq_mhz = 40,
        .light_sleep_enable = true
    };
    
    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret != ESP_OK) {
        logPowerEvent("PowerManager", "Failed to configure power management");
        return false;
    }

    initialized = true;
    logPowerEvent("PowerManager", "Power management initialized successfully");
    
    return true;
}

void PowerManager::shutdown() {
    if (!initialized) {
        return;
    }

    logPowerEvent("PowerManager", "Shutting down power management");

    // Disable power management
    esp_pm_config_esp32s3_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 240,
        .light_sleep_enable = false
    };
    esp_pm_configure(&pm_config);

    initialized = false;
}

void PowerManager::update() {
    if (!initialized) {
        return;
    }

    // Update power statistics
    updatePowerStats();

    // Handle automatic power management
    if (auto_power_management) {
        handleIdleTimeout();
    }
}

// === POWER MODE MANAGEMENT ===

bool PowerManager::setPowerMode(PowerMode mode) {
    if (!initialized) {
        return false;
    }

    if (mode == current_mode) {
        return true; // Already in requested mode
    }

    PowerMode old_mode = current_mode;
    
    if (configurePowerManagement(mode)) {
        current_mode = mode;
        
        // Call callback if set
        if (power_mode_callback) {
            power_mode_callback(old_mode, current_mode);
        }
        
        logPowerEvent("PowerMode", ("Changed from " + String((int)old_mode) + " to " + String((int)current_mode)).c_str());
        return true;
    }
    
    return false;
}

PowerMode PowerManager::getPowerMode() {
    return current_mode;
}

bool PowerManager::setCPUFrequency(uint32_t frequency_mhz) {
    // Validate frequency
    if (frequency_mhz != 80 && frequency_mhz != 160 && frequency_mhz != 240) {
        logPowerEvent("PowerManager", ("Invalid CPU frequency: " + String(frequency_mhz)).c_str());
        return false;
    }

    esp_pm_config_esp32s3_t pm_config = {
        .max_freq_mhz = frequency_mhz,
        .min_freq_mhz = 40, // Keep minimum low for power saving
        .light_sleep_enable = (frequency_mhz < 240) // Disable light sleep at max frequency
    };
    
    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret == ESP_OK) {
        logPowerEvent("PowerManager", ("CPU frequency set to " + String(frequency_mhz) + " MHz").c_str());
        return true;
    } else {
        logPowerEvent("PowerManager", ("Failed to set CPU frequency: " + String(ret)).c_str());
        return false;
    }
}

uint32_t PowerManager::getCPUFrequency() {
    return getCpuFrequencyMhz();
}

// === SLEEP MANAGEMENT ===

bool PowerManager::enterLightSleep(uint32_t duration_ms) {
    if (!initialized) {
        return false;
    }

    logPowerEvent("LightSleep", ("Entering light sleep for " + String(duration_ms) + " ms").c_str());

    // Configure wakeup timer if duration specified
    if (duration_ms > 0) {
        enableTimerWakeup(duration_ms);
    }

    // Call sleep callback
    if (sleep_callback) {
        sleep_callback(SleepMode::LIGHT_SLEEP, duration_ms);
    }

    // Enter light sleep
    uint32_t sleep_start = millis();
    esp_light_sleep_start();
    uint32_t sleep_duration = millis() - sleep_start;

    // Update statistics
    total_sleep_time += sleep_duration;
    wake_count++;
    last_wakeup_source = determineWakeupSource();

    // Call wakeup callback
    if (wakeup_callback) {
        wakeup_callback(last_wakeup_source);
    }

    logPowerEvent("LightSleep", ("Woke up after " + String(sleep_duration) + " ms").c_str());
    
    return true;
}

bool PowerManager::enterDeepSleep(uint32_t duration_ms) {
    if (!initialized) {
        return false;
    }

    logPowerEvent("DeepSleep", ("Entering deep sleep for " + String(duration_ms) + " ms").c_str());

    // Configure wakeup timer if duration specified
    if (duration_ms > 0) {
        enableTimerWakeup(duration_ms);
    }

    // Call sleep callback
    if (sleep_callback) {
        sleep_callback(SleepMode::DEEP_SLEEP, duration_ms);
    }

    // Prepare for deep sleep
    if (hardware_manager) {
        // Shutdown non-essential hardware
        hardware_manager->setDisplayPower(false);
    }

    // Enter deep sleep (this will reset the device)
    esp_deep_sleep_start();
    
    // This line should never be reached
    return false;
}

bool PowerManager::enterHibernation(uint32_t duration_ms) {
    if (!initialized) {
        return false;
    }

    logPowerEvent("Hibernation", ("Entering hibernation for " + String(duration_ms) + " ms").c_str());

    // Configure wakeup timer if duration specified
    if (duration_ms > 0) {
        enableTimerWakeup(duration_ms);
    }

    // Call sleep callback
    if (sleep_callback) {
        sleep_callback(SleepMode::HIBERNATION, duration_ms);
    }

    // Disable all peripherals for maximum power saving
    esp_wifi_stop();
    esp_bt_controller_disable();

    // Enter hibernation (ultra-deep sleep)
    esp_deep_sleep_start();
    
    // This line should never be reached
    return false;
}

// === WAKEUP SOURCE CONFIGURATION ===

bool PowerManager::configureWakeupSources(uint32_t sources) {
    // Disable all wakeup sources first
    disableAllWakeupSources();

    bool success = true;

    if (sources & (1 << (int)WakeupSource::TIMER)) {
        // Timer wakeup will be configured when entering sleep
    }

    if (sources & (1 << (int)WakeupSource::TOUCH)) {
        enableTouchWakeup();
    }

    if (sources & (1 << (int)WakeupSource::BUTTON)) {
        // Configure button wakeup (implementation depends on hardware)
        enableGPIOWakeup(GPIO_NUM_0, 0); // Boot button as example
    }

    if (sources & (1 << (int)WakeupSource::GPIO)) {
        // GPIO wakeup sources will be configured individually
    }

    if (sources & (1 << (int)WakeupSource::UART)) {
        esp_sleep_enable_uart_wakeup(0);
    }

    return success;
}

void PowerManager::enableTimerWakeup(uint32_t duration_ms) {
    esp_sleep_enable_timer_wakeup(duration_ms * 1000ULL); // Convert to microseconds
    logPowerEvent("Wakeup", ("Timer wakeup enabled for " + String(duration_ms) + " ms").c_str());
}

void PowerManager::enableTouchWakeup(uint16_t threshold) {
    esp_sleep_enable_touchpad_wakeup();
    logPowerEvent("Wakeup", ("Touch wakeup enabled with threshold " + String(threshold)).c_str());
}

void PowerManager::enableGPIOWakeup(gpio_num_t gpio_num, int level) {
    esp_sleep_enable_ext0_wakeup(gpio_num, level);
    logPowerEvent("Wakeup", ("GPIO wakeup enabled on pin " + String(gpio_num)).c_str());
}

void PowerManager::disableAllWakeupSources() {
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
}

// === BATTERY MANAGEMENT ===

float PowerManager::getBatteryVoltage() {
    if (hardware_manager) {
        auto battery_info = hardware_manager->getBatteryInfo();
        return battery_info.voltage;
    }
    return 0.0f;
}

uint8_t PowerManager::getBatteryPercentage() {
    if (hardware_manager) {
        auto battery_info = hardware_manager->getBatteryInfo();
        return battery_info.percentage;
    }
    return 0;
}

bool PowerManager::isCharging() {
    if (hardware_manager) {
        auto battery_info = hardware_manager->getBatteryInfo();
        return battery_info.charging;
    }
    return false;
}

float PowerManager::getEstimatedBatteryLife(float current_ma) {
    float battery_capacity_mah = 2000.0f; // Typical Li-ion battery capacity
    uint8_t battery_percent = getBatteryPercentage();
    
    if (current_ma <= 0 || battery_percent == 0) {
        return 0.0f;
    }
    
    float remaining_capacity = (battery_capacity_mah * battery_percent) / 100.0f;
    return remaining_capacity / current_ma; // Hours
}

// === POWER MONITORING ===

PowerStats PowerManager::getPowerStats() {
    PowerStats stats;

    stats.uptime_ms = millis() - boot_time;
    stats.sleep_time_ms = total_sleep_time;
    stats.wake_count = wake_count;
    stats.average_current_ma = calculateAverageCurrent();
    stats.battery_voltage = getBatteryVoltage();
    stats.battery_percentage = getBatteryPercentage();
    stats.cpu_frequency_mhz = getCPUFrequency();
    stats.charging = isCharging();
    stats.last_wakeup_source = last_wakeup_source;

    return stats;
}

void PowerManager::resetStats() {
    boot_time = millis();
    total_sleep_time = 0;
    wake_count = 0;
    logPowerEvent("PowerManager", "Power statistics reset");
}

WakeupSource PowerManager::getLastWakeupSource() {
    return last_wakeup_source;
}

String PowerManager::getWakeupReasonString() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            return "Power on reset or external reset";
        case ESP_SLEEP_WAKEUP_ALL:
            return "All wakeup sources";
        case ESP_SLEEP_WAKEUP_EXT0:
            return "External signal using RTC_IO";
        case ESP_SLEEP_WAKEUP_EXT1:
            return "External signal using RTC_CNTL";
        case ESP_SLEEP_WAKEUP_TIMER:
            return "Timer";
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            return "Touchpad";
        case ESP_SLEEP_WAKEUP_ULP:
            return "ULP program";
        case ESP_SLEEP_WAKEUP_GPIO:
            return "GPIO";
        case ESP_SLEEP_WAKEUP_UART:
            return "UART";
        case ESP_SLEEP_WAKEUP_WIFI:
            return "WiFi";
        case ESP_SLEEP_WAKEUP_COCPU:
            return "COCPU";
        case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
            return "COCPU trap trigger";
        case ESP_SLEEP_WAKEUP_BT:
            return "Bluetooth";
        default:
            return "Unknown (" + String(wakeup_reason) + ")";
    }
}

// === CALLBACK MANAGEMENT ===

void PowerManager::setPowerModeCallback(PowerModeCallback callback) {
    power_mode_callback = callback;
}

void PowerManager::setSleepCallback(SleepCallback callback) {
    sleep_callback = callback;
}

void PowerManager::setWakeupCallback(WakeupCallback callback) {
    wakeup_callback = callback;
}

// === UTILITY METHODS ===

void PowerManager::setAutoPowerManagement(bool enabled) {
    auto_power_management = enabled;
    logPowerEvent("PowerManager", enabled ? "Auto power management enabled" : "Auto power management disabled");
}

void PowerManager::setIdleTimeout(uint32_t timeout_ms) {
    idle_timeout_ms = timeout_ms;
    logPowerEvent("PowerManager", ("Idle timeout set to " + String(timeout_ms) + " ms").c_str());
}

void PowerManager::resetIdleTimer() {
    last_activity_time = millis();
}

void PowerManager::setHardwareManager(HardwareManager* hw) {
    hardware_manager = hw;
}

void PowerManager::setLogger(Logger* logger_instance) {
    logger = logger_instance;
}

// === PRIVATE METHODS ===

bool PowerManager::configurePowerManagement(PowerMode mode) {
    esp_pm_config_esp32s3_t pm_config;

    switch (mode) {
        case PowerMode::HIGH_PERFORMANCE:
            pm_config.max_freq_mhz = 240;
            pm_config.min_freq_mhz = 240;
            pm_config.light_sleep_enable = false;
            break;

        case PowerMode::BALANCED:
            pm_config.max_freq_mhz = 160;
            pm_config.min_freq_mhz = 40;
            pm_config.light_sleep_enable = true;
            break;

        case PowerMode::POWER_SAVE:
            pm_config.max_freq_mhz = 80;
            pm_config.min_freq_mhz = 40;
            pm_config.light_sleep_enable = true;
            break;

        case PowerMode::ULTRA_LOW_POWER:
            pm_config.max_freq_mhz = 80;
            pm_config.min_freq_mhz = 40;
            pm_config.light_sleep_enable = true;
            break;

        default:
            return false;
    }

    esp_err_t ret = esp_pm_configure(&pm_config);
    return (ret == ESP_OK);
}

void PowerManager::updatePowerStats() {
    // Update statistics periodically
    static uint32_t last_stats_update = 0;
    uint32_t now = millis();

    if (now - last_stats_update > 10000) { // Update every 10 seconds
        last_stats_update = now;
        // Statistics are updated on-demand in getPowerStats()
    }
}

void PowerManager::handleIdleTimeout() {
    uint32_t now = millis();

    if (now - last_activity_time > idle_timeout_ms) {
        // Device has been idle, enter power save mode
        if (current_mode != PowerMode::POWER_SAVE) {
            logPowerEvent("AutoPower", "Entering power save mode due to inactivity");
            setPowerMode(PowerMode::POWER_SAVE);
        }

        // Consider entering light sleep for extended idle periods
        if (now - last_activity_time > idle_timeout_ms * 2) {
            logPowerEvent("AutoPower", "Entering light sleep due to extended inactivity");
            enterLightSleep(60000); // Sleep for 1 minute
        }
    }
}

WakeupSource PowerManager::determineWakeupSource() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER:
            return WakeupSource::TIMER;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            return WakeupSource::TOUCH;
        case ESP_SLEEP_WAKEUP_EXT0:
        case ESP_SLEEP_WAKEUP_EXT1:
            return WakeupSource::BUTTON;
        case ESP_SLEEP_WAKEUP_GPIO:
            return WakeupSource::GPIO;
        case ESP_SLEEP_WAKEUP_UART:
            return WakeupSource::UART;
        case ESP_SLEEP_WAKEUP_WIFI:
            return WakeupSource::WIFI;
        default:
            return WakeupSource::EXTERNAL;
    }
}

void PowerManager::logPowerEvent(const char* event, const char* details) {
    if (logger) {
        if (details) {
            logger->info(event, "%s", details);
        } else {
            logger->info(event, "Power event");
        }
    } else {
        // Fallback to Serial if logger not available
        Serial.printf("[POWER] %s: %s\n", event, details ? details : "");
    }
}

float PowerManager::calculateAverageCurrent() {
    // This is a simplified calculation
    // In a real implementation, you would measure actual current consumption
    float base_current = 50.0f; // Base current in mA

    switch (current_mode) {
        case PowerMode::HIGH_PERFORMANCE:
            return base_current * 2.0f;
        case PowerMode::BALANCED:
            return base_current * 1.2f;
        case PowerMode::POWER_SAVE:
            return base_current * 0.8f;
        case PowerMode::ULTRA_LOW_POWER:
            return base_current * 0.5f;
        default:
            return base_current;
    }
}

uint8_t PowerManager::voltageToPercentage(float voltage) {
    // Li-ion battery voltage to percentage conversion
    if (voltage >= 4.2f) return 100;
    if (voltage >= 4.0f) return 80 + (voltage - 4.0f) * 100;
    if (voltage >= 3.8f) return 60 + (voltage - 3.8f) * 100;
    if (voltage >= 3.6f) return 40 + (voltage - 3.6f) * 100;
    if (voltage >= 3.4f) return 20 + (voltage - 3.4f) * 100;
    if (voltage >= 3.2f) return (voltage - 3.2f) * 100;
    return 0;
}
