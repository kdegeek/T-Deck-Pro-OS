/**
 * @file      main_integrated.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Integrated main entry point for T-Deck-Pro OS (Phase 1 + Phase 2)
 */

#include <Arduino.h>
#include "simple_logger.h"
#include "simple_hardware.h"

// Phase 2 Integration Layer (conditional compilation)
// TEMPORARILY DISABLED FOR DEBUGGING
//#ifdef INTEGRATION_LAYER_ENABLED
//#include "integration/service_container.h"
//#include "integration/event_bridge.h"
//#include "integration/config_manager.h"
//#include "integration/service_manager.h"
//#endif

#ifdef LVGL_INTEGRATION_ENABLED
#include "lvgl_integration.h"
#endif

// Global instances
SimpleLogger* logger = nullptr;
SimpleHardware* hardware = nullptr;

// TEMPORARILY DISABLED FOR DEBUGGING
//#ifdef INTEGRATION_LAYER_ENABLED
//ServiceContainer* serviceContainer = nullptr;
//EventBridge* eventBridge = nullptr;
//ConfigManager* configManager = nullptr;
//ServiceManager* serviceManager = nullptr;
//#endif

// System state
bool system_initialized = false;
bool phase2_enabled = false;
uint32_t last_update_time = 0;
uint32_t update_interval_ms = 100; // 10Hz update rate

// Function declarations
bool initializePhase1();
bool initializePhase2();
void updateSystem();
void handleSystemError(const char* error_msg);
void printSystemStatus();

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000); // Wait for serial to stabilize
    
    Serial.println("=== T-Deck-Pro OS Integrated Boot ===");
    Serial.println("Version: 1.0.0-integrated");
    Serial.println("Build: " __DATE__ " " __TIME__);
    
    // Initialize Phase 1 (Hardware Abstraction Layer)
    if (!initializePhase1()) {
        handleSystemError("Phase 1 initialization failed");
        return;
    }
    
    Serial.println("Phase 1 initialization complete");
    
#ifdef PHASE2_ENABLED
    // Initialize Phase 2 (Integration Layer & Services)
    if (!initializePhase2()) {
        Serial.println("WARNING: Phase 2 initialization failed, continuing with Phase 1 only");
        phase2_enabled = false;
    } else {
        Serial.println("Phase 2 initialization complete");
        phase2_enabled = true;
    }
#else
    Serial.println("Phase 2 disabled at compile time");
#endif
    
    system_initialized = true;
    printSystemStatus();
    
    Serial.println("=== T-Deck-Pro OS Boot Complete ===");
}

void loop() {
    if (!system_initialized) {
        delay(1000);
        return;
    }
    
    uint32_t current_time = millis();
    if (current_time - last_update_time >= update_interval_ms) {
        updateSystem();
        last_update_time = current_time;
    }
    
    // Small delay to prevent watchdog issues
    delay(1);
}

bool initializePhase1() {
    Serial.println("Initializing Phase 1 components...");
    
    // Initialize logger
    logger = SimpleLogger::getInstance();
    if (!logger) {
        Serial.println("ERROR: Failed to create logger instance");
        return false;
    }

    if (!logger->init(LOG_INFO)) {
        Serial.println("ERROR: Failed to initialize logger");
        return false;
    }

    LOG_INFO("System", "Logger initialized");
    
    // Initialize hardware abstraction layer
    hardware = SimpleHardware::getInstance();
    if (!hardware) {
        LOG_ERROR("System", "Failed to get hardware instance");
        return false;
    }

    if (!hardware->init()) {
        LOG_ERROR("System", "Hardware initialization failed");
        return false;
    }
    
    LOG_INFO("System", "Hardware abstraction layer initialized");
    
    // Run hardware diagnostics
    if (!hardware->runDiagnostics()) {
        LOG_WARN("System", "Hardware diagnostics reported issues");
    }
    
    return true;
}

bool initializePhase2() {
// TEMPORARILY DISABLED FOR DEBUGGING
//#ifdef INTEGRATION_LAYER_ENABLED
    LOG_INFO("System", "Phase 2 initialization temporarily disabled for debugging");
    return true;

/*    try {
        // Initialize service container
        serviceContainer = new ServiceContainer();
        if (!serviceContainer || !serviceContainer->initialize()) {
            LOG_ERROR("System", "Failed to initialize service container");
            return false;
        }
        LOG_INFO("System", "Service container initialized");
        
        // Initialize event bridge
        eventBridge = new EventBridge();
        if (!eventBridge || !eventBridge->initialize()) {
            LOG_ERROR("System", "Failed to initialize event bridge");
            return false;
        }
        LOG_INFO("System", "Event bridge initialized");
        
        // Initialize configuration manager
        configManager = new ConfigManager(ConfigStorage::LITTLEFS);
        if (!configManager || !configManager->initialize()) {
            LOG_ERROR("System", "Failed to initialize config manager");
            return false;
        }
        LOG_INFO("System", "Configuration manager initialized");
        
        // Initialize service manager
        serviceManager = new ServiceManager();
        if (!serviceManager) {
            LOG_ERROR("System", "Failed to create service manager");
            return false;
        }
        
        // Configure service manager with dependencies
        serviceManager->setServiceContainer(std::shared_ptr<ServiceContainer>(serviceContainer));
        serviceManager->setEventBridge(std::shared_ptr<EventBridge>(eventBridge));
        serviceManager->setConfigManager(std::shared_ptr<ConfigManager>(configManager));
        serviceManager->initializeWithHardware(hardware);
        
        if (!serviceManager->initialize()) {
            LOG_ERROR("System", "Failed to initialize service manager");
            return false;
        }
        LOG_INFO("System", "Service manager initialized");
        
        // Start all registered services
        if (!serviceManager->startAllServices()) {
            LOG_WARN("System", "Some services failed to start");
        }
        
        LOG_INFO("System", "Phase 2 integration layer ready");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERRORF("System", "Exception during Phase 2 initialization: %s", e.what());
        return false;
    } catch (...) {
        LOG_ERROR("System", "Unknown exception during Phase 2 initialization");
        return false;
    }
#else
    LOG_INFO("System", "Phase 2 integration layer disabled at compile time");
    return false;
#endif
*/
}

void updateSystem() {
    // Update Phase 1 components
    if (hardware) {
        hardware->update();
    }
    
// TEMPORARILY DISABLED FOR DEBUGGING
//#ifdef INTEGRATION_LAYER_ENABLED
//    if (phase2_enabled) {
//        // Update Phase 2 components
//        if (eventBridge) {
//            eventBridge->update();
//        }
//
//        if (configManager) {
//            configManager->update();
//        }
//
//        if (serviceManager) {
//            serviceManager->update();
//        }
//    }
//#endif

#ifdef LVGL_INTEGRATION_ENABLED
    // Update LVGL if hardware is ready
    if (hardware && hardware->isLVGLReady()) {
        hardware->updateLVGL();
    }
#endif
}

void handleSystemError(const char* error_msg) {
    Serial.print("SYSTEM ERROR: ");
    Serial.println(error_msg);
    
    if (logger) {
        LOG_ERRORF("System", "Critical error: %s", error_msg);
    }
    
    // Flash LED or display error on screen if possible
    if (hardware) {
        hardware->updateDisplay("SYSTEM ERROR", 10, 30);
        hardware->refreshDisplay(true);
    }
    
    // Enter error state - keep system responsive but limited
    while (true) {
        delay(1000);
        Serial.println("System in error state - restart required");
    }
}

void printSystemStatus() {
    LOG_INFO("System", "=== System Status ===");
    LOG_INFOF("System", "Phase 1 (Hardware): %s", "ACTIVE");
    LOG_INFOF("System", "Phase 2 (Services): %s", phase2_enabled ? "ACTIVE" : "DISABLED");
    
    if (hardware) {
        LOG_INFOF("System", "Free Heap: %luKB", hardware->getFreeHeap() / 1024);
        LOG_INFOF("System", "Free PSRAM: %luKB", hardware->getFreePSRAM() / 1024);
        LOG_INFOF("System", "Uptime: %lus", hardware->getUptime() / 1000);
    }
    
// TEMPORARILY DISABLED FOR DEBUGGING
//#ifdef INTEGRATION_LAYER_ENABLED
//    if (phase2_enabled && serviceManager) {
//        LOG_INFOF("System", "Services Started: %lu", serviceManager->getServicesStartedCount());
//        LOG_INFOF("System", "Services Failed: %lu", serviceManager->getServicesFailedCount());
//    }
//#endif
    
    LOG_INFO("System", "==================");
}
