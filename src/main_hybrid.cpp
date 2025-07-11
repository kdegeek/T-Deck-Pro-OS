/**
 * @file      main_hybrid.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Hybrid main entry point - Phase 1 base + incremental Phase 2 integration
 */

#include <Arduino.h>
#include <WString.h>
#include "simple_logger.h"
#include "simple_hardware.h"
#include "lvgl.h"

// Use the exact same global objects as working Phase 1
SimpleLogger* logger = nullptr;
SimpleHardware* hardware = nullptr;

// Phase 2 integration flags (start disabled, enable incrementally)
#define HYBRID_STEP_1_SERVICE_CONTAINER true
#define HYBRID_STEP_2_EVENT_BRIDGE true
#define HYBRID_STEP_3_CONFIG_MANAGER true
#define HYBRID_STEP_4_SERVICE_MANAGER true

// Phase 2 includes (conditional)
#if HYBRID_STEP_1_SERVICE_CONTAINER
#include "integration/service_container.h"
ServiceContainer* serviceContainer = nullptr;
#endif

#if HYBRID_STEP_2_EVENT_BRIDGE
#include "integration/event_bridge.h"
EventBridge* eventBridge = nullptr;
#endif

#if HYBRID_STEP_3_CONFIG_MANAGER
#include "integration/config_manager.h"
ConfigManager* configManager = nullptr;
#endif

#if HYBRID_STEP_4_SERVICE_MANAGER
#include "integration/service_manager.h"
ServiceManager* serviceManager = nullptr;
#endif

// System state
bool system_initialized = false;
uint32_t last_update_time = 0;
uint32_t update_interval_ms = 100;
uint32_t last_display_update = 0;
uint32_t display_update_interval = 3000; // 3 second minimum between display updates for e-paper stability
int touch_count = 0;
bool display_updating = false; // Flag to prevent simultaneous display updates
uint32_t last_display_refresh_start = 0; // Track when display refresh started
const uint32_t min_display_refresh_interval = 3000; // Minimum 3 seconds between e-paper refreshes

// Function declarations
bool initializePhase1();
bool initializePhase2Incremental();
void updateSystem();
void handleTouch();
void displaySystemInfo();
bool isPhase2Operational();
void setupLVGLTouchEvents();
void lvgl_touch_event_cb(lv_event_t * e);

bool initializePhase1() {
    // EXACT COPY of working simple Phase 1 initialization
    Serial.println("=== T-Deck-Pro OS Hybrid (Phase 1 Base) ===");
    Serial.println("Initializing Phase 1 components...");

    // 1. Initialize Logger (using simple pattern)
    logger = SimpleLogger::getInstance();
    if (!logger) {
        Serial.println("ERROR: Failed to create logger instance");
        return false;
    }

    if (!logger->init(LOG_INFO)) {
        Serial.println("ERROR: Failed to initialize logger");
        return false;
    }

    LOG_INFO("System", "Logger initialized successfully");

    // 2. Initialize Hardware Manager (using simple pattern)
    hardware = SimpleHardware::getInstance();
    if (!hardware) {
        LOG_ERROR("System", "Failed to create hardware instance");
        return false;
    }

    if (!hardware->init()) {
        LOG_ERROR("System", "Hardware initialization failed");
        return false;
    }

    LOG_INFO("System", "Hardware manager initialized successfully");

    // 3. Display welcome message (Phase 2 status will be updated after initialization)
    hardware->updateDisplay("T-Deck-Pro OS\nHybrid Mode\n\nPhase 1: Ready\nPhase 2: Initializing...", 10, 30);
    hardware->refreshDisplay();

    LOG_INFO("System", "Phase 1 initialization completed successfully");
    return true;
}

bool initializePhase2Incremental() {
    LOG_INFO("System", "Phase 2 incremental initialization...");
    
    // Step 1: Service Container
    #if HYBRID_STEP_1_SERVICE_CONTAINER
    LOG_INFO("System", "Initializing Service Container...");
    serviceContainer = new ServiceContainer();
    if (!serviceContainer || !serviceContainer->initialize()) {
        LOG_ERROR("System", "Service Container initialization failed");
        return false;
    }
    LOG_INFO("System", "Service Container initialized");
    #endif
    
    // Step 2: Event Bridge
    #if HYBRID_STEP_2_EVENT_BRIDGE
    LOG_INFO("System", "Initializing Event Bridge...");
    eventBridge = new EventBridge();
    if (!eventBridge || !eventBridge->initialize()) {
        LOG_ERROR("System", "Event Bridge initialization failed");
        return false;
    }
    LOG_INFO("System", "Event Bridge initialized");
    #endif
    
    // Step 3: Config Manager
    #if HYBRID_STEP_3_CONFIG_MANAGER
    LOG_INFO("System", "Initializing Config Manager...");
    configManager = new ConfigManager(ConfigStorage::LITTLEFS);
    if (!configManager || !configManager->initialize()) {
        LOG_ERROR("System", "Config Manager initialization failed");
        return false;
    }
    LOG_INFO("System", "Config Manager initialized");
    #endif
    
    // Step 4: Service Manager
    #if HYBRID_STEP_4_SERVICE_MANAGER
    LOG_INFO("System", "Initializing Service Manager...");
    serviceManager = new ServiceManager();
    if (!serviceManager) {
        LOG_ERROR("System", "Service Manager creation failed");
        return false;
    }

    // Wire up dependencies
    if (serviceContainer) {
        serviceManager->setServiceContainer(std::shared_ptr<ServiceContainer>(serviceContainer));
    }
    if (eventBridge) {
        serviceManager->setEventBridge(std::shared_ptr<EventBridge>(eventBridge));
    }
    if (configManager) {
        serviceManager->setConfigManager(std::shared_ptr<ConfigManager>(configManager));
    }

    if (!serviceManager->initialize()) {
        LOG_ERROR("System", "Service Manager initialization failed");
        return false;
    }
    LOG_INFO("System", "Service Manager initialized");
    #endif
    
    LOG_INFO("System", "Phase 2 incremental initialization complete");
    return true;
}

bool isPhase2Operational() {
    int operational_components = 0;

    #if HYBRID_STEP_1_SERVICE_CONTAINER
    if (serviceContainer && serviceContainer->isInitialized()) {
        operational_components++;
    }
    #endif

    #if HYBRID_STEP_2_EVENT_BRIDGE
    if (eventBridge && eventBridge->isInitialized()) {
        operational_components++;
    }
    #endif

    #if HYBRID_STEP_3_CONFIG_MANAGER
    if (configManager && configManager->isInitialized()) {
        operational_components++;
    }
    #endif

    #if HYBRID_STEP_4_SERVICE_MANAGER
    if (serviceManager && serviceManager->isInitialized()) {
        operational_components++;
    }
    #endif

    // Phase 2 is operational if at least one component is running
    return operational_components > 0;
}

void lvgl_touch_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    // Handle both PRESSED and CLICKED events for better responsiveness
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_CLICKED) {
        uint32_t current_time = millis();

        // Get touch coordinates
        lv_indev_t * indev = lv_indev_get_act();
        if (indev) {
            lv_point_t point;
            lv_indev_get_point(indev, &point);
            LOG_INFOF("System", "LVGL touch %s at (%d, %d)",
                     (code == LV_EVENT_PRESSED) ? "PRESSED" : "CLICKED", point.x, point.y);
        }

        // Conservative debounce to prevent system overload (1 second)
        if (current_time - last_display_update > 1000) {
            LOG_INFOF("System", "Processing LVGL touch event (%s)",
                     (code == LV_EVENT_PRESSED) ? "PRESSED" : "CLICKED");
            handleTouch();
        } else {
            LOG_INFO("System", "LVGL touch debounced");
        }
    }
}

void setupLVGLTouchEvents() {
    // Get the active screen and add touch event handler
    lv_obj_t * scr = lv_scr_act();
    if (scr) {
        // Register multiple touch events for better responsiveness
        lv_obj_add_event_cb(scr, lvgl_touch_event_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(scr, lvgl_touch_event_cb, LV_EVENT_CLICKED, NULL);
        LOG_INFO("System", "LVGL touch events registered on active screen (PRESSED + CLICKED)");
    } else {
        LOG_ERROR("System", "Failed to get active screen for touch events");
    }
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000); // Wait for serial to stabilize
    
    // Initialize Phase 1 (proven working implementation)
    if (!initializePhase1()) {
        Serial.println("CRITICAL ERROR: Phase 1 initialization failed");
        while (true) {
            delay(1000);
            Serial.println("System halted - restart required");
        }
    }
    
    // Initialize Phase 2 incrementally
    bool phase2_success = initializePhase2Incremental();
    if (!phase2_success) {
        LOG_WARN("System", "Phase 2 initialization failed - continuing with Phase 1 only");
    }

    // Update display with final status
    String final_status = "T-Deck-Pro OS\nHybrid Mode\n\nPhase 1: Active\n";
    if (isPhase2Operational()) {
        final_status += "Phase 2: Active";
    } else {
        final_status += "Phase 2: Disabled";
    }

    if (hardware) {
        hardware->updateDisplay(final_status.c_str(), 10, 30);
        hardware->refreshDisplay();
    }

    // Setup LVGL touch events
    setupLVGLTouchEvents();

    system_initialized = true;
    last_update_time = millis();

    LOG_INFO("System", "=== HYBRID SYSTEM READY ===");
    LOG_INFO("System", "Touch screen to cycle through functions");
}

void loop() {
    if (!system_initialized) return;

    uint32_t current_time = millis();

    // Update system at regular intervals
    if (current_time - last_update_time >= update_interval_ms) {
        updateSystem();
        last_update_time = current_time;
    }

    // Handle LVGL tasks (this processes touch events)
    lv_task_handler();

    // Power management update (simple approach for hybrid testing)
    // Power.update(); // TODO: Add power management in Phase 2

    // Optimized delay for balance between responsiveness and CPU usage
    // 5ms provides good responsiveness without being too aggressive
    delay(5);
}

void updateSystem() {
    // Update Phase 1 components (exact same as working version)
    if (hardware) {
        hardware->update();
    }

    // Update Phase 2 components (incremental)
    #if HYBRID_STEP_2_EVENT_BRIDGE
    if (eventBridge) {
        eventBridge->update();
    }
    #endif

    #if HYBRID_STEP_3_CONFIG_MANAGER
    if (configManager) {
        configManager->update();
    }
    #endif

    #if HYBRID_STEP_4_SERVICE_MANAGER
    if (serviceManager) {
        serviceManager->update();
    }
    #endif
}

void handleTouch() {
    uint32_t current_time = millis();

    // Check if enough time has passed since last display refresh
    if (current_time - last_display_refresh_start < min_display_refresh_interval) {
        uint32_t time_remaining = min_display_refresh_interval - (current_time - last_display_refresh_start);
        LOG_INFOF("System", "Touch ignored - display cooling down (%lu ms remaining)", time_remaining);
        return;
    }

    // Prevent simultaneous display updates
    if (display_updating) {
        LOG_INFO("System", "Touch ignored - display update in progress");
        return;
    }

    touch_count++;
    last_display_update = current_time;
    last_display_refresh_start = current_time;

    LOG_INFOF("System", "Touch %d detected - updating display", touch_count);

    // Simple display update for hybrid testing
    display_updating = true;
    displaySystemInfo();
    display_updating = false;

    LOG_INFOF("System", "Touch %d processed successfully", touch_count);
}

void displaySystemInfo() {
    LOG_INFO("System", "Updating display with system info...");

    String info = "T-Deck-Pro OS\nHybrid Mode\n\n";
    info += "Phase 1: Active\n";

    if (isPhase2Operational()) {
        info += "Phase 2: Active\n\n";

        // Show individual component status
        #if HYBRID_STEP_1_SERVICE_CONTAINER
        if (serviceContainer && serviceContainer->isInitialized()) {
            info += "Service Container: OK\n";
        }
        #endif
        #if HYBRID_STEP_2_EVENT_BRIDGE
        if (eventBridge && eventBridge->isInitialized()) {
            info += "Event Bridge: OK\n";
        }
        #endif
        #if HYBRID_STEP_3_CONFIG_MANAGER
        if (configManager && configManager->isInitialized()) {
            info += "Config Manager: OK\n";
        }
        #endif
        #if HYBRID_STEP_4_SERVICE_MANAGER
        if (serviceManager && serviceManager->isInitialized()) {
            info += "Service Manager: OK\n";
        }
        #endif
    } else {
        info += "Phase 2: Disabled\n";
    }

    info += "\nTouch: " + String(touch_count);
    info += "\nUptime: " + String(millis() / 1000) + "s";

    if (hardware) {
        LOG_INFO("System", "Direct display update (no refresh)...");
        // Use updateDisplay only - it includes the display() call
        // Avoid calling refreshDisplay() which causes continuous resets
        hardware->updateDisplay(info.c_str(), 10, 30);
        LOG_INFO("System", "Display update complete");
    } else {
        LOG_ERROR("System", "Hardware not available for display update");
    }
}
