/**
 * @file boot_manager.h
 * @brief T-Deck-Pro Boot Manager - System initialization and boot sequence
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Handles complete system boot sequence with hardware initialization
 */

#ifndef BOOT_MANAGER_H
#define BOOT_MANAGER_H

#include <Arduino.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_partition.h>
#include <esp_heap_caps.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <SPIFFS.h>

#include "config/os_config_corrected.h"
#include "hal/board_config_corrected.h"

// ===== BOOT STATES =====
enum class BootState {
    INIT,
    HARDWARE_INIT,
    DISPLAY_INIT,
    STORAGE_INIT,
    COMMUNICATION_INIT,
    SERVICES_INIT,
    APPLICATIONS_INIT,
    READY,
    ERROR,
    EMERGENCY_MODE
};

// ===== BOOT CONFIGURATION =====
#define BOOT_TIMEOUT_MS 30000
#define BOOT_RETRY_COUNT 3
#define EMERGENCY_MODE_TIMEOUT_MS 10000

/**
 * @brief Boot manager for T-Deck-Pro OS
 * @note Handles complete system initialization sequence
 */
class BootManager {
public:
    BootManager();
    ~BootManager();
    
    /**
     * @brief Initialize boot manager
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Execute complete boot sequence
     * @return true if boot successful
     */
    bool boot();
    
    /**
     * @brief Get current boot state
     * @return Current boot state
     */
    BootState getState() const { return current_state_; }
    
    /**
     * @brief Get boot error message
     * @return Error message string
     */
    String getErrorMessage() const { return error_message_; }
    
    /**
     * @brief Get boot time in milliseconds
     * @return Boot time
     */
    uint32_t getBootTime() const { return boot_time_; }
    
    /**
     * @brief Check if system is ready
     * @return true if system ready
     */
    bool isReady() const { return current_state_ == BootState::READY; }
    
    /**
     * @brief Check if in emergency mode
     * @return true if emergency mode
     */
    bool isEmergencyMode() const { return current_state_ == BootState::EMERGENCY_MODE; }
    
    /**
     * @brief Enter emergency mode
     * @param reason Emergency mode reason
     */
    void enterEmergencyMode(const String& reason);
    
    /**
     * @brief Get system health status
     * @return Health status JSON string
     */
    String getHealthStatus();
    
    /**
     * @brief Perform system shutdown
     * @param reason Shutdown reason
     */
    void shutdown(const String& reason = "Normal shutdown");
    
    /**
     * @brief Perform system restart
     * @param reason Restart reason
     */
    void restart(const String& reason = "Normal restart");
    
    /**
     * @brief Get boot progress percentage
     * @return Progress 0-100
     */
    uint8_t getBootProgress() const;
    
    /**
     * @brief Update boot progress
     * @param progress Progress 0-100
     */
    void updateProgress(uint8_t progress);
    
    /**
     * @brief Set boot status message
     * @param message Status message
     */
    void setStatusMessage(const String& message);
    
    /**
     * @brief Get current status message
     * @return Status message
     */
    String getStatusMessage() const { return status_message_; }

private:
    // ===== BOOT SEQUENCE METHODS =====
    
    /**
     * @brief Initialize hardware components
     * @return true if successful
     */
    bool initializeHardware();
    
    /**
     * @brief Initialize display system
     * @return true if successful
     */
    bool initializeDisplay();
    
    /**
     * @brief Initialize storage systems
     * @return true if successful
     */
    bool initializeStorage();
    
    /**
     * @brief Initialize communication systems
     * @return true if successful
     */
    bool initializeCommunication();
    
    /**
     * @brief Initialize core services
     * @return true if successful
     */
    bool initializeServices();
    
    /**
     * @brief Initialize applications
     * @return true if successful
     */
    bool initializeApplications();
    
    /**
     * @brief Validate system configuration
     * @return true if valid
     */
    bool validateConfiguration();
    
    /**
     * @brief Check system resources
     * @return true if sufficient
     */
    bool checkSystemResources();
    
    /**
     * @brief Initialize GPIO pins
     * @return true if successful
     */
    bool initializeGPIO();
    
    /**
     * @brief Initialize I2C bus
     * @return true if successful
     */
    bool initializeI2C();
    
    /**
     * @brief Initialize SPI bus
     * @return true if successful
     */
    bool initializeSPI();
    
    /**
     * @brief Initialize UART interfaces
     * @return true if successful
     */
    bool initializeUART();
    
    /**
     * @brief Detect hardware components
     * @return true if successful
     */
    bool detectHardware();
    
    /**
     * @brief Initialize power management
     * @return true if successful
     */
    bool initializePowerManagement();
    
    /**
     * @brief Initialize sensors
     * @return true if successful
     */
    bool initializeSensors();
    
    /**
     * @brief Initialize connectivity modules
     * @return true if successful
     */
    bool initializeConnectivity();
    
    /**
     * @brief Set boot state
     * @param state New boot state
     */
    void setState(BootState state);
    
    /**
     * @brief Set error message
     * @param message Error message
     */
    void setError(const String& message);
    
    /**
     * @brief Log boot event
     * @param event Event description
     */
    void logBootEvent(const String& event);
    
    /**
     * @brief Check for emergency mode conditions
     * @return true if emergency mode needed
     */
    bool checkEmergencyMode();
    
    /**
     * @brief Perform emergency mode initialization
     * @return true if successful
     */
    bool initializeEmergencyMode();

private:
    // ===== MEMBER VARIABLES =====
    BootState current_state_;
    String error_message_;
    String status_message_;
    uint32_t boot_start_time_;
    uint32_t boot_time_;
    uint8_t boot_progress_;
    bool initialized_;
    bool emergency_mode_;
    
    // Hardware detection results
    bool display_detected_;
    bool wifi_detected_;
    bool modem_detected_;
    bool lora_detected_;
    bool gps_detected_;
    bool sd_card_detected_;
    bool battery_detected_;
    
    // System resource information
    size_t free_heap_;
    size_t free_psram_;
    size_t flash_size_;
    uint32_t cpu_freq_;
};

// ===== GLOBAL BOOT MANAGER INSTANCE =====
extern BootManager* g_boot_manager;

// ===== BOOT UTILITY FUNCTIONS =====

/**
 * @brief Initialize global boot manager
 * @return true if successful
 */
bool initializeBootManager();

/**
 * @brief Get global boot manager instance
 * @return Boot manager pointer
 */
BootManager* getBootManager();

/**
 * @brief System panic handler
 * @param reason Panic reason
 */
void systemPanic(const String& reason);

/**
 * @brief Emergency mode handler
 * @param reason Emergency reason
 */
void enterEmergencyMode(const String& reason);

#endif // BOOT_MANAGER_H 