/**
 * @file hardware_manager.h
 * @brief T-Deck-Pro Hardware Manager - Centralized hardware initialization
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Completely rewritten to match corrected implementation
 */

#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <vector>
#include <map>
#include <string>
#include <cstdint>
#include "config/os_config_corrected.h"

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
 * @brief Peripheral state tracking
 */
struct PeripheralState {
    bool detected;
    bool enabled;
    bool initialized;
    uint32_t last_check;
    String error_message;
};

/**
 * @brief Centralized hardware manager for all T-Deck-Pro components
 * @note Completely rewritten with corrected pin configurations
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
     * @brief Deinitialize hardware components
     */
    void deinitialize();
    
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
     * @brief Get battery voltage in volts
     * @return Battery voltage or 0 if unavailable
     */
    float getBatteryVoltage();
    
    /**
     * @brief Update battery voltage reading
     */
    void updateBatteryVoltage();
    
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
    
    // ===== PERIPHERAL DETECTION =====
    
    /**
     * @brief Check if display is available
     * @return true if display detected
     */
    bool isDisplayAvailable() const;
    
    /**
     * @brief Check if WiFi is available
     * @return true if WiFi detected
     */
    bool isWiFiAvailable() const;
    
    /**
     * @brief Check if 4G modem is available
     * @return true if modem detected
     */
    bool isModemAvailable() const;
    
    /**
     * @brief Check if LoRa is available
     * @return true if LoRa detected
     */
    bool isLoRaAvailable() const;
    
    /**
     * @brief Check if GPS is available
     * @return true if GPS detected
     */
    bool isGPSAvailable() const;
    
    /**
     * @brief Check if SD card is available
     * @return true if SD card detected
     */
    bool isSDCardAvailable() const;
    
    /**
     * @brief Process hardware events
     */
    void processEvents();
    
    /**
     * @brief Check if hardware manager is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    uint16_t get_battery_voltage();
    
private:
    // ===== CORE INITIALIZATION =====
    
    /**
     * @brief Validate hardware configuration
     * @return true if configuration is valid
     */
    bool validateConfiguration();
    
    /**
     * @brief Initialize GPIO pins
     * @return true if successful
     */
    bool initializeGPIO();
    
    /**
     * @brief Initialize SPI bus
     * @return true if successful
     */
    bool initializeSPI();
    
    /**
     * @brief Initialize I2C bus
     * @return true if successful
     */
    bool initializeI2C();
    
    /**
     * @brief Initialize power management
     * @return true if successful
     */
    bool initializePowerManagement();
    
    /**
     * @brief Detect and initialize peripherals
     * @return true if successful
     */
    bool detectPeripherals();
    
    // ===== INITIALIZATION METHODS =====
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

    // ===== CHECK METHODS =====
    void check_power_status();
    void check_sensor_status();
    void check_connectivity_status();

    // ===== PERIPHERAL DETECTION =====
    
    /**
     * @brief Check if I2C device is present
     * @param address I2C address to check
     * @return true if device found
     */
    bool isI2CDevicePresent(uint8_t address);
    
    /**
     * @brief Enable peripheral
     * @param peripheral Peripheral name
     * @return true if successful
     */
    bool enablePeripheral(const char* peripheral);
    
    /**
     * @brief Disable peripheral
     * @param peripheral Peripheral name
     * @return true if successful
     */
    bool disablePeripheral(const char* peripheral);
    
    // ===== UTILITY FUNCTIONS =====
    
    /**
     * @brief Set component status
     * @param name Component name
     * @param status Component status
     * @param error Error message
     */
    void set_component_status(const String& name, HardwareStatus status, const String& error = "");
    
    /**
     * @brief Find component by name
     * @param name Component name
     * @return Pointer to component or nullptr
     */
    HardwareComponent* find_component(const String& name);
    
    // ===== HARDWARE STATE =====
    
    bool initialized_;                    ///< Overall initialization state
    bool spi_initialized_;               ///< SPI bus initialization state
    bool i2c_initialized_;              ///< I2C bus initialization state
    bool power_management_initialized_;  ///< Power management initialization state
    
    float battery_voltage_;              ///< Current battery voltage
    uint32_t last_battery_check_;       ///< Last battery check timestamp
    
    std::map<String, PeripheralState> peripheral_states_;  ///< Peripheral state tracking
    
    // Hardware components tracking
    std::vector<HardwareComponent> components;
    
    // Hardware state
    bool system_ready;
    uint32_t last_health_check;
    uint32_t initialization_start_time;
    
    static const uint32_t HEALTH_CHECK_INTERVAL = 5000; // 5 seconds
    static const uint32_t BATTERY_CHECK_INTERVAL = 30000; // 30 seconds
};

#endif // HARDWARE_MANAGER_H