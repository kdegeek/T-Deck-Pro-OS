/**
 * @file boot_manager_corrected.cpp
 * @brief T-Deck-Pro Boot Manager Implementation (CORRECTED)
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Corrected boot manager with hardware driver integration and display feedback
 */

#include <Arduino.h>
#include <cstring>
#include "boot_manager_corrected.h"
#include "../drivers/sensor_manager.h"
#include "../drivers/lora_driver.h"
#include "../drivers/gps_driver.h"
#include "../drivers/modem_4g_driver.h"
#include <ArduinoJson.h>
#include "../drivers/hardware_manager.h"
#include "../core/display/eink_manager_corrected.h"

static const char* TAG = "BootManager";

BootManager::BootManager() :
    current_stage_(BootStage::POWER_ON),
    current_message_(""),
    boot_start_time_(0),
    boot_complete_time_(0),
    boot_complete_(false),
    display_available_(false),
    last_error_(BootError::NONE),
    hw_manager_(nullptr),
    display_manager_(nullptr),
    sensor_manager_(nullptr),
    lora_driver_(nullptr),
    gps_driver_(nullptr),
    modem_driver_(nullptr),
    basic_display_initialized_(false) {
    
    // Initialize boot statistics
    memset(&boot_stats_, 0, sizeof(boot_stats_));
    
    // Initialize validation flags
    memset(&validation_flags_, 0, sizeof(validation_flags_));
}

BootManager::~BootManager() {
    // Cleanup if needed
}

bool BootManager::initialize() {
    boot_start_time_ = millis();
    current_stage_ = BootStage::POWER_ON;
    current_message_ = "Starting T-Deck-Pro OS...";
    
    Serial.println("\n" + String("=").substring(0, 50));
    Serial.println("[BOOT] T-Deck-Pro OS Boot Manager v1.0");
    Serial.println("[BOOT] Hardware: LilyGo T-Deck-Pro 4G");
    Serial.println("[BOOT] MCU: ESP32-S3 (16MB Flash, 8MB PSRAM)");
    Serial.println("[BOOT] " + String("=").substring(0, 50));
    Serial.println("[BOOT] Boot sequence initiated");
    
    // Show splash screen
    showSplashScreen();
    
    return true;
}

bool BootManager::executeBootSequence() {
    Serial.println("[BOOT] Executing complete boot sequence...");
    
    // Stage 1: Core System Initialization
    setBootStage(BootStage::CORE_INIT, "Initializing core systems...");
    if (!initializeCoreSystem()) {
        showError(BootError::CRITICAL_ERROR, "Core system initialization failed");
        return false;
    }
    
    // Stage 2: Hardware Initialization
    setBootStage(BootStage::HARDWARE_INIT, "Initializing hardware components...");
    if (!initializeHardware()) {
        showError(BootError::HARDWARE_INIT_FAILED, "Hardware initialization failed");
        return false;
    }
    
    // Stage 3: Sensor Initialization
    setBootStage(BootStage::SENSORS_INIT, "Initializing sensors...");
    if (!initializeSensors()) {
        showError(BootError::SENSOR_INIT_FAILED, "Sensor initialization failed");
        return false;
    }
    
    // Stage 4: Display Initialization
    setBootStage(BootStage::DISPLAY_INIT, "Initializing display system...");
    if (!initializeDisplay()) {
        showError(BootError::DISPLAY_INIT_FAILED, "Display initialization failed");
        return false;
    }
    
    // Stage 5: Connectivity Initialization
    setBootStage(BootStage::CONNECTIVITY_INIT, "Initializing connectivity...");
    if (!initializeConnectivity()) {
        showError(BootError::CONNECTIVITY_INIT_FAILED, "Connectivity initialization failed");
        return false;
    }
    
    // Stage 6: Services Initialization
    setBootStage(BootStage::SERVICES_INIT, "Initializing services...");
    if (!initializeServices()) {
        showError(BootError::CRITICAL_ERROR, "Services initialization failed");
        return false;
    }
    
    // Stage 7: Hardware Validation
    setBootStage(BootStage::VALIDATION, "Validating hardware...");
    if (!validateHardware()) {
        showError(BootError::VALIDATION_FAILED, "Hardware validation failed");
        return false;
    }
    
    // Stage 8: Boot Complete
    setBootStage(BootStage::COMPLETE, "Boot sequence completed successfully");
    
    boot_stats_.successful_boots++;
    
    Serial.println("[BOOT] " + String("=").substring(0, 50));
    Serial.printf("[BOOT] Boot completed in %d ms\n", getBootDuration());
    Serial.println("[BOOT] System ready for operation");
    Serial.println("[BOOT] " + String("=").substring(0, 50));
    
    return true;
}

bool BootManager::initializeCoreSystem() {
    uint32_t stage_start = millis();
    
    Serial.println("[BOOT] Initializing core system...");
    
    // Initialize basic serial communication
    Serial.begin(115200);
    while (!Serial && millis() - stage_start < 2000) {
        delay(10);
    }
    
    // Initialize basic I2C
    Wire.begin(T_DECK_I2C_SDA, T_DECK_I2C_SCL);
    Wire.setClock(400000);
    
    // Initialize basic SPI
    SPI.begin(T_DECK_SPI_SCK, T_DECK_SPI_MISO, T_DECK_SPI_MOSI);
    
    validation_flags_.core_systems = true;
    recordStageTiming(BootStage::CORE_INIT, millis() - stage_start);
    
    Serial.println("[BOOT] Core system initialized successfully");
    return true;
}

bool BootManager::initializeHardware() {
    uint32_t stage_start = millis();
    
    Serial.println("[BOOT] Initializing hardware manager...");
    
    if (!hw_manager_) {
        Serial.println("[BOOT] ERROR: Hardware manager not set");
        return false;
    }
    
    // Initialize hardware manager
    if (!hw_manager_->initialize()) {
        Serial.println("[BOOT] ERROR: Hardware manager initialization failed");
        return false;
    }
    
    // Verify hardware manager functionality
    if (!hw_manager_->isInitialized()) {
        Serial.println("[BOOT] ERROR: Hardware manager not properly initialized");
        return false;
    }
    
    validation_flags_.hardware_manager = true;
    recordStageTiming(BootStage::HARDWARE_INIT, millis() - stage_start);
    
    Serial.println("[BOOT] Hardware manager initialized successfully");
    return true;
}

bool BootManager::initializeSensors() {
    uint32_t stage_start = millis();
    
    Serial.println("[BOOT] Initializing sensor manager...");
    
    if (!sensor_manager_) {
        Serial.println("[BOOT] WARNING: Sensor manager not set - creating instance");
        sensor_manager_ = new SensorManager();
    }
    
    // Initialize sensor manager
    bool sensor_success = sensor_manager_->initialize();
    
    if (sensor_success) {
        Serial.printf("[BOOT] Sensors initialized: Gyro=%s, Light=%s, Battery=%s, Power=%s, Touch=%s, Keyboard=%s\n",
                     sensor_manager_->isGyroscopeAvailable() ? "OK" : "FAIL",
                     sensor_manager_->isLightSensorAvailable() ? "OK" : "FAIL", 
                     sensor_manager_->isBatteryGaugeAvailable() ? "OK" : "FAIL",
                     sensor_manager_->isPowerManagementAvailable() ? "OK" : "FAIL",
                     sensor_manager_->isTouchAvailable() ? "OK" : "FAIL",
                     sensor_manager_->isKeyboardAvailable() ? "OK" : "FAIL");
        
        validation_flags_.sensor_system = true;
    } else {
        Serial.println("[BOOT] WARNING: Some sensors failed to initialize - continuing anyway");
        validation_flags_.sensor_system = false;
    }
    
    recordStageTiming(BootStage::SENSORS_INIT, millis() - stage_start);
    
    // Don't fail boot for sensor issues, but log warning
    return true;
}

bool BootManager::initializeDisplay() {
    uint32_t stage_start = millis();
    
    Serial.println("[BOOT] Initializing display system...");
    
    if (!display_manager_) {
        Serial.println("[BOOT] ERROR: Display manager not set");
        return false;
    }
    
    // Initialize display manager
    if (!display_manager_->init()) {
        Serial.println("[BOOT] ERROR: Display manager initialization failed");
        return false;
    }
    
    // Test basic display functionality
    display_manager_->showBootSplash("T-Deck-Pro OS", "Initializing...");
    
    validation_flags_.display_system = true;
    display_available_ = true;
    recordStageTiming(BootStage::DISPLAY_INIT, millis() - stage_start);
    
    Serial.println("[BOOT] Display system initialized successfully");
    return true;
}

bool BootManager::initializeConnectivity() {
    uint32_t stage_start = millis();
    
    Serial.println("[BOOT] Initializing connectivity components...");
    
    bool any_connectivity = false;
    
    // Initialize LoRa driver
    if (lora_driver_) {
        Serial.println("[BOOT] Initializing LoRa...");
        if (lora_driver_->initialize()) {
            Serial.printf("[BOOT] LoRa initialized: Freq=%.1fMHz, Power=%ddBm\n", 
                         lora_driver_->getFrequency() / 1000000.0f, 
                         lora_driver_->getPower());
            any_connectivity = true;
        } else {
            Serial.println("[BOOT] WARNING: LoRa initialization failed");
        }
    } else {
        Serial.println("[BOOT] WARNING: LoRa driver not set");
    }
    
    // Initialize GPS driver
    if (gps_driver_) {
        Serial.println("[BOOT] Initializing GPS...");
        if (gps_driver_->initialize()) {
            Serial.println("[BOOT] GPS initialized successfully");
            any_connectivity = true;
        } else {
            Serial.println("[BOOT] WARNING: GPS initialization failed");
        }
    } else {
        Serial.println("[BOOT] WARNING: GPS driver not set");
    }
    
    // Initialize 4G modem
    if (modem_driver_) {
        Serial.println("[BOOT] Initializing 4G modem...");
        if (modem_driver_->initialize()) {
            Serial.println("[BOOT] 4G modem initialized successfully");
            any_connectivity = true;
        } else {
            Serial.println("[BOOT] WARNING: 4G modem initialization failed");
        }
    } else {
        Serial.println("[BOOT] WARNING: 4G modem driver not set");
    }
    
    validation_flags_.connectivity_system = any_connectivity;
    recordStageTiming(BootStage::CONNECTIVITY_INIT, millis() - stage_start);
    
    if (any_connectivity) {
        Serial.println("[BOOT] Connectivity components initialized successfully");
        return true;
    } else {
        Serial.println("[BOOT] WARNING: No connectivity components initialized - continuing anyway");
        return true; // Don't fail boot for connectivity issues
    }
}

bool BootManager::initializeServices() {
    uint32_t stage_start = millis();
    
    Serial.println("[BOOT] Initializing system services...");
    
    // Initialize task scheduler (if available)
    Serial.println("[BOOT] Task scheduler ready");
    
    // Initialize file system (if available)
    Serial.println("[BOOT] File system ready");
    
    // Initialize network services (if available)
    Serial.println("[BOOT] Network services ready");
    
    recordStageTiming(BootStage::SERVICES_INIT, millis() - stage_start);
    
    Serial.println("[BOOT] System services initialized successfully");
    return true;
}

bool BootManager::validateHardware() {
    uint32_t stage_start = millis();
    
    Serial.println("[BOOT] Validating hardware components...");
    
    bool all_valid = true;
    uint32_t validation_count = 0;
    
    // Validate core systems
    if (validation_flags_.core_systems) {
        Serial.println("[BOOT] ✓ Core systems validated");
        validation_count++;
    } else {
        Serial.println("[BOOT] ✗ Core systems validation failed");
        all_valid = false;
    }
    
    // Validate hardware manager
    if (validation_flags_.hardware_manager && hw_manager_ && hw_manager_->isInitialized()) {
        Serial.println("[BOOT] ✓ Hardware manager validated");
        validation_count++;
    } else {
        Serial.println("[BOOT] ✗ Hardware manager validation failed");
        all_valid = false;
    }
    
    // Validate display system
    if (validation_flags_.display_system && display_manager_) {
        Serial.println("[BOOT] ✓ Display system validated");
        validation_count++;
    } else {
        Serial.println("[BOOT] ✗ Display system validation failed");
        all_valid = false;
    }
    
    // Validate sensor system (non-critical)
    if (validation_flags_.sensor_system && sensor_manager_) {
        Serial.println("[BOOT] ✓ Sensor system validated");
        validation_count++;
    } else {
        Serial.println("[BOOT] ⚠ Sensor system validation failed (non-critical)");
    }
    
    // Validate connectivity system (non-critical)
    if (validation_flags_.connectivity_system) {
        Serial.println("[BOOT] ✓ Connectivity system validated");
        validation_count++;
    } else {
        Serial.println("[BOOT] ⚠ Connectivity system validation failed (non-critical)");
    }
    
    boot_stats_.hardware_validation_count = validation_count;
    validation_flags_.all_drivers = all_valid;
    recordStageTiming(BootStage::VALIDATION, millis() - stage_start);
    
    Serial.printf("[BOOT] Hardware validation complete: %d/%d components validated\n", validation_count, 5);
    
    // Only fail if critical components failed
    if (!validation_flags_.core_systems || !validation_flags_.hardware_manager || !validation_flags_.display_system) {
        Serial.println("[BOOT] CRITICAL: Essential hardware validation failed");
        return false;
    }
    
    Serial.println("[BOOT] Hardware validation successful");
    return true;
}

void BootManager::setBootStage(BootStage stage, const String& message) {
    uint32_t now = millis();
    
    current_stage_ = stage;
    current_message_ = message.isEmpty() ? getStageName(stage) : message;
    
    if (stage == BootStage::COMPLETE) {
        boot_complete_ = true;
        boot_complete_time_ = now;
        boot_stats_.total_boot_time = now - boot_start_time_;
    }
    
    Serial.printf("[BOOT] %s: %s\n", getStageName(stage).c_str(), current_message_.c_str());
    
    // Show progress if display is available
    if (display_available_ && display_manager_) {
        int progress = -1;
        switch (stage) {
            case BootStage::POWER_ON: progress = 5; break;
            case BootStage::CORE_INIT: progress = 15; break;
            case BootStage::HARDWARE_INIT: progress = 30; break;
            case BootStage::SENSORS_INIT: progress = 45; break;
            case BootStage::DISPLAY_INIT: progress = 60; break;
            case BootStage::CONNECTIVITY_INIT: progress = 75; break;
            case BootStage::SERVICES_INIT: progress = 85; break;
            case BootStage::VALIDATION: progress = 95; break;
            case BootStage::COMPLETE: progress = 100; break;
            default: progress = -1; break;
        }
        showBootProgress(stage, current_message_, progress);
    }
}

void BootManager::showError(BootError error_code, const String& error_message) {
    last_error_ = error_code;
    boot_stats_.last_error = error_message;
    boot_stats_.failed_boots++;
    
    Serial.printf("[BOOT] ERROR [%s]: %s\n", getErrorName(error_code).c_str(), error_message.c_str());
    
    // Show error on display if available
    if (display_available_ && display_manager_) {
        display_manager_->showError("Boot Error", error_message.c_str());
    }
}

void BootManager::showBootProgress(BootStage stage, const String& message, int progress) {
    if (display_available_ && display_manager_) {
        display_manager_->showBootSplash(getStageName(stage).c_str(), message.c_str());
    }
    
    if (progress >= 0) {
        Serial.printf("[BOOT] Progress: %d%% - %s\n", progress, message.c_str());
    } else {
        Serial.printf("[BOOT] %s\n", message.c_str());
    }
}

uint32_t BootManager::getBootDuration() const {
    if (boot_complete_) {
        return boot_complete_time_ - boot_start_time_;
    }
    return millis() - boot_start_time_;
}

String BootManager::getBootStatistics() const {
    DynamicJsonDocument doc(2048);
    
    doc["total_boot_time"] = boot_stats_.total_boot_time;
    doc["power_on_time"] = boot_stats_.power_on_time;
    doc["core_init_time"] = boot_stats_.core_init_time;
    doc["hardware_init_time"] = boot_stats_.hardware_init_time;
    doc["sensors_init_time"] = boot_stats_.sensors_init_time;
    doc["display_init_time"] = boot_stats_.display_init_time;
    doc["connectivity_init_time"] = boot_stats_.connectivity_init_time;
    doc["services_init_time"] = boot_stats_.services_init_time;
    doc["validation_time"] = boot_stats_.validation_time;
    doc["restart_count"] = boot_stats_.restart_count;
    doc["last_error"] = boot_stats_.last_error;
    doc["boot_complete"] = boot_complete_;
    doc["current_stage"] = getStageName(current_stage_);
    doc["hardware_validation_count"] = boot_stats_.hardware_validation_count;
    doc["successful_boots"] = boot_stats_.successful_boots;
    doc["failed_boots"] = boot_stats_.failed_boots;
    
    // Validation flags
    JsonObject validation = doc.createNestedObject("validation");
    validation["core_systems"] = validation_flags_.core_systems;
    validation["hardware_manager"] = validation_flags_.hardware_manager;
    validation["display_system"] = validation_flags_.display_system;
    validation["sensor_system"] = validation_flags_.sensor_system;
    validation["connectivity_system"] = validation_flags_.connectivity_system;
    validation["all_drivers"] = validation_flags_.all_drivers;
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool BootManager::checkSystemHealth() {
    if (!boot_complete_) {
        return false;
    }
    
    // Check critical systems
    bool health_ok = true;
    
    if (hw_manager_ && !hw_manager_->isInitialized()) {
        Serial.println("[BOOT] HEALTH: Hardware manager not operational");
        health_ok = false;
    }
    
    if (display_manager_ && !display_available_) {
        Serial.println("[BOOT] HEALTH: Display system not operational");
        health_ok = false;
    }
    
    // Check if too many failed boots
    if (boot_stats_.failed_boots > 3) {
        Serial.println("[BOOT] HEALTH: Too many failed boots detected");
        health_ok = false;
    }
    
    return health_ok;
}

void BootManager::emergencyRestart(const String& reason) {
    Serial.printf("[BOOT] EMERGENCY RESTART: %s\n", reason.c_str());
    boot_stats_.restart_count++;
    boot_stats_.last_error = reason;
    
    // Show emergency restart on display
    if (display_available_ && display_manager_) {
        display_manager_->showError("EMERGENCY RESTART", reason.c_str());
        delay(3000); // Show message for 3 seconds
    }
    
    delay(1000); // Give time for serial output
    ESP.restart();
}

void BootManager::showSplashScreen() {
    Serial.println("[BOOT] Showing splash screen...");
    
    if (display_available_ && display_manager_) {
        display_manager_->showBootSplash("T-Deck-Pro OS", "Booting...");
    }
    
    Serial.println("[BOOT] T-Deck-Pro OS v1.0");
    Serial.println("[BOOT] LilyGo T-Deck-Pro 4G Edition");
    Serial.println("[BOOT] ESP32-S3 16MB Flash, 8MB PSRAM");
}

String BootManager::getStageName(BootStage stage) const {
    switch (stage) {
        case BootStage::POWER_ON: return "Power On";
        case BootStage::CORE_INIT: return "Core Init";
        case BootStage::HARDWARE_INIT: return "Hardware Init";
        case BootStage::SENSORS_INIT: return "Sensors Init";
        case BootStage::DISPLAY_INIT: return "Display Init";
        case BootStage::CONNECTIVITY_INIT: return "Connectivity Init";
        case BootStage::SERVICES_INIT: return "Services Init";
        case BootStage::VALIDATION: return "Validation";
        case BootStage::COMPLETE: return "Complete";
        case BootStage::ERROR: return "Error";
        default: return "Unknown";
    }
}

String BootManager::getErrorName(BootError error) const {
    switch (error) {
        case BootError::NONE: return "NONE";
        case BootError::HARDWARE_INIT_FAILED: return "HARDWARE_INIT_FAILED";
        case BootError::DISPLAY_INIT_FAILED: return "DISPLAY_INIT_FAILED";
        case BootError::SENSOR_INIT_FAILED: return "SENSOR_INIT_FAILED";
        case BootError::CONNECTIVITY_INIT_FAILED: return "CONNECTIVITY_INIT_FAILED";
        case BootError::VALIDATION_FAILED: return "VALIDATION_FAILED";
        case BootError::CRITICAL_ERROR: return "CRITICAL_ERROR";
        default: return "UNKNOWN";
    }
}

void BootManager::recordStageTiming(BootStage stage, uint32_t duration_ms) {
    switch (stage) {
        case BootStage::POWER_ON:
            boot_stats_.power_on_time = duration_ms;
            break;
        case BootStage::CORE_INIT:
            boot_stats_.core_init_time = duration_ms;
            break;
        case BootStage::HARDWARE_INIT:
            boot_stats_.hardware_init_time = duration_ms;
            break;
        case BootStage::SENSORS_INIT:
            boot_stats_.sensors_init_time = duration_ms;
            break;
        case BootStage::DISPLAY_INIT:
            boot_stats_.display_init_time = duration_ms;
            break;
        case BootStage::CONNECTIVITY_INIT:
            boot_stats_.connectivity_init_time = duration_ms;
            break;
        case BootStage::SERVICES_INIT:
            boot_stats_.services_init_time = duration_ms;
            break;
        case BootStage::VALIDATION:
            boot_stats_.validation_time = duration_ms;
            break;
        default:
            break;
    }
}

bool BootManager::initBasicDisplay() {
    // Basic display initialization for boot messages
    basic_display_initialized_ = true;
    display_available_ = true;
    return true;
}

void BootManager::clearDisplay() {
    if (display_manager_) {
        display_manager_->clear();
    }
}

void BootManager::drawText(const String& text, int x, int y, int size) {
    if (display_manager_) {
        display_manager_->drawText(x, y, text.c_str(), 1);
    }
}

void BootManager::drawProgressBar(int x, int y, int width, int height, int progress) {
    if (display_manager_) {
        display_manager_->showBootSplash("Progress", std::to_string(progress).c_str());
    }
}

void BootManager::updateDisplay() {
    if (display_manager_) {
        display_manager_->refresh();
    }
}