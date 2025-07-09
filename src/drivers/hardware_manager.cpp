/**
 * @file hardware_manager.cpp
 * @brief T-Deck-Pro Hardware Manager Implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "hardware_manager.h"
#include <ArduinoJson.h>

static const char* TAG = "HardwareManager";

HardwareManager::HardwareManager() 
    : system_ready(false)
    , last_health_check(0)
    , initialization_start_time(0)
    , i2c_initialized(false)
    , spi_initialized(false) {
    
    // Define all hardware components
    components = {
        {"Power Management", HardwareStatus::NOT_INITIALIZED, "", true, 0},
        {"I2C Bus", HardwareStatus::NOT_INITIALIZED, "", true, 0},
        {"SPI Bus", HardwareStatus::NOT_INITIALIZED, "", true, 0},
        {"E-ink Display", HardwareStatus::NOT_INITIALIZED, "", true, 0},
        {"Touch Controller", HardwareStatus::NOT_INITIALIZED, "", true, 0},
        {"Keyboard", HardwareStatus::NOT_INITIALIZED, "", true, 0},
        {"Light Sensor", HardwareStatus::NOT_INITIALIZED, "", false, 0},
        {"Gyroscope", HardwareStatus::NOT_INITIALIZED, "", false, 0},
        {"WiFi", HardwareStatus::NOT_INITIALIZED, "", false, 0},
        {"4G Modem", HardwareStatus::NOT_INITIALIZED, "", false, 0},
        {"LoRa", HardwareStatus::NOT_INITIALIZED, "", false, 0},
        {"GPS", HardwareStatus::NOT_INITIALIZED, "", false, 0},
        {"Bluetooth", HardwareStatus::NOT_INITIALIZED, "", false, 0},
        {"SD Card", HardwareStatus::NOT_INITIALIZED, "", false, 0},
        {"Audio", HardwareStatus::NOT_INITIALIZED, "", false, 0}
    };
}

HardwareManager::~HardwareManager() {
    // Cleanup if needed
}

bool HardwareManager::initialize() {
    Serial.println("[HW] Starting hardware initialization...");
    initialization_start_time = millis();
    
    // Step 1: Initialize GPIO pins for proper SPI isolation
    // LORA, SD, EPD use the same SPI, avoid mutual influence
    pinMode(BOARD_EPD_CS, OUTPUT); 
    digitalWrite(BOARD_EPD_CS, HIGH);
    pinMode(BOARD_SD_CS, OUTPUT); 
    digitalWrite(BOARD_SD_CS, HIGH);
    pinMode(BOARD_LORA_CS, OUTPUT); 
    digitalWrite(BOARD_LORA_CS, HIGH);
    
    // Initialize control pins
    pinMode(BOARD_KEYBOARD_LED, OUTPUT);
    pinMode(BOARD_MOTOR_PIN, OUTPUT);
    pinMode(BOARD_6609_EN, OUTPUT);         // 4G module enable
    pinMode(BOARD_LORA_EN, OUTPUT);         // LoRa module enable
    pinMode(BOARD_GPS_EN, OUTPUT);          // GPS module enable
    pinMode(BOARD_1V8_EN, OUTPUT);          // Gyroscope module enable
    pinMode(BOARD_A7682E_PWRKEY, OUTPUT);   // 4G power key
    
    // Set initial states
    digitalWrite(BOARD_KEYBOARD_LED, LOW);
    digitalWrite(BOARD_MOTOR_PIN, LOW);
    digitalWrite(BOARD_6609_EN, HIGH);      // Enable 4G
    digitalWrite(BOARD_LORA_EN, HIGH);      // Enable LoRa
    digitalWrite(BOARD_GPS_EN, HIGH);       // Enable GPS
    digitalWrite(BOARD_1V8_EN, HIGH);       // Enable gyroscope
    
    // Step 2: Power Management (Critical)
    if (!init_power_management()) {
        Serial.println("[HW] CRITICAL: Power management failed!");
        return false;
    }
    
    // Step 3: Communication buses
    if (!init_i2c_bus()) {
        Serial.println("[HW] CRITICAL: I2C bus failed!");
        return false;
    }
    
    if (!init_spi_bus()) {
        Serial.println("[HW] CRITICAL: SPI bus failed!");
        return false;
    }
    
    // Step 4: Essential UI components
    if (!init_display()) {
        Serial.println("[HW] CRITICAL: Display failed!");
        return false;
    }
    
    if (!init_touch()) {
        Serial.println("[HW] CRITICAL: Touch controller failed!");
        return false;
    }
    
    if (!init_keyboard()) {
        Serial.println("[HW] CRITICAL: Keyboard failed!");
        return false;
    }
    
    // Step 5: Optional components (failures are non-critical)
    init_sensors();
    init_storage();
    init_audio();
    init_connectivity();
    
    // Check if all required components are ready
    system_ready = true;
    for (const auto& component : components) {
        if (component.required && component.status != HardwareStatus::READY) {
            system_ready = false;
            Serial.printf("[HW] Required component failed: %s\n", component.name.c_str());
        }
    }
    
    uint32_t init_time = millis() - initialization_start_time;
    Serial.printf("[HW] Hardware initialization completed in %lu ms\n", init_time);
    Serial.printf("[HW] System ready: %s\n", system_ready ? "YES" : "NO");
    
    return system_ready;
}

bool HardwareManager::init_power_management() {
    set_component_status("Power Management", HardwareStatus::INITIALIZING);
    
    // Initialize BQ25896 power management IC
    Wire.beginTransmission(BOARD_I2C_ADDR_BQ25896);
    if (Wire.endTransmission() == 0) {
        // Power management IC found, configure it
        // Note: This is a simplified version, full implementation would use XPowersLib
        Serial.println("[HW] BQ25896 power management IC detected");
        set_component_status("Power Management", HardwareStatus::READY);
        return true;
    } else {
        set_component_status("Power Management", HardwareStatus::ERROR, "BQ25896 not found");
        return false;
    }
}

bool HardwareManager::init_i2c_bus() {
    set_component_status("I2C Bus", HardwareStatus::INITIALIZING);
    
    if (!i2c_initialized) {
        Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
        Wire.setClock(400000); // 400kHz
        i2c_initialized = true;
    }
    
    // Test I2C bus by scanning for known devices
    int devices_found = 0;
    uint8_t known_addresses[] = {
        BOARD_I2C_ADDR_TOUCH,
        BOARD_I2C_ADDR_KEYBOARD,
        BOARD_I2C_ADDR_BQ25896,
        BOARD_I2C_ADDR_BQ27220
    };
    
    for (uint8_t addr : known_addresses) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            devices_found++;
        }
    }
    
    if (devices_found > 0) {
        Serial.printf("[HW] I2C bus ready, found %d devices\n", devices_found);
        set_component_status("I2C Bus", HardwareStatus::READY);
        return true;
    } else {
        set_component_status("I2C Bus", HardwareStatus::ERROR, "No I2C devices found");
        return false;
    }
}

bool HardwareManager::init_spi_bus() {
    set_component_status("SPI Bus", HardwareStatus::INITIALIZING);
    
    if (!spi_initialized) {
        SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
        spi_initialized = true;
    }
    
    Serial.println("[HW] SPI bus initialized");
    set_component_status("SPI Bus", HardwareStatus::READY);
    return true;
}

bool HardwareManager::init_display() {
    set_component_status("E-ink Display", HardwareStatus::INITIALIZING);
    
    // E-ink display initialization will be handled by display_driver.cpp
    // For now, just mark as ready if SPI is working
    if (spi_initialized) {
        Serial.println("[HW] E-ink display ready for initialization");
        set_component_status("E-ink Display", HardwareStatus::READY);
        return true;
    } else {
        set_component_status("E-ink Display", HardwareStatus::ERROR, "SPI not ready");
        return false;
    }
}

bool HardwareManager::init_touch() {
    set_component_status("Touch Controller", HardwareStatus::INITIALIZING);
    
    // Check for touch controller on I2C
    Wire.beginTransmission(BOARD_I2C_ADDR_TOUCH);
    if (Wire.endTransmission() == 0) {
        Serial.println("[HW] Touch controller (CST328) detected");
        set_component_status("Touch Controller", HardwareStatus::READY);
        return true;
    } else {
        set_component_status("Touch Controller", HardwareStatus::ERROR, "CST328 not found");
        return false;
    }
}

bool HardwareManager::init_keyboard() {
    set_component_status("Keyboard", HardwareStatus::INITIALIZING);
    
    // Check for keyboard controller on I2C
    Wire.beginTransmission(BOARD_I2C_ADDR_KEYBOARD);
    if (Wire.endTransmission() == 0) {
        Serial.println("[HW] Keyboard controller (TCA8418) detected");
        set_component_status("Keyboard", HardwareStatus::READY);
        return true;
    } else {
        set_component_status("Keyboard", HardwareStatus::ERROR, "TCA8418 not found");
        return false;
    }
}

bool HardwareManager::init_sensors() {
    // Light sensor
    set_component_status("Light Sensor", HardwareStatus::INITIALIZING);
    Wire.beginTransmission(BOARD_I2C_ADDR_LTR_553ALS);
    if (Wire.endTransmission() == 0) {
        Serial.println("[HW] Light sensor (LTR-553ALS) detected");
        set_component_status("Light Sensor", HardwareStatus::READY);
    } else {
        set_component_status("Light Sensor", HardwareStatus::ERROR, "LTR-553ALS not found");
    }
    
    // Gyroscope
    set_component_status("Gyroscope", HardwareStatus::INITIALIZING);
    Wire.beginTransmission(BOARD_I2C_ADDR_GYROSCOPE);
    if (Wire.endTransmission() == 0) {
        Serial.println("[HW] Gyroscope (BHI260AP) detected");
        set_component_status("Gyroscope", HardwareStatus::READY);
    } else {
        set_component_status("Gyroscope", HardwareStatus::ERROR, "BHI260AP not found");
    }
    
    return true; // Non-critical
}

bool HardwareManager::init_connectivity() {
    // WiFi
    set_component_status("WiFi", HardwareStatus::INITIALIZING);
    // WiFi initialization will be handled by connectivity_driver.cpp
    set_component_status("WiFi", HardwareStatus::READY);
    
    // 4G Modem
    set_component_status("4G Modem", HardwareStatus::INITIALIZING);
    // 4G initialization will be handled by connectivity_driver.cpp
    set_component_status("4G Modem", HardwareStatus::READY);
    
    // LoRa
    set_component_status("LoRa", HardwareStatus::INITIALIZING);
    // LoRa initialization will be handled by connectivity_driver.cpp
    set_component_status("LoRa", HardwareStatus::READY);
    
    // GPS
    set_component_status("GPS", HardwareStatus::INITIALIZING);
    // GPS initialization will be handled by connectivity_driver.cpp
    set_component_status("GPS", HardwareStatus::READY);
    
    // Bluetooth
    set_component_status("Bluetooth", HardwareStatus::INITIALIZING);
    // Bluetooth initialization will be handled by connectivity_driver.cpp
    set_component_status("Bluetooth", HardwareStatus::READY);
    
    return true;
}

bool HardwareManager::init_storage() {
    set_component_status("SD Card", HardwareStatus::INITIALIZING);
    // SD card initialization will be handled by storage_driver.cpp
    set_component_status("SD Card", HardwareStatus::READY);
    return true;
}

bool HardwareManager::init_audio() {
    set_component_status("Audio", HardwareStatus::INITIALIZING);
    // Audio initialization will be handled by audio driver
    set_component_status("Audio", HardwareStatus::READY);
    return true;
}

void HardwareManager::update() {
    uint32_t now = millis();
    
    if (now - last_health_check > HEALTH_CHECK_INTERVAL) {
        last_health_check = now;
        
        check_power_status();
        check_sensor_status();
        check_connectivity_status();
    }
}

void HardwareManager::check_power_status() {
    // Update power-related status
    // This would check battery levels, charging status, etc.
}

void HardwareManager::check_sensor_status() {
    // Update sensor status
    // This would check if sensors are still responding
}

void HardwareManager::check_connectivity_status() {
    // Update connectivity status
    // This would check WiFi, 4G, LoRa, etc.
}

HardwareStatus HardwareManager::get_component_status(const String& component_name) {
    HardwareComponent* component = find_component(component_name);
    return component ? component->status : HardwareStatus::NOT_INITIALIZED;
}

String HardwareManager::get_health_summary() {
    DynamicJsonDocument doc(2048);
    JsonArray components_array = doc.createNestedArray("components");
    
    for (const auto& component : components) {
        JsonObject comp_obj = components_array.createNestedObject();
        comp_obj["name"] = component.name;
        comp_obj["status"] = static_cast<int>(component.status);
        comp_obj["required"] = component.required;
        if (component.error_message.length() > 0) {
            comp_obj["error"] = component.error_message;
        }
    }
    
    doc["system_ready"] = system_ready;
    doc["uptime"] = millis();
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool HardwareManager::is_system_ready() {
    return system_ready;
}

uint16_t HardwareManager::get_battery_voltage() {
    // This would read from the BQ27220 battery gauge
    // For now, return a placeholder
    return 3700; // 3.7V
}

bool HardwareManager::is_usb_connected() {
    // This would check the power management IC
    // For now, return a placeholder
    return true;
}

float HardwareManager::get_temperature() {
    // This would read from the gyroscope or other temperature sensor
    // For now, return a placeholder
    return 25.0; // 25°C
}

bool HardwareManager::set_component_enabled(const String& component_name, bool enable) {
    HardwareComponent* component = find_component(component_name);
    if (!component) return false;
    
    if (enable) {
        component->status = HardwareStatus::READY;
    } else {
        component->status = HardwareStatus::DISABLED;
    }
    
    return true;
}

void HardwareManager::set_component_status(const String& name, HardwareStatus status, const String& error) {
    HardwareComponent* component = find_component(name);
    if (component) {
        component->status = status;
        component->error_message = error;
        component->last_check = millis();
    }
}

HardwareComponent* HardwareManager::find_component(const String& name) {
    for (auto& component : components) {
        if (component.name == name) {
            return &component;
        }
    }
    return nullptr;
}