/**
 * @file sensor_manager.h
 * @brief Sensor Manager Header for T-Deck-Pro I2C Sensors
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Manages all I2C sensors: gyroscope, light sensor, battery gauge, power management
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Wire.h>

/**
 * @brief Sensor states
 */
typedef enum {
    SENSOR_STATE_UNINITIALIZED,
    SENSOR_STATE_INITIALIZING,
    SENSOR_STATE_READY,
    SENSOR_STATE_READING,
    SENSOR_STATE_ERROR,
    SENSOR_STATE_SLEEPING
} SensorState;

/**
 * @brief Gyroscope data structure (BHI260AP)
 */
struct GyroscopeData {
    float accel_x;          ///< Acceleration X-axis (m/s²)
    float accel_y;          ///< Acceleration Y-axis (m/s²)
    float accel_z;          ///< Acceleration Z-axis (m/s²)
    float gyro_x;           ///< Gyroscope X-axis (rad/s)
    float gyro_y;           ///< Gyroscope Y-axis (rad/s)
    float gyro_z;           ///< Gyroscope Z-axis (rad/s)
    float temperature;      ///< Temperature (°C)
    uint32_t timestamp;     ///< Measurement timestamp
    bool valid;             ///< Data validity flag
};

/**
 * @brief Light sensor data structure (LTR-553ALS)
 */
struct LightSensorData {
    uint16_t lux;           ///< Illuminance in lux
    uint16_t als_ch0;       ///< ALS channel 0 raw data
    uint16_t als_ch1;       ///< ALS channel 1 raw data
    uint16_t proximity;     ///< Proximity sensor data
    float gain;             ///< Current gain setting
    uint32_t timestamp;     ///< Measurement timestamp
    bool valid;             ///< Data validity flag
};

/**
 * @brief Battery gauge data structure (BQ27220)
 */
struct BatteryData {
    uint16_t voltage_mv;        ///< Battery voltage in millivolts
    int16_t current_ma;         ///< Battery current in milliamps (+ charging, - discharging)
    uint16_t capacity_mah;      ///< Remaining capacity in mAh
    uint16_t full_capacity_mah; ///< Full charge capacity in mAh
    uint8_t state_of_charge;    ///< State of charge percentage (0-100%)
    uint8_t state_of_health;    ///< State of health percentage (0-100%)
    int16_t temperature_c;      ///< Battery temperature in °C
    uint16_t cycle_count;       ///< Charge cycle count
    uint16_t time_to_empty;     ///< Time to empty in minutes
    uint16_t time_to_full;      ///< Time to full charge in minutes
    uint32_t timestamp;         ///< Measurement timestamp
    bool valid;                 ///< Data validity flag
    bool charging;              ///< Charging status
    bool discharging;           ///< Discharging status
};

/**
 * @brief Power management data structure (BQ25896)
 */
struct PowerManagementData {
    uint16_t input_voltage_mv;  ///< Input voltage in millivolts
    uint16_t input_current_ma;  ///< Input current in milliamps
    uint16_t charge_voltage_mv; ///< Charge voltage in millivolts
    uint16_t charge_current_ma; ///< Charge current in milliamps
    uint8_t charge_status;      ///< Charge status register
    uint8_t fault_status;       ///< Fault status register
    bool input_present;         ///< Input power present
    bool charging_enabled;      ///< Charging enabled
    bool thermal_regulation;    ///< Thermal regulation active
    bool watchdog_fault;        ///< Watchdog timer fault
    uint32_t timestamp;         ///< Measurement timestamp
    bool valid;                 ///< Data validity flag
};

/**
 * @brief Touch controller data structure (CST328)
 */
struct TouchData {
    uint16_t x;                 ///< Touch X coordinate
    uint16_t y;                 ///< Touch Y coordinate
    uint8_t pressure;           ///< Touch pressure
    uint8_t touch_count;        ///< Number of touches
    bool touched;               ///< Touch detected
    uint32_t timestamp;         ///< Touch timestamp
    bool valid;                 ///< Data validity flag
};

/**
 * @brief Keyboard controller data structure (TCA8418)
 */
struct KeyboardData {
    uint32_t key_map;           ///< Pressed key bitmap
    uint8_t last_key;           ///< Last pressed key code
    uint8_t key_count;          ///< Number of pressed keys
    bool key_pressed;           ///< Key press event
    bool key_released;          ///< Key release event
    uint32_t timestamp;         ///< Event timestamp
    bool valid;                 ///< Data validity flag
};

/**
 * @brief Sensor Manager Class
 * 
 * Manages all I2C sensors on T-Deck-Pro including:
 * - BHI260AP Gyroscope/Accelerometer
 * - LTR-553ALS Light/Proximity sensor
 * - BQ27220 Battery fuel gauge
 * - BQ25896 Power management IC
 * - CST328 Touch controller
 * - TCA8418 Keyboard controller
 */
class SensorManager {
public:
    /**
     * @brief Constructor
     */
    SensorManager();
    
    /**
     * @brief Destructor
     */
    ~SensorManager();
    
    // ===== CORE INITIALIZATION =====
    
    /**
     * @brief Initialize all sensors
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Deinitialize all sensors
     */
    void deinitialize();
    
    /**
     * @brief Check if sensor manager is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    // ===== SENSOR-SPECIFIC INITIALIZATION =====
    
    /**
     * @brief Initialize gyroscope sensor
     * @return true if successful
     */
    bool initializeGyroscope();
    
    /**
     * @brief Initialize light sensor
     * @return true if successful
     */
    bool initializeLightSensor();
    
    /**
     * @brief Initialize battery gauge
     * @return true if successful
     */
    bool initializeBatteryGauge();
    
    /**
     * @brief Initialize power management IC
     * @return true if successful
     */
    bool initializePowerManagement();
    
    /**
     * @brief Initialize touch controller
     * @return true if successful
     */
    bool initializeTouchController();
    
    /**
     * @brief Initialize keyboard controller
     * @return true if successful
     */
    bool initializeKeyboardController();
    
    // ===== DATA READING =====
    
    /**
     * @brief Read gyroscope data
     * @param data Reference to gyroscope data structure
     * @return true if successful
     */
    bool readGyroscope(GyroscopeData& data);
    
    /**
     * @brief Read light sensor data
     * @param data Reference to light sensor data structure
     * @return true if successful
     */
    bool readLightSensor(LightSensorData& data);
    
    /**
     * @brief Read battery data
     * @param data Reference to battery data structure
     * @return true if successful
     */
    bool readBattery(BatteryData& data);
    
    /**
     * @brief Read power management data
     * @param data Reference to power management data structure
     * @return true if successful
     */
    bool readPowerManagement(PowerManagementData& data);
    
    /**
     * @brief Read touch data
     * @param data Reference to touch data structure
     * @return true if successful
     */
    bool readTouch(TouchData& data);
    
    /**
     * @brief Read keyboard data
     * @param data Reference to keyboard data structure
     * @return true if successful
     */
    bool readKeyboard(KeyboardData& data);
    
    // ===== BULK OPERATIONS =====
    
    /**
     * @brief Update all sensor data
     * @return true if at least one sensor updated successfully
     */
    bool updateAllSensors();
    
    /**
     * @brief Get last known gyroscope data
     * @return Gyroscope data structure
     */
    const GyroscopeData& getLastGyroscopeData() const { return gyro_data_; }
    
    /**
     * @brief Get last known light sensor data
     * @return Light sensor data structure
     */
    const LightSensorData& getLastLightData() const { return light_data_; }
    
    /**
     * @brief Get last known battery data
     * @return Battery data structure
     */
    const BatteryData& getLastBatteryData() const { return battery_data_; }
    
    /**
     * @brief Get last known power management data
     * @return Power management data structure
     */
    const PowerManagementData& getLastPowerData() const { return power_data_; }
    
    /**
     * @brief Get last known touch data
     * @return Touch data structure
     */
    const TouchData& getLastTouchData() const { return touch_data_; }
    
    /**
     * @brief Get last known keyboard data
     * @return Keyboard data structure
     */
    const KeyboardData& getLastKeyboardData() const { return keyboard_data_; }
    
    // ===== SENSOR STATUS =====
    
    /**
     * @brief Check if gyroscope is available
     * @return true if available
     */
    bool isGyroscopeAvailable() const { return gyro_available_; }
    
    /**
     * @brief Check if light sensor is available
     * @return true if available
     */
    bool isLightSensorAvailable() const { return light_available_; }
    
    /**
     * @brief Check if battery gauge is available
     * @return true if available
     */
    bool isBatteryGaugeAvailable() const { return battery_available_; }
    
    /**
     * @brief Check if power management IC is available
     * @return true if available
     */
    bool isPowerManagementAvailable() const { return power_available_; }
    
    /**
     * @brief Check if touch controller is available
     * @return true if available
     */
    bool isTouchAvailable() const { return touch_available_; }
    
    /**
     * @brief Check if keyboard controller is available
     * @return true if available
     */
    bool isKeyboardAvailable() const { return keyboard_available_; }
    
    // ===== CONFIGURATION =====
    
    /**
     * @brief Set gyroscope sampling rate
     * @param rate_hz Sampling rate in Hz
     * @return true if successful
     */
    bool setGyroscopeSampleRate(uint16_t rate_hz);
    
    /**
     * @brief Set light sensor gain
     * @param gain Gain setting (0-7)
     * @return true if successful
     */
    bool setLightSensorGain(uint8_t gain);
    
    /**
     * @brief Enable/disable sensor auto-update
     * @param enabled true to enable auto-update
     */
    void setAutoUpdate(bool enabled) { auto_update_enabled_ = enabled; }
    
    /**
     * @brief Set auto-update interval
     * @param interval_ms Update interval in milliseconds
     */
    void setAutoUpdateInterval(uint32_t interval_ms) { auto_update_interval_ = interval_ms; }
    
    // ===== EVENT PROCESSING =====
    
    /**
     * @brief Process sensor events and interrupts
     */
    void processEvents();
    
    /**
     * @brief Set callback for gyroscope updates
     * @param callback Function to call when gyroscope data updated
     */
    void setGyroscopeCallback(void (*callback)(const GyroscopeData&));
    
    /**
     * @brief Set callback for battery updates
     * @param callback Function to call when battery data updated
     */
    void setBatteryCallback(void (*callback)(const BatteryData&));
    
    /**
     * @brief Set callback for touch events
     * @param callback Function to call when touch detected
     */
    void setTouchCallback(void (*callback)(const TouchData&));
    
    /**
     * @brief Set callback for keyboard events
     * @param callback Function to call when key pressed
     */
    void setKeyboardCallback(void (*callback)(const KeyboardData&));

private:
    // ===== INTERNAL METHODS =====
    
    /**
     * @brief Write to I2C device register
     * @param device_addr I2C device address
     * @param reg_addr Register address
     * @param value Value to write
     * @return true if successful
     */
    bool writeRegister(uint8_t device_addr, uint8_t reg_addr, uint8_t value);
    
    /**
     * @brief Read from I2C device register
     * @param device_addr I2C device address
     * @param reg_addr Register address
     * @return Register value
     */
    uint8_t readRegister(uint8_t device_addr, uint8_t reg_addr);
    
    /**
     * @brief Read multiple bytes from I2C device
     * @param device_addr I2C device address
     * @param reg_addr Starting register address
     * @param buffer Buffer to store data
     * @param length Number of bytes to read
     * @return true if successful
     */
    bool readBytes(uint8_t device_addr, uint8_t reg_addr, uint8_t* buffer, uint8_t length);
    
    /**
     * @brief Check if I2C device is present
     * @param device_addr I2C device address
     * @return true if device responds
     */
    bool isDevicePresent(uint8_t device_addr);
    
    /**
     * @brief Handle auto-update timer
     */
    void handleAutoUpdate();
    
    // ===== MEMBER VARIABLES =====
    
    bool initialized_;                    ///< Manager initialization state
    
    // Sensor availability flags
    bool gyro_available_;                 ///< Gyroscope availability
    bool light_available_;                ///< Light sensor availability
    bool battery_available_;              ///< Battery gauge availability
    bool power_available_;                ///< Power management availability
    bool touch_available_;                ///< Touch controller availability
    bool keyboard_available_;             ///< Keyboard controller availability
    
    // Sensor state
    SensorState gyro_state_;              ///< Gyroscope state
    SensorState light_state_;             ///< Light sensor state
    SensorState battery_state_;           ///< Battery gauge state
    SensorState power_state_;             ///< Power management state
    SensorState touch_state_;             ///< Touch controller state
    SensorState keyboard_state_;          ///< Keyboard controller state
    
    // Last sensor readings
    GyroscopeData gyro_data_;             ///< Last gyroscope data
    LightSensorData light_data_;          ///< Last light sensor data
    BatteryData battery_data_;            ///< Last battery data
    PowerManagementData power_data_;      ///< Last power management data
    TouchData touch_data_;                ///< Last touch data
    KeyboardData keyboard_data_;          ///< Last keyboard data
    
    // Auto-update settings
    bool auto_update_enabled_;            ///< Auto-update enable flag
    uint32_t auto_update_interval_;       ///< Auto-update interval in ms
    uint32_t last_auto_update_;           ///< Last auto-update timestamp
    
    // Callbacks
    void (*gyro_callback_)(const GyroscopeData&);     ///< Gyroscope callback
    void (*battery_callback_)(const BatteryData&);    ///< Battery callback
    void (*touch_callback_)(const TouchData&);        ///< Touch callback
    void (*keyboard_callback_)(const KeyboardData&);  ///< Keyboard callback
    
    // Statistics
    uint32_t total_reads_;                ///< Total sensor reads
    uint32_t successful_reads_;           ///< Successful sensor reads
    uint32_t i2c_errors_;                 ///< I2C communication errors
};

#endif // SENSOR_MANAGER_H