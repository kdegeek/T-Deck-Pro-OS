/**
 * @file hardware_test_app.h
 * @brief T-Deck-Pro Hardware Test Application
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Demonstrates hardware manager functionality and integration
 */

#ifndef HARDWARE_TEST_APP_H
#define HARDWARE_TEST_APP_H

#include <Arduino.h>
#include "../core/display/eink_manager_corrected.h"
#include "../core/input/keyboard_manager.h"
#include "../core/communication/lora_manager.h"
#include "../core/utils/logger.h"

// ===== HARDWARE TEST APP CLASS =====
class HardwareTestApp {
public:
    // Constructor/Destructor
    HardwareTestApp();
    ~HardwareTestApp();
    
    // Initialization
    bool initialize();
    void cleanup();
    
    // Processing
    void process();
    
    // Test functions
    void testDisplay();
    void testKeyboard();
    void testLoRa();
    void testIntegration();
    
    // Status
    String getStatus() const;
    bool isInitialized() const;

private:
    bool initialized_;
    uint32_t test_counter_;
    uint32_t last_test_time_;
    
    // Test states
    bool display_test_complete_;
    bool keyboard_test_complete_;
    bool lora_test_complete_;
    bool integration_test_complete_;
    
    // Internal methods
    void updateDisplay();
    void handleKeyboardInput();
    void handleLoRaMessages();
    void runDisplayTest();
    void runKeyboardTest();
    void runLoRaTest();
    void runIntegrationTest();
};

// ===== GLOBAL HARDWARE TEST APP INSTANCE =====
extern HardwareTestApp* g_hardware_test_app;

// ===== GLOBAL HARDWARE TEST FUNCTIONS =====
bool initializeHardwareTestApp();
HardwareTestApp* getHardwareTestApp();

#endif // HARDWARE_TEST_APP_H 