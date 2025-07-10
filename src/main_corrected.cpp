/**
 * @file main_corrected.cpp
 * @brief T-Deck-Pro Main Application - Complete OS Integration
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Main application integrating all core components and applications
 */

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// Core system includes
#include "core/utils/logger.h"
#include "core/storage/storage_manager.h"
#include "core/launcher.h"

// App includes
#include "apps/file_manager_app.h"
#include "apps/meshtastic_app.h"
#include "apps/settings_app.h"

// Hardware manager includes
#include "core/display/eink_manager_corrected.h"
#include "core/input/keyboard_manager.h"
#include "core/communication/lora_manager.h"

// Test app includes
#include "apps/hardware_test_app.h"

// ===== GLOBAL SYSTEM VARIABLES =====
bool system_initialized = false;
uint32_t system_start_time = 0;
uint32_t last_heartbeat = 0;

// ===== SYSTEM INITIALIZATION =====

bool initializeSystem() {
    logInfo("SYSTEM", "Initializing T-Deck-Pro OS");
    
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);
    
    logInfo("SYSTEM", "Serial communication initialized");
    
    // Initialize logger
    if (!initializeLogger()) {
        logError("SYSTEM", "Failed to initialize logger");
        return false;
    }
    
    logInfo("SYSTEM", "Logger initialized");
    
    // Initialize storage manager
    if (!initializeStorageManager()) {
        logError("SYSTEM", "Failed to initialize storage manager");
        return false;
    }
    
    logInfo("SYSTEM", "Storage manager initialized");
    
    // Initialize launcher
    if (!initializeLauncher()) {
        logError("SYSTEM", "Failed to initialize launcher");
        return false;
    }
    
    logInfo("SYSTEM", "Launcher initialized");
    
    // Initialize hardware managers
    if (!initializeEinkManager()) {
        logError("SYSTEM", "Failed to initialize E-ink display manager");
        return false;
    }
    
    if (!initializeKeyboardManager()) {
        logError("SYSTEM", "Failed to initialize keyboard manager");
        return false;
    }
    
    if (!initializeLoRaManager()) {
        logError("SYSTEM", "Failed to initialize LoRa manager");
        return false;
    }
    
    logInfo("SYSTEM", "Hardware managers initialized");
    
    // Initialize core applications
    if (!initializeFileManagerApp()) {
        logError("SYSTEM", "Failed to initialize file manager app");
        return false;
    }
    
    if (!initializeMeshtasticApp()) {
        logError("SYSTEM", "Failed to initialize meshtastic app");
        return false;
    }
    
    if (!initializeSettingsApp()) {
        logError("SYSTEM", "Failed to initialize settings app");
        return false;
    }
    
    logInfo("SYSTEM", "Core applications initialized");
    
    // Initialize hardware test app
    if (!initializeHardwareTestApp()) {
        logError("SYSTEM", "Failed to initialize hardware test app");
        return false;
    }
    
    logInfo("SYSTEM", "Hardware test app initialized");
    
    system_initialized = true;
    system_start_time = millis();
    
    logInfo("SYSTEM", "T-Deck-Pro OS initialization complete");
    return true;
}

void processSystem() {
    if (!system_initialized) {
        if (!initializeSystem()) {
            logCritical("SYSTEM", "System initialization failed");
            return;
        }
    }
    
    // Process launcher (handles app switching and UI)
    Launcher* launcher = getLauncher();
    if (launcher) {
        launcher->process();
    }
    
    // Process logger
    Logger* logger = getLogger();
    if (logger) {
        logger->process();
    }
    
    // Process storage manager
    StorageManager* storage = getStorageManager();
    if (storage) {
        storage->process();
    }
    
    // Process hardware managers
    EinkManager* eink = getEinkManager();
    if (eink) {
        eink->process();
    }
    
    KeyboardManager* keyboard = getKeyboardManager();
    if (keyboard) {
        keyboard->process();
    }
    
    LoRaManager* lora = getLoRaManager();
    if (lora) {
        lora->process();
    }
    
    // Process hardware test app
    HardwareTestApp* hw_test = getHardwareTestApp();
    if (hw_test) {
        hw_test->process();
    }
    
    // System heartbeat
    uint32_t now = millis();
    if (now - last_heartbeat > 30000) { // Every 30 seconds
        logInfo("SYSTEM", "System heartbeat - Uptime: " + String(now / 1000) + "s");
        last_heartbeat = now;
    }
}

// ===== ARDUINO SETUP AND LOOP =====

void setup() {
    // Initialize system
    if (!initializeSystem()) {
        Serial.println("FATAL: System initialization failed");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("T-Deck-Pro OS Ready");
}

void loop() {
    // Main system processing loop
    processSystem();
    
    // Small delay to prevent watchdog issues
    delay(10);
}

// ===== SYSTEM UTILITY FUNCTIONS =====

String getSystemStatus() {
    DynamicJsonDocument doc(2048);
    
    doc["system_initialized"] = system_initialized;
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["min_free_heap"] = ESP.getMinFreeHeap();
    doc["max_alloc_heap"] = ESP.getMaxAllocHeap();
    
    // Get launcher status
    Launcher* launcher = getLauncher();
    if (launcher) {
        JsonObject launcher_status = doc.createNestedObject("launcher");
        DynamicJsonDocument launcher_doc(1024);
        launcher_doc.parse(launcher->getLauncherStatus());
        launcher_status.set(launcher_doc);
    }
    
    // Get logger status
    Logger* logger = getLogger();
    if (logger) {
        JsonObject logger_status = doc.createNestedObject("logger");
        DynamicJsonDocument logger_doc(1024);
        logger_doc.parse(logger->getStatistics());
        logger_status.set(logger_doc);
    }
    
    // Get storage status
    StorageManager* storage = getStorageManager();
    if (storage) {
        JsonObject storage_status = doc.createNestedObject("storage");
        StorageStats spiffs_stats = storage->getStats(StorageType::SPIFFS);
        StorageStats sd_stats = storage->getStats(StorageType::SD_CARD);
        
        JsonObject spiffs = storage_status.createNestedObject("spiffs");
        spiffs["total_space"] = spiffs_stats.total_space;
        spiffs["used_space"] = spiffs_stats.used_space;
        spiffs["free_space"] = spiffs_stats.free_space;
        spiffs["file_count"] = spiffs_stats.file_count;
        
        JsonObject sd = storage_status.createNestedObject("sd_card");
        sd["total_space"] = sd_stats.total_space;
        sd["used_space"] = sd_stats.used_space;
        sd["free_space"] = sd_stats.free_space;
        sd["file_count"] = sd_stats.file_count;
    }
    
    // Get hardware manager status
    EinkManager* eink = getEinkManager();
    if (eink) {
        JsonObject eink_status = doc.createNestedObject("eink");
        DynamicJsonDocument eink_doc(1024);
        eink_doc.parse(eink->getDisplayStatus());
        eink_status.set(eink_doc);
    }
    
    KeyboardManager* keyboard = getKeyboardManager();
    if (keyboard) {
        JsonObject keyboard_status = doc.createNestedObject("keyboard");
        DynamicJsonDocument keyboard_doc(1024);
        keyboard_doc.parse(keyboard->getStatus());
        keyboard_status.set(keyboard_doc);
    }
    
    LoRaManager* lora = getLoRaManager();
    if (lora) {
        JsonObject lora_status = doc.createNestedObject("lora");
        DynamicJsonDocument lora_doc(1024);
        lora_doc.parse(lora->getStatus());
        lora_status.set(lora_doc);
    }
    
    // Get hardware test app status
    HardwareTestApp* hw_test = getHardwareTestApp();
    if (hw_test) {
        JsonObject hw_test_status = doc.createNestedObject("hardware_test");
        DynamicJsonDocument hw_test_doc(1024);
        hw_test_doc.parse(hw_test->getStatus());
        hw_test_status.set(hw_test_doc);
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

void printSystemStatus() {
    Serial.println("=== T-Deck-Pro System Status ===");
    Serial.println(getSystemStatus());
    Serial.println("=================================");
}

uint32_t getSystemUptime() {
    return millis() / 1000;
}

bool isSystemInitialized() {
    return system_initialized;
} 