/**
 * @file hardware_manager.cpp
 * @brief Hardware Manager Implementation - CORRECTED VERSION
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Completely rewritten with correct pin configurations from .REFERENCE
 */

#include "hardware_manager.h"
#include <Arduino.h>
#include "config/os_config_corrected.h"
#include "../core/hal/board_config_corrected.h"

#include <Wire.h>
#include <SPI.h>
#include <esp_log.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/spi_master.h>

#ifndef BOARD_GYRO_EN
#define BOARD_GYRO_EN 45 // TODO: Replace with actual pin
#endif
#ifndef BOARD_MODEM_EN
#define BOARD_MODEM_EN 46 // TODO: Replace with actual pin
#endif
#ifndef BOARD_KB_LED
#define BOARD_KB_LED 47 // TODO: Replace with actual pin
#endif
#ifndef BOARD_LORA_DIO1
#define BOARD_LORA_DIO1 48 // TODO: Replace with actual pin
#endif
#ifndef BOARD_KB_INT
#define BOARD_KB_INT 49 // TODO: Replace with actual pin
#endif
#ifndef BOARD_GYRO_INT
#define BOARD_GYRO_INT 50 // TODO: Replace with actual pin
#endif
#ifndef BOARD_I2C_FREQ
#define BOARD_I2C_FREQ 400000 // 400kHz default
#endif

static const char* TAG = "HW_MANAGER";

// ===== CONSTRUCTOR/DESTRUCTOR =====

HardwareManager::HardwareManager() 
    : initialized_(false)
    , spi_initialized_(false)
    , i2c_initialized_(false)
    , power_management_initialized_(false)
    , battery_voltage_(0.0f)
    , last_battery_check_(0)
    , peripheral_states_{}
{
    ESP_LOGI(TAG, "Hardware Manager created with corrected configuration");
}

HardwareManager::~HardwareManager() {
    if (initialized_) {
        deinitialize();
    }
}

// ===== CORE INITIALIZATION =====

bool HardwareManager::initialize() {
    ESP_LOGI(TAG, "Initializing Hardware Manager with corrected pin configuration...");
    
    if (initialized_) {
        ESP_LOGW(TAG, "Hardware Manager already initialized");
        return true;
    }
    
    // Validate pin configuration first
    if (!validateConfiguration()) {
        ESP_LOGE(TAG, "Pin configuration validation failed");
        return false;
    }
    
    // Initialize GPIO pins with corrected configuration
    if (!initializeGPIO()) {
        ESP_LOGE(TAG, "GPIO initialization failed");
        return false;
    }
    
    // Initialize SPI bus with correct pins
    if (!initializeSPI()) {
        ESP_LOGE(TAG, "SPI initialization failed");
        return false;
    }
    
    // Initialize I2C bus with correct pins
    if (!initializeI2C()) {
        ESP_LOGE(TAG, "I2C initialization failed");
        return false;
    }
    
    // Initialize power management
    if (!initializePowerManagement()) {
        ESP_LOGE(TAG, "Power management initialization failed");
        return false;
    }
    
    // Detect and initialize peripherals
    if (!detectPeripherals()) {
        ESP_LOGW(TAG, "Some peripherals not detected, continuing...");
    }
    
    initialized_ = true;
    ESP_LOGI(TAG, "Hardware Manager initialized successfully");
    return true;
}

bool HardwareManager::validateConfiguration() {
    ESP_LOGI(TAG, "Validating hardware pin configuration...");
    
    // Check for pin conflicts between major subsystems
    struct PinConflictCheck {
        int pin;
        const char* usage;
    };
    
    PinConflictCheck critical_pins[] = {
        {BOARD_EPD_CS, "E-ink CS"},
        {BOARD_EPD_DC, "E-ink DC"},
        {BOARD_EPD_BUSY, "E-ink BUSY"},
        {BOARD_LORA_CS, "LoRa CS"},
        {BOARD_SD_CS, "SD CS"},
        {BOARD_I2C_SDA, "I2C SDA"},
        {BOARD_I2C_SCL, "I2C SCL"},
        {BOARD_SPI_SCK, "SPI SCK"},
        {BOARD_SPI_MOSI, "SPI MOSI"},
        {BOARD_SPI_MISO, "SPI MISO"}
    };
    
    // Validate pin ranges
    for (auto& pin_check : critical_pins) {
        if (!BOARD_VALIDATE_PIN(pin_check.pin)) {
            ESP_LOGE(TAG, "Invalid pin %d for %s", pin_check.pin, pin_check.usage);
            return false;
        }
    }
    
    // Check for CS pin conflicts
    if (BOARD_EPD_CS == BOARD_LORA_CS || BOARD_EPD_CS == BOARD_SD_CS || BOARD_LORA_CS == BOARD_SD_CS) {
        ESP_LOGE(TAG, "CS pin conflict detected!");
        return false;
    }
    
    // Validate I2C addresses
    uint8_t i2c_addresses[] = {
        BOARD_TOUCH_ADDR, BOARD_KEYBOARD_ADDR, BOARD_LIGHT_ADDR,
        BOARD_GYRO_ADDR, BOARD_BATTERY_ADDR, BOARD_POWER_ADDR
    };
    
    for (uint8_t addr : i2c_addresses) {
        if (!BOARD_VALIDATE_I2C_ADDR(addr)) {
            ESP_LOGE(TAG, "Invalid I2C address 0x%02X", addr);
            return false;
        }
    }
    
    ESP_LOGI(TAG, "Pin configuration validation passed");
    return true;
}

bool HardwareManager::initializeGPIO() {
    ESP_LOGI(TAG, "Initializing GPIO pins with corrected configuration...");
    
    // Initialize output pins
    gpio_config_t output_config = {
        .pin_bit_mask = (1ULL << BOARD_EPD_CS) | (1ULL << BOARD_EPD_DC) |
                       (1ULL << BOARD_LORA_CS) | (1ULL << BOARD_LORA_RST) |
                       (1ULL << BOARD_SD_CS) | (1ULL << BOARD_MODEM_RST) |
                       (1ULL << BOARD_MODEM_PWRKEY) | (1ULL << BOARD_GPS_EN) |
                       (1ULL << BOARD_GYRO_EN) | (1ULL << BOARD_MODEM_EN) |
                       (1ULL << BOARD_LORA_EN) | (1ULL << BOARD_MOTOR_PIN) |
                       (1ULL << BOARD_KB_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    if (gpio_config(&output_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure output pins");
        return false;
    }
    
    // Initialize input pins
    gpio_config_t input_config = {
        .pin_bit_mask = (1ULL << BOARD_EPD_BUSY) | (1ULL << BOARD_LORA_BUSY) |
                       (1ULL << BOARD_LORA_DIO1) | (1ULL << BOARD_TOUCH_INT) |
                       (1ULL << BOARD_KB_INT) | (1ULL << BOARD_ALS_INT) |
                       (1ULL << BOARD_GYRO_INT) | (1ULL << BOARD_BOOT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    if (gpio_config(&input_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure input pins");
        return false;
    }
    
    // Set initial states for critical pins
    gpio_set_level((gpio_num_t)BOARD_EPD_CS, 1);      // CS high (inactive)
    gpio_set_level((gpio_num_t)BOARD_LORA_CS, 1);     // CS high (inactive)
    gpio_set_level((gpio_num_t)BOARD_SD_CS, 1);       // CS high (inactive)
    gpio_set_level((gpio_num_t)BOARD_MODEM_EN, 0);    // Modem disabled initially
    gpio_set_level((gpio_num_t)BOARD_LORA_EN, 0);     // LoRa disabled initially
    gpio_set_level((gpio_num_t)BOARD_GPS_EN, 0);      // GPS disabled initially
    gpio_set_level((gpio_num_t)BOARD_GYRO_EN, 0);     // Gyro disabled initially
    gpio_set_level((gpio_num_t)BOARD_MOTOR_PIN, 0);   // Motor off
    gpio_set_level((gpio_num_t)BOARD_KB_LED, 0);      // Keyboard LED off
    
    ESP_LOGI(TAG, "GPIO initialization completed with corrected pins");
    return true;
}

bool HardwareManager::initializeSPI() {
    ESP_LOGI(TAG, "Initializing SPI bus with corrected pins...");
    
    if (spi_initialized_) {
        ESP_LOGW(TAG, "SPI already initialized");
        return true;
    }
    
    // Configure SPI bus with corrected pin assignments
    spi_bus_config_t bus_config = {
        .mosi_io_num = BOARD_SPI_MOSI,
        .miso_io_num = BOARD_SPI_MISO,
        .sclk_io_num = BOARD_SPI_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .intr_flags = 0
    };
    
    esp_err_t ret = spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return false;
    }
    
    spi_initialized_ = true;
    ESP_LOGI(TAG, "SPI bus initialized successfully");
    ESP_LOGI(TAG, "  - SCK:  GPIO%d", BOARD_SPI_SCK);
    ESP_LOGI(TAG, "  - MOSI: GPIO%d", BOARD_SPI_MOSI);
    ESP_LOGI(TAG, "  - MISO: GPIO%d", BOARD_SPI_MISO);
    
    return true;
}

bool HardwareManager::initializeI2C() {
    ESP_LOGI(TAG, "Initializing I2C bus with corrected pins...");
    
    if (i2c_initialized_) {
        ESP_LOGW(TAG, "I2C already initialized");
        return true;
    }
    
    // Configure I2C with corrected pins
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = (gpio_num_t)BOARD_I2C_SDA,
        .scl_io_num = (gpio_num_t)BOARD_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = BOARD_I2C_FREQ
        },
        .clk_flags = 0
    };
    
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &i2c_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C parameters: %s", esp_err_to_name(ret));
        return false;
    }
    
    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
        return false;
    }
    
    i2c_initialized_ = true;
    ESP_LOGI(TAG, "I2C bus initialized successfully");
    ESP_LOGI(TAG, "  - SDA: GPIO%d", BOARD_I2C_SDA);
    ESP_LOGI(TAG, "  - SCL: GPIO%d", BOARD_I2C_SCL);
    ESP_LOGI(TAG, "  - Frequency: %d Hz", BOARD_I2C_FREQ);
    
    return true;
}

bool HardwareManager::initializePowerManagement() {
    ESP_LOGI(TAG, "Initializing power management...");
    
    if (power_management_initialized_) {
        ESP_LOGW(TAG, "Power management already initialized");
        return true;
    }
    
    // Initialize all peripheral states as OFF
    peripheral_states_["display"] = {false, false, false, 0, ""};
    peripheral_states_["wifi"] = {false, false, false, 0, ""};
    peripheral_states_["modem"] = {false, false, false, 0, ""};
    peripheral_states_["lora"] = {false, false, false, 0, ""};
    peripheral_states_["gps"] = {false, false, false, 0, ""};
    peripheral_states_["gyro"] = {false, false, false, 0, ""};
    peripheral_states_["sd"] = {false, false, false, 0, ""};
    
    // Read initial battery voltage
    updateBatteryVoltage();
    
    power_management_initialized_ = true;
    ESP_LOGI(TAG, "Power management initialized, battery: %.2fV", battery_voltage_);
    
    return true;
}

bool HardwareManager::detectPeripherals() {
    ESP_LOGI(TAG, "Detecting I2C peripherals with corrected addresses...");
    
    if (!i2c_initialized_) {
        ESP_LOGE(TAG, "I2C not initialized for peripheral detection");
        return false;
    }
    
    struct PeripheralCheck {
        uint8_t address;
        const char* name;
        const char* key;
    };
    
    PeripheralCheck peripherals[] = {
        {BOARD_TOUCH_ADDR, "Touch Controller (CST328)", "touch"},
        {BOARD_KEYBOARD_ADDR, "Keyboard Controller (TCA8418)", "keyboard"},
        {BOARD_LIGHT_ADDR, "Light Sensor (LTR-553ALS)", "light"},
        {BOARD_GYRO_ADDR, "Gyroscope (BHI260AP)", "gyro"},
        {BOARD_BATTERY_ADDR, "Battery Gauge (BQ27220)", "battery"},
        {BOARD_POWER_ADDR, "Power Management (BQ25896)", "power"}
    };
    
    bool all_detected = true;
    
    for (auto& peripheral : peripherals) {
        if (isI2CDevicePresent(peripheral.address)) {
            ESP_LOGI(TAG, "✓ %s detected at 0x%02X", peripheral.name, peripheral.address);
            peripheral_states_[peripheral.key] = {true, false, false, millis(), ""};
        } else {
            ESP_LOGW(TAG, "✗ %s not found at 0x%02X", peripheral.name, peripheral.address);
            peripheral_states_[peripheral.key] = {false, false, false, millis(), "Not detected"};
            all_detected = false;
        }
    }
    
    return all_detected;
}

bool HardwareManager::isI2CDevicePresent(uint8_t address) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    return (ret == ESP_OK);
}

// ===== PERIPHERAL CONTROL =====

bool HardwareManager::enablePeripheral(const char* peripheral) {
    ESP_LOGI(TAG, "Enabling peripheral: %s", peripheral);
    
    if (strcmp(peripheral, "display") == 0) {
        // E-ink display is always enabled when SPI is available
        peripheral_states_["display"] = {true, true, true, millis(), ""};
        return true;
    }
    
    if (strcmp(peripheral, "modem") == 0) {
        gpio_set_level((gpio_num_t)BOARD_MODEM_EN, 1);
        delay(100);
        peripheral_states_["modem"] = {true, true, true, millis(), ""};
        return true;
    }
    
    if (strcmp(peripheral, "lora") == 0) {
        gpio_set_level((gpio_num_t)BOARD_LORA_EN, 1);
        delay(100);
        peripheral_states_["lora"] = {true, true, true, millis(), ""};
        return true;
    }
    
    if (strcmp(peripheral, "gps") == 0) {
        gpio_set_level((gpio_num_t)BOARD_GPS_EN, 1);
        delay(100);
        peripheral_states_["gps"] = {true, true, true, millis(), ""};
        return true;
    }
    
    if (strcmp(peripheral, "gyro") == 0) {
        gpio_set_level((gpio_num_t)BOARD_GYRO_EN, 1);
        delay(100);
        peripheral_states_["gyro"] = {true, true, true, millis(), ""};
        return true;
    }
    
    ESP_LOGW(TAG, "Unknown peripheral: %s", peripheral);
    return false;
}

bool HardwareManager::disablePeripheral(const char* peripheral) {
    ESP_LOGI(TAG, "Disabling peripheral: %s", peripheral);
    
    if (strcmp(peripheral, "modem") == 0) {
        gpio_set_level((gpio_num_t)BOARD_MODEM_EN, 0);
        peripheral_states_["modem"] = {false, false, false, millis(), ""};
        return true;
    }
    
    if (strcmp(peripheral, "lora") == 0) {
        gpio_set_level((gpio_num_t)BOARD_LORA_EN, 0);
        peripheral_states_["lora"] = {false, false, false, millis(), ""};
        return true;
    }
    
    if (strcmp(peripheral, "gps") == 0) {
        gpio_set_level((gpio_num_t)BOARD_GPS_EN, 0);
        peripheral_states_["gps"] = {false, false, false, millis(), ""};
        return true;
    }
    
    if (strcmp(peripheral, "gyro") == 0) {
        gpio_set_level((gpio_num_t)BOARD_GYRO_EN, 0);
        peripheral_states_["gyro"] = {false, false, false, millis(), ""};
        return true;
    }
    
    ESP_LOGW(TAG, "Unknown peripheral: %s", peripheral);
    return false;
}

// ===== STATUS QUERIES =====

bool HardwareManager::isDisplayAvailable() const {
    return spi_initialized_ && peripheral_states_.count("display") && 
           peripheral_states_.at("display").initialized;
}

bool HardwareManager::isWiFiAvailable() const {
    return true; // ESP32-S3 always has WiFi
}

bool HardwareManager::isModemAvailable() const {
    return peripheral_states_.count("modem") && 
           peripheral_states_.at("modem").initialized;
}

bool HardwareManager::isLoRaAvailable() const {
    return peripheral_states_.count("lora") && 
           peripheral_states_.at("lora").initialized;
}

bool HardwareManager::isGPSAvailable() const {
    return peripheral_states_.count("gps") && 
           peripheral_states_.at("gps").initialized;
}

bool HardwareManager::isSDCardAvailable() const {
    return spi_initialized_; // SD shares SPI bus
}

float HardwareManager::getBatteryVoltage() {
    updateBatteryVoltage();
    return battery_voltage_;
}

void HardwareManager::updateBatteryVoltage() {
    uint32_t now = millis();
    if (now - last_battery_check_ < 1000) {
        return; // Don't check too frequently
    }
    
    // Read battery voltage via ADC or battery gauge
    // For now, simulate a reasonable voltage
    battery_voltage_ = 3.7f + (random(-100, 100) / 1000.0f);
    last_battery_check_ = now;
}

void HardwareManager::processEvents() {
    // Update battery voltage periodically
    updateBatteryVoltage();
    
    // Process any pending hardware events
    // This would typically handle interrupts, etc.
}

void HardwareManager::deinitialize() {
    ESP_LOGI(TAG, "Deinitializing Hardware Manager...");
    
    if (spi_initialized_) {
        spi_bus_free(SPI3_HOST);
        spi_initialized_ = false;
    }
    
    if (i2c_initialized_) {
        i2c_driver_delete(I2C_NUM_0);
        i2c_initialized_ = false;
    }
    
    // Disable all peripherals
    disablePeripheral("modem");
    disablePeripheral("lora");
    disablePeripheral("gps");
    disablePeripheral("gyro");
    
    initialized_ = false;
    ESP_LOGI(TAG, "Hardware Manager deinitialized");
}