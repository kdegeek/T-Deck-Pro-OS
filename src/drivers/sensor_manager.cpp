/**
 * @file sensor_manager.cpp
 * @brief Sensor Manager Implementation for T-Deck-Pro I2C Sensors
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Implements all I2C sensors: gyroscope, light sensor, battery gauge, power management
 */

#include "sensor_manager.h"
#include "config/os_config_corrected.h"
#include <Arduino.h>

// I2C Device Addresses (from os_config_corrected.h)
#define BHI260AP_I2C_ADDRESS    0x28    // Gyroscope/Accelerometer
#define LTR553ALS_I2C_ADDRESS   0x23    // Light/Proximity sensor
#define BQ27220_I2C_ADDRESS     0x55    // Battery fuel gauge
#define BQ25896_I2C_ADDRESS     0x6B    // Power management IC
#define CST328_I2C_ADDRESS      0x1A    // Touch controller
#define TCA8418_I2C_ADDRESS     0x34    // Keyboard controller

// BHI260AP Gyroscope Registers
#define BHI260AP_REG_CHIP_ID        0x00
#define BHI260AP_REG_ERR_REG        0x02
#define BHI260AP_REG_STATUS         0x03
#define BHI260AP_REG_ACC_X_L        0x12
#define BHI260AP_REG_ACC_X_H        0x13
#define BHI260AP_REG_ACC_Y_L        0x14
#define BHI260AP_REG_ACC_Y_H        0x15
#define BHI260AP_REG_ACC_Z_L        0x16
#define BHI260AP_REG_ACC_Z_H        0x17
#define BHI260AP_REG_GYR_X_L        0x18
#define BHI260AP_REG_GYR_X_H        0x19
#define BHI260AP_REG_GYR_Y_L        0x1A
#define BHI260AP_REG_GYR_Y_H        0x1B
#define BHI260AP_REG_GYR_Z_L        0x1C
#define BHI260AP_REG_GYR_Z_H        0x1D
#define BHI260AP_REG_TEMP_L         0x20
#define BHI260AP_REG_TEMP_H         0x21
#define BHI260AP_REG_ACC_CONF       0x40
#define BHI260AP_REG_GYR_CONF       0x42
#define BHI260AP_CHIP_ID_VALUE      0x89

// LTR-553ALS Light Sensor Registers
#define LTR553ALS_REG_ALS_CONTR     0x80
#define LTR553ALS_REG_PS_CONTR      0x81
#define LTR553ALS_REG_PS_LED        0x82
#define LTR553ALS_REG_PS_N_PULSES   0x83
#define LTR553ALS_REG_PS_MEAS_RATE  0x84
#define LTR553ALS_REG_ALS_MEAS_RATE 0x85
#define LTR553ALS_REG_PART_ID       0x86
#define LTR553ALS_REG_MANUFAC_ID    0x87
#define LTR553ALS_REG_ALS_DATA_CH1_0 0x88
#define LTR553ALS_REG_ALS_DATA_CH1_1 0x89
#define LTR553ALS_REG_ALS_DATA_CH0_0 0x8A
#define LTR553ALS_REG_ALS_DATA_CH0_1 0x8B
#define LTR553ALS_REG_ALS_STATUS    0x8C
#define LTR553ALS_REG_PS_DATA_0     0x8D
#define LTR553ALS_REG_PS_DATA_1     0x8E
#define LTR553ALS_REG_PS_STATUS     0x8F
#define LTR553ALS_PART_ID_VALUE     0x92

// BQ27220 Battery Gauge Registers
#define BQ27220_REG_VOLTAGE         0x04
#define BQ27220_REG_CURRENT         0x10
#define BQ27220_REG_TEMPERATURE     0x02
#define BQ27220_REG_SOC             0x1C
#define BQ27220_REG_SOH             0x20
#define BQ27220_REG_REMAINING_CAP   0x05
#define BQ27220_REG_FULL_CAP        0x06
#define BQ27220_REG_CYCLE_COUNT     0x17
#define BQ27220_REG_TIME_TO_EMPTY   0x11
#define BQ27220_REG_TIME_TO_FULL    0x13
#define BQ27220_REG_FLAGS           0x06

// BQ25896 Power Management Registers
#define BQ25896_REG_INPUT_SOURCE    0x00
#define BQ25896_REG_POWER_ON_CONF   0x01
#define BQ25896_REG_CHARGE_CURRENT  0x02
#define BQ25896_REG_CHARGE_VOLTAGE  0x04
#define BQ25896_REG_CHARGE_TERM     0x05
#define BQ25896_REG_BOOST_THERMAL   0x06
#define BQ25896_REG_MISC_OPERATION  0x07
#define BQ25896_REG_SYSTEM_STATUS   0x08
#define BQ25896_REG_FAULT_STATUS    0x09
#define BQ25896_REG_VENDOR_PART     0x0A
#define BQ25896_REG_ADC_VBUS        0x11
#define BQ25896_REG_ADC_VSYS        0x12
#define BQ25896_REG_ADC_ICHG        0x13

// CST328 Touch Controller Registers
#define CST328_REG_DEVICE_MODE      0x00
#define CST328_REG_GESTURE_ID       0x01
#define CST328_REG_TD_STATUS        0x02
#define CST328_REG_TOUCH1_XH        0x03
#define CST328_REG_TOUCH1_XL        0x04
#define CST328_REG_TOUCH1_YH        0x05
#define CST328_REG_TOUCH1_YL        0x06
#define CST328_REG_FW_VERSION       0xA6

// TCA8418 Keyboard Controller Registers
#define TCA8418_REG_CFG             0x01
#define TCA8418_REG_INT_STAT        0x02
#define TCA8418_REG_KEY_LCK_EC      0x03
#define TCA8418_REG_KEY_EVENT_A     0x04
#define TCA8418_REG_KEY_EVENT_B     0x05
#define TCA8418_REG_KP_LCK_TIMER    0x06
#define TCA8418_REG_UNLOCK1         0x07
#define TCA8418_REG_UNLOCK2         0x08
#define TCA8418_REG_GPIO_INT_STAT1  0x09
#define TCA8418_REG_GPIO_INT_STAT2  0x0A
#define TCA8418_REG_GPIO_INT_STAT3  0x0B

SensorManager::SensorManager() :
    initialized_(false),
    gyro_available_(false),
    light_available_(false),
    battery_available_(false),
    power_available_(false),
    touch_available_(false),
    keyboard_available_(false),
    gyro_state_(SENSOR_STATE_UNINITIALIZED),
    light_state_(SENSOR_STATE_UNINITIALIZED),
    battery_state_(SENSOR_STATE_UNINITIALIZED),
    power_state_(SENSOR_STATE_UNINITIALIZED),
    touch_state_(SENSOR_STATE_UNINITIALIZED),
    keyboard_state_(SENSOR_STATE_UNINITIALIZED),
    auto_update_enabled_(false),
    auto_update_interval_(1000),
    last_auto_update_(0),
    gyro_callback_(nullptr),
    battery_callback_(nullptr),
    touch_callback_(nullptr),
    keyboard_callback_(nullptr),
    total_reads_(0),
    successful_reads_(0),
    i2c_errors_(0) {
    
    // Initialize data structures
    memset(&gyro_data_, 0, sizeof(gyro_data_));
    memset(&light_data_, 0, sizeof(light_data_));
    memset(&battery_data_, 0, sizeof(battery_data_));
    memset(&power_data_, 0, sizeof(power_data_));
    memset(&touch_data_, 0, sizeof(touch_data_));
    memset(&keyboard_data_, 0, sizeof(keyboard_data_));
}

SensorManager::~SensorManager() {
    deinitialize();
}

bool SensorManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    Serial.println("[SensorManager] Initializing sensors...");
    
    // Initialize I2C if not already done
    Wire.begin(T_DECK_I2C_SDA, T_DECK_I2C_SCL);
    Wire.setClock(400000); // 400kHz
    
    // Initialize individual sensors
    bool any_success = false;
    
    // Initialize gyroscope
    if (initializeGyroscope()) {
        gyro_available_ = true;
        gyro_state_ = SENSOR_STATE_READY;
        any_success = true;
        Serial.println("[SensorManager] Gyroscope initialized");
    } else {
        Serial.println("[SensorManager] Gyroscope initialization failed");
    }
    
    // Initialize light sensor
    if (initializeLightSensor()) {
        light_available_ = true;
        light_state_ = SENSOR_STATE_READY;
        any_success = true;
        Serial.println("[SensorManager] Light sensor initialized");
    } else {
        Serial.println("[SensorManager] Light sensor initialization failed");
    }
    
    // Initialize battery gauge
    if (initializeBatteryGauge()) {
        battery_available_ = true;
        battery_state_ = SENSOR_STATE_READY;
        any_success = true;
        Serial.println("[SensorManager] Battery gauge initialized");
    } else {
        Serial.println("[SensorManager] Battery gauge initialization failed");
    }
    
    // Initialize power management
    if (initializePowerManagement()) {
        power_available_ = true;
        power_state_ = SENSOR_STATE_READY;
        any_success = true;
        Serial.println("[SensorManager] Power management initialized");
    } else {
        Serial.println("[SensorManager] Power management initialization failed");
    }
    
    // Initialize touch controller
    if (initializeTouchController()) {
        touch_available_ = true;
        touch_state_ = SENSOR_STATE_READY;
        any_success = true;
        Serial.println("[SensorManager] Touch controller initialized");
    } else {
        Serial.println("[SensorManager] Touch controller initialization failed");
    }
    
    // Initialize keyboard controller
    if (initializeKeyboardController()) {
        keyboard_available_ = true;
        keyboard_state_ = SENSOR_STATE_READY;
        any_success = true;
        Serial.println("[SensorManager] Keyboard controller initialized");
    } else {
        Serial.println("[SensorManager] Keyboard controller initialization failed");
    }
    
    initialized_ = any_success;
    
    if (initialized_) {
        Serial.printf("[SensorManager] Initialization complete - %d sensors available\n",
                     (gyro_available_ ? 1 : 0) + (light_available_ ? 1 : 0) + 
                     (battery_available_ ? 1 : 0) + (power_available_ ? 1 : 0) +
                     (touch_available_ ? 1 : 0) + (keyboard_available_ ? 1 : 0));
    } else {
        Serial.println("[SensorManager] Initialization failed - no sensors available");
    }
    
    return initialized_;
}

void SensorManager::deinitialize() {
    if (!initialized_) {
        return;
    }
    
    // Reset all states
    gyro_available_ = false;
    light_available_ = false;
    battery_available_ = false;
    power_available_ = false;
    touch_available_ = false;
    keyboard_available_ = false;
    
    gyro_state_ = SENSOR_STATE_UNINITIALIZED;
    light_state_ = SENSOR_STATE_UNINITIALIZED;
    battery_state_ = SENSOR_STATE_UNINITIALIZED;
    power_state_ = SENSOR_STATE_UNINITIALIZED;
    touch_state_ = SENSOR_STATE_UNINITIALIZED;
    keyboard_state_ = SENSOR_STATE_UNINITIALIZED;
    
    initialized_ = false;
    
    Serial.println("[SensorManager] Deinitialized");
}

bool SensorManager::initializeGyroscope() {
    if (!isDevicePresent(BHI260AP_I2C_ADDRESS)) {
        return false;
    }
    
    // Check chip ID
    uint8_t chip_id = readRegister(BHI260AP_I2C_ADDRESS, BHI260AP_REG_CHIP_ID);
    if (chip_id != BHI260AP_CHIP_ID_VALUE) {
        Serial.printf("[SensorManager] Gyroscope chip ID mismatch: 0x%02X\n", chip_id);
        return false;
    }
    
    // Configure accelerometer
    writeRegister(BHI260AP_I2C_ADDRESS, BHI260AP_REG_ACC_CONF, 0x28); // Normal mode, ODR 100Hz
    delay(10);
    
    // Configure gyroscope
    writeRegister(BHI260AP_I2C_ADDRESS, BHI260AP_REG_GYR_CONF, 0x28); // Normal mode, ODR 100Hz
    delay(10);
    
    return true;
}

bool SensorManager::initializeLightSensor() {
    if (!isDevicePresent(LTR553ALS_I2C_ADDRESS)) {
        return false;
    }
    
    // Check part ID
    uint8_t part_id = readRegister(LTR553ALS_I2C_ADDRESS, LTR553ALS_REG_PART_ID);
    if (part_id != LTR553ALS_PART_ID_VALUE) {
        Serial.printf("[SensorManager] Light sensor part ID mismatch: 0x%02X\n", part_id);
        return false;
    }
    
    // Configure ALS
    writeRegister(LTR553ALS_I2C_ADDRESS, LTR553ALS_REG_ALS_CONTR, 0x01); // ALS active mode
    delay(10);
    
    // Set measurement rate
    writeRegister(LTR553ALS_I2C_ADDRESS, LTR553ALS_REG_ALS_MEAS_RATE, 0x03); // 500ms integration, 500ms repeat rate
    delay(10);
    
    // Configure proximity sensor
    writeRegister(LTR553ALS_I2C_ADDRESS, LTR553ALS_REG_PS_CONTR, 0x03); // PS active mode, gain 16x
    delay(10);
    
    return true;
}

bool SensorManager::initializeBatteryGauge() {
    if (!isDevicePresent(BQ27220_I2C_ADDRESS)) {
        return false;
    }
    
    // BQ27220 is typically auto-configured, just verify communication
    uint16_t voltage = (readRegister(BQ27220_I2C_ADDRESS, BQ27220_REG_VOLTAGE + 1) << 8) |
                       readRegister(BQ27220_I2C_ADDRESS, BQ27220_REG_VOLTAGE);
    
    if (voltage == 0 || voltage > 5000) { // Voltage should be reasonable (0-5V)
        return false;
    }
    
    return true;
}

bool SensorManager::initializePowerManagement() {
    if (!isDevicePresent(BQ25896_I2C_ADDRESS)) {
        return false;
    }
    
    // Read vendor/part register to verify device
    uint8_t vendor_part = readRegister(BQ25896_I2C_ADDRESS, BQ25896_REG_VENDOR_PART);
    if ((vendor_part & 0x38) != 0x00) { // Check part number bits
        Serial.printf("[SensorManager] Power management part mismatch: 0x%02X\n", vendor_part);
        return false;
    }
    
    // Configure for safe operation
    writeRegister(BQ25896_I2C_ADDRESS, BQ25896_REG_INPUT_SOURCE, 0x32); // Input current limit 500mA
    delay(10);
    
    writeRegister(BQ25896_I2C_ADDRESS, BQ25896_REG_CHARGE_CURRENT, 0x20); // Charge current 512mA
    delay(10);
    
    return true;
}

bool SensorManager::initializeTouchController() {
    if (!isDevicePresent(CST328_I2C_ADDRESS)) {
        return false;
    }
    
    // Read firmware version to verify device
    uint8_t fw_version = readRegister(CST328_I2C_ADDRESS, CST328_REG_FW_VERSION);
    Serial.printf("[SensorManager] Touch controller FW version: 0x%02X\n", fw_version);
    
    // Configure device mode
    writeRegister(CST328_I2C_ADDRESS, CST328_REG_DEVICE_MODE, 0x00); // Working mode
    delay(10);
    
    return true;
}

bool SensorManager::initializeKeyboardController() {
    if (!isDevicePresent(TCA8418_I2C_ADDRESS)) {
        return false;
    }
    
    // Configure keyboard controller
    writeRegister(TCA8418_I2C_ADDRESS, TCA8418_REG_CFG, 0x80); // Enable key events
    delay(10);
    
    return true;
}

bool SensorManager::readGyroscope(GyroscopeData& data) {
    if (!gyro_available_ || gyro_state_ != SENSOR_STATE_READY) {
        return false;
    }
    
    total_reads_++;
    gyro_state_ = SENSOR_STATE_READING;
    
    uint8_t buffer[12];
    bool success = readBytes(BHI260AP_I2C_ADDRESS, BHI260AP_REG_ACC_X_L, buffer, 12);
    
    if (success) {
        // Parse accelerometer data (LSB = 0.061 mg)
        int16_t acc_raw_x = (buffer[1] << 8) | buffer[0];
        int16_t acc_raw_y = (buffer[3] << 8) | buffer[2];
        int16_t acc_raw_z = (buffer[5] << 8) | buffer[4];
        
        data.accel_x = acc_raw_x * 0.061f * 9.81f / 1000.0f; // Convert to m/s²
        data.accel_y = acc_raw_y * 0.061f * 9.81f / 1000.0f;
        data.accel_z = acc_raw_z * 0.061f * 9.81f / 1000.0f;
        
        // Parse gyroscope data (LSB = 0.004375 dps)
        int16_t gyro_raw_x = (buffer[7] << 8) | buffer[6];
        int16_t gyro_raw_y = (buffer[9] << 8) | buffer[8];
        int16_t gyro_raw_z = (buffer[11] << 8) | buffer[10];
        
        data.gyro_x = gyro_raw_x * 0.004375f * PI / 180.0f; // Convert to rad/s
        data.gyro_y = gyro_raw_y * 0.004375f * PI / 180.0f;
        data.gyro_z = gyro_raw_z * 0.004375f * PI / 180.0f;
        
        // Read temperature
        uint8_t temp_buffer[2];
        if (readBytes(BHI260AP_I2C_ADDRESS, BHI260AP_REG_TEMP_L, temp_buffer, 2)) {
            int16_t temp_raw = (temp_buffer[1] << 8) | temp_buffer[0];
            data.temperature = temp_raw / 512.0f + 23.0f; // Convert to °C
        }
        
        data.timestamp = millis();
        data.valid = true;
        
        gyro_data_ = data;
        successful_reads_++;
        
        if (gyro_callback_) {
            gyro_callback_(data);
        }
    } else {
        i2c_errors_++;
        data.valid = false;
    }
    
    gyro_state_ = SENSOR_STATE_READY;
    return success;
}

bool SensorManager::readLightSensor(LightSensorData& data) {
    if (!light_available_ || light_state_ != SENSOR_STATE_READY) {
        return false;
    }
    
    total_reads_++;
    light_state_ = SENSOR_STATE_READING;
    
    // Read ALS data
    uint8_t als_buffer[4];
    bool success = readBytes(LTR553ALS_I2C_ADDRESS, LTR553ALS_REG_ALS_DATA_CH1_0, als_buffer, 4);
    
    if (success) {
        data.als_ch1 = (als_buffer[1] << 8) | als_buffer[0];
        data.als_ch0 = (als_buffer[3] << 8) | als_buffer[2];
        
        // Calculate lux (simplified calculation)
        if (data.als_ch0 > 0) {
            float ratio = (float)data.als_ch1 / (float)data.als_ch0;
            if (ratio < 0.45f) {
                data.lux = (1.7743f * data.als_ch0 + 1.1059f * data.als_ch1);
            } else if (ratio < 0.64f) {
                data.lux = (4.2785f * data.als_ch0 - 1.9548f * data.als_ch1);
            } else {
                data.lux = (0.5926f * data.als_ch0 - 0.1185f * data.als_ch1);
            }
        } else {
            data.lux = 0;
        }
        
        data.gain = 1.0f; // Default gain
        
        // Read proximity data
        uint8_t ps_buffer[2];
        if (readBytes(LTR553ALS_I2C_ADDRESS, LTR553ALS_REG_PS_DATA_0, ps_buffer, 2)) {
            data.proximity = (ps_buffer[1] << 8) | ps_buffer[0];
        }
        
        data.timestamp = millis();
        data.valid = true;
        
        light_data_ = data;
        successful_reads_++;
    } else {
        i2c_errors_++;
        data.valid = false;
    }
    
    light_state_ = SENSOR_STATE_READY;
    return success;
}

bool SensorManager::readBattery(BatteryData& data) {
    if (!battery_available_ || battery_state_ != SENSOR_STATE_READY) {
        return false;
    }
    
    total_reads_++;
    battery_state_ = SENSOR_STATE_READING;
    
    // Read voltage (16-bit)
    uint8_t voltage_buffer[2];
    bool success = readBytes(BQ27220_I2C_ADDRESS, BQ27220_REG_VOLTAGE, voltage_buffer, 2);
    
    if (success) {
        data.voltage_mv = (voltage_buffer[1] << 8) | voltage_buffer[0];
        
        // Read current (16-bit signed)
        uint8_t current_buffer[2];
        if (readBytes(BQ27220_I2C_ADDRESS, BQ27220_REG_CURRENT, current_buffer, 2)) {
            data.current_ma = (int16_t)((current_buffer[1] << 8) | current_buffer[0]);
        }
        
        // Read temperature
        uint8_t temp_buffer[2];
        if (readBytes(BQ27220_I2C_ADDRESS, BQ27220_REG_TEMPERATURE, temp_buffer, 2)) {
            uint16_t temp_raw = (temp_buffer[1] << 8) | temp_buffer[0];
            data.temperature_c = (temp_raw / 10) - 273; // Convert from 0.1K to °C
        }
        
        // Read state of charge
        uint8_t soc_buffer[2];
        if (readBytes(BQ27220_I2C_ADDRESS, BQ27220_REG_SOC, soc_buffer, 2)) {
            data.state_of_charge = (soc_buffer[1] << 8) | soc_buffer[0];
        }
        
        // Read remaining capacity
        uint8_t cap_buffer[2];
        if (readBytes(BQ27220_I2C_ADDRESS, BQ27220_REG_REMAINING_CAP, cap_buffer, 2)) {
            data.capacity_mah = (cap_buffer[1] << 8) | cap_buffer[0];
        }
        
        // Read full capacity
        uint8_t full_cap_buffer[2];
        if (readBytes(BQ27220_I2C_ADDRESS, BQ27220_REG_FULL_CAP, full_cap_buffer, 2)) {
            data.full_capacity_mah = (full_cap_buffer[1] << 8) | full_cap_buffer[0];
        }
        
        // Determine charging status
        data.charging = (data.current_ma > 0);
        data.discharging = (data.current_ma < 0);
        
        data.timestamp = millis();
        data.valid = true;
        
        battery_data_ = data;
        successful_reads_++;
        
        if (battery_callback_) {
            battery_callback_(data);
        }
    } else {
        i2c_errors_++;
        data.valid = false;
    }
    
    battery_state_ = SENSOR_STATE_READY;
    return success;
}

bool SensorManager::readPowerManagement(PowerManagementData& data) {
    if (!power_available_ || power_state_ != SENSOR_STATE_READY) {
        return false;
    }
    
    total_reads_++;
    power_state_ = SENSOR_STATE_READING;
    
    // Read system status
    uint8_t sys_status = readRegister(BQ25896_I2C_ADDRESS, BQ25896_REG_SYSTEM_STATUS);
    bool success = (sys_status != 0xFF); // Basic communication check
    
    if (success) {
        data.charge_status = sys_status;
        
        // Read fault status
        data.fault_status = readRegister(BQ25896_I2C_ADDRESS, BQ25896_REG_FAULT_STATUS);
        
        // Read ADC values
        uint8_t vbus_adc = readRegister(BQ25896_I2C_ADDRESS, BQ25896_REG_ADC_VBUS);
        data.input_voltage_mv = (vbus_adc & 0x7F) * 100 + 2600; // VBUS ADC
        
        uint8_t vsys_adc = readRegister(BQ25896_I2C_ADDRESS, BQ25896_REG_ADC_VSYS);
        data.charge_voltage_mv = (vsys_adc & 0x7F) * 20 + 2304; // VSYS ADC
        
        uint8_t ichg_adc = readRegister(BQ25896_I2C_ADDRESS, BQ25896_REG_ADC_ICHG);
        data.charge_current_ma = (ichg_adc & 0x7F) * 50; // ICHG ADC
        
        // Parse status flags
        data.input_present = (sys_status & 0x04) != 0;
        data.charging_enabled = (sys_status & 0x18) != 0;
        data.thermal_regulation = (sys_status & 0x02) != 0;
        data.watchdog_fault = (data.fault_status & 0x80) != 0;
        
        data.timestamp = millis();
        data.valid = true;
        
        power_data_ = data;
        successful_reads_++;
    } else {
        i2c_errors_++;
        data.valid = false;
    }
    
    power_state_ = SENSOR_STATE_READY;
    return success;
}

bool SensorManager::readTouch(TouchData& data) {
    if (!touch_available_ || touch_state_ != SENSOR_STATE_READY) {
        return false;
    }
    
    total_reads_++;
    touch_state_ = SENSOR_STATE_READING;
    
    // Read touch status
    uint8_t td_status = readRegister(CST328_I2C_ADDRESS, CST328_REG_TD_STATUS);
    bool success = (td_status != 0xFF);
    
    if (success) {
        data.touch_count = td_status & 0x0F;
        data.touched = (data.touch_count > 0);
        
        if (data.touched) {
            // Read first touch point
            uint8_t touch_buffer[4];
            if (readBytes(CST328_I2C_ADDRESS, CST328_REG_TOUCH1_XH, touch_buffer, 4)) {
                data.x = ((touch_buffer[0] & 0x0F) << 8) | touch_buffer[1];
                data.y = ((touch_buffer[2] & 0x0F) << 8) | touch_buffer[3];
                data.pressure = (touch_buffer[0] & 0xC0) >> 6; // Event flag as pressure
            }
        } else {
            data.x = 0;
            data.y = 0;
            data.pressure = 0;
        }
        
        data.timestamp = millis();
        data.valid = true;
        
        touch_data_ = data;
        successful_reads_++;
        
        if (touch_callback_ && data.touched) {
            touch_callback_(data);
        }
    } else {
        i2c_errors_++;
        data.valid = false;
    }
    
    touch_state_ = SENSOR_STATE_READY;
    return success;
}

bool SensorManager::readKeyboard(KeyboardData& data) {
    if (!keyboard_available_ || keyboard_state_ != SENSOR_STATE_READY) {
        return false;
    }
    
    total_reads_++;
    keyboard_state_ = SENSOR_STATE_READING;
    
    // Read interrupt status
    uint8_t int_status = readRegister(TCA8418_I2C_ADDRESS, TCA8418_REG_INT_STAT);
    bool success = (int_status != 0xFF);
    
    if (success) {
        data.key_pressed = (int_status & 0x01) != 0;
        data.key_released = (int_status & 0x02) != 0;
        
        if (data.key_pressed || data.key_released) {
            // Read key events
            data.last_key = readRegister(TCA8418_I2C_ADDRESS, TCA8418_REG_KEY_EVENT_A);
            data.key_count = 1;
            data.key_map = (1 << data.last_key);
        } else {
            data.last_key = 0;
            data.key_count = 0;
            data.key_map = 0;
        }
        
        data.timestamp = millis();
        data.valid = true;
        
        keyboard_data_ = data;
        successful_reads_++;
        
        if (keyboard_callback_ && (data.key_pressed || data.key_released)) {
            keyboard_callback_(data);
        }
    } else {
        i2c_errors_++;
        data.valid = false;
    }
    
    keyboard_state_ = SENSOR_STATE_READY;
    return success;
}

bool SensorManager::updateAllSensors() {
    if (!initialized_) {
        return false;
    }
    
    bool any_success = false;
    
    if (gyro_available_) {
        GyroscopeData gyro_data;
        if (readGyroscope(gyro_data)) {
            any_success = true;
        }
    }
    
    if (light_available_) {
        LightSensorData light_data;
        if (readLightSensor(light_data)) {
            any_success = true;
        }
    }
    
    if (battery_available_) {
        BatteryData battery_data;
        if (readBattery(battery_data)) {
            any_success = true;
        }
    }
    
    if (power_available_) {
        PowerManagementData power_data;
        if (readPowerManagement(power_data)) {
            any_success = true;
        }
    }
    
    if (touch_available_) {
        TouchData touch_data;
        if (readTouch(touch_data)) {
            any_success = true;
        }
    }
    
    if (keyboard_available_) {
        KeyboardData keyboard_data;
        if (readKeyboard(keyboard_data)) {
            any_success = true;
        }
    }
    
    return any_success;
}

bool SensorManager::setGyroscopeSampleRate(uint16_t rate_hz) {
    if (!gyro_available_) {
        return false;
    }
    
    uint8_t config_value = 0x20; // Normal mode base
    
    if (rate_hz <= 25) {
        config_value |= 0x06; // 25Hz
    } else if (rate_hz <= 50) {
        config_value |= 0x07; // 50Hz
    } else if (rate_hz <= 100) {
        config_value |= 0x08; // 100Hz
    } else {
        config_value |= 0x09; // 200Hz
    }
    
    writeRegister(BHI260AP_I2C_ADDRESS, BHI260AP_REG_ACC_CONF, config_value);
    writeRegister(BHI260AP_I2C_ADDRESS, BHI260AP_REG_GYR_CONF, config_value);
    
    return true;
}

bool SensorManager::setLightSensorGain(uint8_t gain) {
    if (!light_available_ || gain > 7) {
        return false;
    }
    
    uint8_t config = 0x01 | (gain << 2); // Active mode with specified gain
    return writeRegister(LTR553ALS_I2C_ADDRESS, LTR553ALS_REG_ALS_CONTR, config);
}

void SensorManager::processEvents() {
    if (!initialized_) {
        return;
    }
    
    // Handle auto-update
    if (auto_update_enabled_) {
        handleAutoUpdate();
    }
    
    // Process any interrupt-driven events here
    // This could be expanded to handle GPIO interrupts from sensors
}

void SensorManager::setGyroscopeCallback(void (*callback)(const GyroscopeData&)) {
    gyro_callback_ = callback;
}

void SensorManager::setBatteryCallback(void (*callback)(const BatteryData&)) {
    battery_callback_ = callback;
}

void SensorManager::setTouchCallback(void (*callback)(const TouchData&)) {
    touch_callback_ = callback;
}

void SensorManager::setKeyboardCallback(void (*callback)(const KeyboardData&)) {
    keyboard_callback_ = callback;
}

bool SensorManager::writeRegister(uint8_t device_addr, uint8_t reg_addr, uint8_t value) {
    Wire.beginTransmission(device_addr);
    Wire.write(reg_addr);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

uint8_t SensorManager::readRegister(uint8_t device_addr, uint8_t reg_addr) {
    Wire.beginTransmission(device_addr);
    Wire.write(reg_addr);
    if (Wire.endTransmission() != 0) {
        return 0xFF;
    }
    
    Wire.requestFrom(device_addr, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    
    return 0xFF;
}

bool SensorManager::readBytes(uint8_t device_addr, uint8_t reg_addr, uint8_t* buffer, uint8_t length) {
    if (!buffer || length == 0) {
        return false;
    }
    
    Wire.beginTransmission(device_addr);
    Wire.write(reg_addr);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    Wire.requestFrom(device_addr, length);
    
    uint8_t i = 0;
    while (Wire.available() && i < length) {
        buffer[i++] = Wire.read();
    }
    
    return (i == length);
}

bool SensorManager::isDevicePresent(uint8_t device_addr) {
    Wire.beginTransmission(device_addr);
    return (Wire.endTransmission() == 0);
}

void SensorManager::handleAutoUpdate() {
    uint32_t current_time = millis();
    
    if (current_time - last_auto_update_ >= auto_update_interval_) {
        updateAllSensors();
        last_auto_update_ = current_time;
    }
}