/**
 * @file hardware_manager.h
 * @brief T-Deck-Pro Hardware Manager - Centralized hardware initialization
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <vector>
#include "../config/os_config.h"

/**
 * @brief Hardware component status
 */
enum class HardwareStatus {
    NOT_INITIALIZED,
    INITIALIZING,
    READY,
    ERROR,
    HARDWARE_DISABLED
};

/**
 * @brief Hardware component information
 */
struct HardwareComponent {
    String name;
    HardwareStatus status;
    String error_message;
    bool required;
    uint32_t last_check;
};

/**
 * @brief Centralized hardware manager for all T-Deck-Pro components
 */
class HardwareManager {
public:
    HardwareManager();
    ~HardwareManager();
    
    /**
     * @brief Initialize all hardware components
     * @return true if all required components initialized successfully
     */
    bool initialize();
    
    /**
     * @brief Update hardware status (call periodically)
     */
    void update();
    
    /**
     * @brief Get status of specific component
     * @param component_name Name of the component
     * @return Component status
     */
    HardwareStatus get_component_status(const String& component_name);
    
    /**
     * @brief Get hardware health summary
     * @return JSON string with all component statuses
     */
    String get_health_summary();
    
    /**
     * @brief Check if all required components are ready
     * @return true if system is ready
     */
    bool is_system_ready();
    
    /**
     * @brief Get battery voltage in millivolts
     * @return Battery voltage or 0 if unavailable
     */
    uint16_t get_battery_voltage();
    
    /**
     * @brief Check if USB power is connected
     * @return true if USB connected
     */
    bool is_usb_connected();
    
    /**
     * @brief Get system temperature in Celsius
     * @return Temperature or -999 if unavailable
     */
    float get_temperature();
    
    /**
     * @brief Enable/disable specific hardware component
     * @param component_name Name of the component
     * @param enable true to enable, false to disable
     * @return true if successful
     */
    bool set_component_enabled(const String& component_name, bool enable);
    
private:
    // Component initialization functions
    bool init_power_management();
    bool init_i2c_bus();
    bool init_spi_bus();
    bool init_display();
    bool init_touch();
    bool init_keyboard();
    bool init_sensors();
    bool init_connectivity();
    bool init_storage();
    bool init_audio();
    
    // Component status checking
    void check_power_status();
    void check_sensor_status();
    void check_connectivity_status();
    
    // Utility functions
    void set_component_status(const String& name, HardwareStatus status, const String& error = "");
    HardwareComponent* find_component(const String& name);
    
    // Hardware components tracking
    std::vector<HardwareComponent> components;
    
    // Hardware state
    bool system_ready;
    uint32_t last_health_check;
    uint32_t initialization_start_time;
    
    // Hardware objects (will be initialized as needed)
    bool i2c_initialized;
    bool spi_initialized;
    
    static const uint32_t HEALTH_CHECK_INTERVAL = 5000; // 5 seconds
};

#endif // HARDWARE_MANAGER_H