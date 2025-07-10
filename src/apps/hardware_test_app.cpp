/**
 * @file hardware_test_app.cpp
 * @brief T-Deck-Pro Hardware Test Application Implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Demonstrates hardware manager functionality and integration
 */

#include "hardware_test_app.h"

// ===== GLOBAL HARDWARE TEST APP INSTANCE =====
HardwareTestApp* g_hardware_test_app = nullptr;

// ===== HARDWARE TEST APP IMPLEMENTATION =====

HardwareTestApp::HardwareTestApp() : initialized_(false), test_counter_(0), last_test_time_(0),
                                     display_test_complete_(false), keyboard_test_complete_(false),
                                     lora_test_complete_(false), integration_test_complete_(false) {
}

HardwareTestApp::~HardwareTestApp() {
    if (initialized_) {
        cleanup();
    }
}

bool HardwareTestApp::initialize() {
    if (initialized_) {
        return true;
    }
    
    logInfo("HW_TEST", "Initializing hardware test application");
    
    // Reset test states
    display_test_complete_ = false;
    keyboard_test_complete_ = false;
    lora_test_complete_ = false;
    integration_test_complete_ = false;
    test_counter_ = 0;
    last_test_time_ = millis();
    
    initialized_ = true;
    
    logInfo("HW_TEST", "Hardware test application initialized successfully");
    return true;
}

void HardwareTestApp::cleanup() {
    if (!initialized_) {
        return;
    }
    
    logInfo("HW_TEST", "Cleaning up hardware test application");
    
    initialized_ = false;
}

void HardwareTestApp::process() {
    if (!initialized_) {
        return;
    }
    
    // Update display
    updateDisplay();
    
    // Handle keyboard input
    handleKeyboardInput();
    
    // Handle LoRa messages
    handleLoRaMessages();
    
    // Run tests periodically
    uint32_t now = millis();
    if (now - last_test_time_ > 10000) { // Every 10 seconds
        test_counter_++;
        last_test_time_ = now;
        
        // Run tests in sequence
        if (!display_test_complete_) {
            runDisplayTest();
        } else if (!keyboard_test_complete_) {
            runKeyboardTest();
        } else if (!lora_test_complete_) {
            runLoRaTest();
        } else if (!integration_test_complete_) {
            runIntegrationTest();
        }
    }
}

void HardwareTestApp::updateDisplay() {
    EinkManager* eink = getEinkManager();
    if (!eink) {
        return;
    }
    
    // Clear display every few seconds
    static uint32_t last_clear = 0;
    uint32_t now = millis();
    if (now - last_clear > 5000) {
        eink->clear();
        last_clear = now;
    }
    
    // Draw test information
    eink->drawText(10, 20, "Hardware Test App", 0x000000, 16);
    eink->drawText(10, 40, "Test Counter: " + String(test_counter_), 0x000000, 12);
    eink->drawText(10, 60, "Display: " + String(display_test_complete_ ? "OK" : "Testing"), 0x000000, 12);
    eink->drawText(10, 80, "Keyboard: " + String(keyboard_test_complete_ ? "OK" : "Testing"), 0x000000, 12);
    eink->drawText(10, 100, "LoRa: " + String(lora_test_complete_ ? "OK" : "Testing"), 0x000000, 12);
    eink->drawText(10, 120, "Integration: " + String(integration_test_complete_ ? "OK" : "Testing"), 0x000000, 12);
    
    // Draw status indicators
    eink->drawRect(200, 20, 20, 20, 0x000000, display_test_complete_);
    eink->drawRect(200, 40, 20, 20, 0x000000, keyboard_test_complete_);
    eink->drawRect(200, 60, 20, 20, 0x000000, lora_test_complete_);
    eink->drawRect(200, 80, 20, 20, 0x000000, integration_test_complete_);
    
    // Refresh display
    eink->refresh();
}

void HardwareTestApp::handleKeyboardInput() {
    KeyboardManager* keyboard = getKeyboardManager();
    if (!keyboard) {
        return;
    }
    
    // Process keyboard events
    while (keyboard->hasEvent()) {
        KeyEvent event = keyboard->getNextEvent();
        
        if (event.type == KeyEventType::PRESS) {
            logInfo("HW_TEST", "Key pressed: " + keyboard->getKeyName(event.key_code));
            
            // Handle special keys
            switch (event.key_code) {
                case 0x08: // ESC
                    logInfo("HW_TEST", "ESC pressed - exiting test");
                    break;
                case 0x05: // ENTER
                    logInfo("HW_TEST", "ENTER pressed - running tests");
                    test_counter_++;
                    break;
                case 0x0A: // UP
                    logInfo("HW_TEST", "UP arrow pressed");
                    break;
                case 0x0B: // DOWN
                    logInfo("HW_TEST", "DOWN arrow pressed");
                    break;
                case 0x0C: // LEFT
                    logInfo("HW_TEST", "LEFT arrow pressed");
                    break;
                case 0x0D: // RIGHT
                    logInfo("HW_TEST", "RIGHT arrow pressed");
                    break;
                default:
                    if (isPrintableKey(event.key_code)) {
                        logInfo("HW_TEST", "Printable key: " + String((char)event.key_code));
                    }
                    break;
            }
        }
    }
}

void HardwareTestApp::handleLoRaMessages() {
    LoRaManager* lora = getLoRaManager();
    if (!lora) {
        return;
    }
    
    // Check for received messages
    if (lora->hasReceived()) {
        LoRaPacket packet = lora->getReceivedPacket();
        
        logInfo("HW_TEST", "Received LoRa message from " + String(packet.source_id[0], HEX));
        logInfo("HW_TEST", "Message type: " + String(packet.type));
        logInfo("HW_TEST", "Message length: " + String(packet.length));
        
        if (packet.length > 0) {
            String message = String((char*)packet.data, packet.length);
            logInfo("HW_TEST", "Message content: " + message);
        }
    }
}

void HardwareTestApp::runDisplayTest() {
    logInfo("HW_TEST", "Running display test");
    
    EinkManager* eink = getEinkManager();
    if (!eink) {
        logError("HW_TEST", "E-ink manager not available");
        return;
    }
    
    // Test basic drawing functions
    eink->clear();
    eink->drawText(10, 20, "Display Test", 0x000000, 16);
    eink->drawText(10, 40, "Drawing shapes...", 0x000000, 12);
    
    // Draw some shapes
    eink->drawRect(50, 60, 50, 30, 0x000000, false);
    eink->drawCircle(150, 75, 20, 0x000000, false);
    eink->drawLine(200, 60, 250, 90, 0x000000, 2);
    
    eink->refresh();
    
    logInfo("HW_TEST", "Display test completed");
    display_test_complete_ = true;
}

void HardwareTestApp::runKeyboardTest() {
    logInfo("HW_TEST", "Running keyboard test");
    
    KeyboardManager* keyboard = getKeyboardManager();
    if (!keyboard) {
        logError("HW_TEST", "Keyboard manager not available");
        return;
    }
    
    // Test keyboard functionality
    ModifierState modifiers = keyboard->getModifierState();
    logInfo("HW_TEST", "Modifier state - Shift: " + String(modifiers.shift) + 
            " Ctrl: " + String(modifiers.ctrl) + " Alt: " + String(modifiers.alt) + 
            " Fn: " + String(modifiers.fn));
    
    // Test key state
    bool has_pressed_keys = false;
    for (uint8_t i = 0; i < 256; i++) {
        if (keyboard->isKeyPressed(i)) {
            logInfo("HW_TEST", "Key pressed: " + keyboard->getKeyName(i));
            has_pressed_keys = true;
        }
    }
    
    if (!has_pressed_keys) {
        logInfo("HW_TEST", "No keys currently pressed");
    }
    
    logInfo("HW_TEST", "Keyboard test completed");
    keyboard_test_complete_ = true;
}

void HardwareTestApp::runLoRaTest() {
    logInfo("HW_TEST", "Running LoRa test");
    
    LoRaManager* lora = getLoRaManager();
    if (!lora) {
        logError("HW_TEST", "LoRa manager not available");
        return;
    }
    
    // Test LoRa transmission
    String test_message = "HW_TEST_" + String(test_counter_);
    if (lora->broadcast(test_message)) {
        logInfo("HW_TEST", "LoRa broadcast sent: " + test_message);
    } else {
        logError("HW_TEST", "Failed to send LoRa broadcast");
    }
    
    // Get LoRa status
    int16_t rssi = lora->getRSSI();
    float snr = lora->getSNR();
    uint32_t packet_count = lora->getPacketCount();
    
    logInfo("HW_TEST", "LoRa status - RSSI: " + String(rssi) + " SNR: " + String(snr) + 
            " Packets: " + String(packet_count));
    
    logInfo("HW_TEST", "LoRa test completed");
    lora_test_complete_ = true;
}

void HardwareTestApp::runIntegrationTest() {
    logInfo("HW_TEST", "Running integration test");
    
    // Test integration between hardware components
    EinkManager* eink = getEinkManager();
    KeyboardManager* keyboard = getKeyboardManager();
    LoRaManager* lora = getLoRaManager();
    
    if (eink && keyboard && lora) {
        // Display integration status
        eink->clear();
        eink->drawText(10, 20, "Integration Test", 0x000000, 16);
        eink->drawText(10, 40, "All hardware OK", 0x000000, 12);
        eink->drawText(10, 60, "Press any key to test", 0x000000, 12);
        eink->refresh();
        
        // Wait for keyboard input
        uint32_t start_time = millis();
        bool key_pressed = false;
        
        while (millis() - start_time < 5000 && !key_pressed) {
            if (keyboard->hasEvent()) {
                KeyEvent event = keyboard->getNextEvent();
                if (event.type == KeyEventType::PRESS) {
                    key_pressed = true;
                    
                    // Send LoRa message with key info
                    String key_message = "Key pressed: " + keyboard->getKeyName(event.key_code);
                    lora->broadcast(key_message);
                    
                    // Update display
                    eink->drawText(10, 80, "Key: " + keyboard->getKeyName(event.key_code), 0x000000, 12);
                    eink->drawText(10, 100, "LoRa sent", 0x000000, 12);
                    eink->refresh();
                }
            }
            delay(10);
        }
        
        if (!key_pressed) {
            eink->drawText(10, 80, "No key pressed", 0x000000, 12);
            eink->refresh();
        }
        
        logInfo("HW_TEST", "Integration test completed");
    } else {
        logError("HW_TEST", "Not all hardware managers available");
    }
    
    integration_test_complete_ = true;
}

void HardwareTestApp::testDisplay() {
    runDisplayTest();
}

void HardwareTestApp::testKeyboard() {
    runKeyboardTest();
}

void HardwareTestApp::testLoRa() {
    runLoRaTest();
}

void HardwareTestApp::testIntegration() {
    runIntegrationTest();
}

String HardwareTestApp::getStatus() const {
    DynamicJsonDocument doc(1024);
    
    doc["initialized"] = initialized_;
    doc["test_counter"] = test_counter_;
    doc["display_test_complete"] = display_test_complete_;
    doc["keyboard_test_complete"] = keyboard_test_complete_;
    doc["lora_test_complete"] = lora_test_complete_;
    doc["integration_test_complete"] = integration_test_complete_;
    
    String output;
    serializeJson(doc, output);
    return output;
}

bool HardwareTestApp::isInitialized() const {
    return initialized_;
}

// ===== GLOBAL HARDWARE TEST FUNCTIONS =====

bool initializeHardwareTestApp() {
    if (g_hardware_test_app) {
        return true;
    }
    
    g_hardware_test_app = new HardwareTestApp();
    if (!g_hardware_test_app) {
        return false;
    }
    
    return g_hardware_test_app->initialize();
}

HardwareTestApp* getHardwareTestApp() {
    return g_hardware_test_app;
} 