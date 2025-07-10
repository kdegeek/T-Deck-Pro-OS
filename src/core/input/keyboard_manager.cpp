/**
 * @file keyboard_manager.cpp
 * @brief T-Deck-Pro Keyboard Manager - TCA8418 Implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Handles TCA8418 keyboard matrix input with event system and modifier tracking
 */

#include "keyboard_manager.h"
#include "../utils/logger.h"
#include "../hal/board_config_corrected.h"

// ===== GLOBAL KEYBOARD MANAGER INSTANCE =====
KeyboardManager* g_keyboard_manager = nullptr;

// ===== KEYBOARD MANAGER IMPLEMENTATION =====

KeyboardManager::KeyboardManager() : keyboard_(nullptr), initialized_(false),
                                    queue_head_(0), queue_tail_(0), event_count_(0),
                                    repeat_delay_(KEY_REPEAT_DELAY), repeat_interval_(KEY_REPEAT_INTERVAL),
                                    key_map_(nullptr) {
    
    // Initialize key states
    memset(key_states_, 0, sizeof(key_states_));
    memset(key_press_times_, 0, sizeof(key_press_times_));
    memset(key_repeat_times_, 0, sizeof(key_repeat_times_));
    
    // Initialize event queue
    memset(event_queue_, 0, sizeof(event_queue_));
}

KeyboardManager::~KeyboardManager() {
    if (initialized_) {
        cleanup();
    }
    
    if (keyboard_) {
        delete keyboard_;
        keyboard_ = nullptr;
    }
}

bool KeyboardManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    logInfo("KEYBOARD", "Initializing keyboard manager");
    
    // Initialize hardware
    if (!initHardware()) {
        logError("KEYBOARD", "Failed to initialize keyboard hardware");
        return false;
    }
    
    initialized_ = true;
    
    logInfo("KEYBOARD", "Keyboard manager initialized successfully");
    return true;
}

void KeyboardManager::cleanup() {
    if (!initialized_) {
        return;
    }
    
    logInfo("KEYBOARD", "Cleaning up keyboard manager");
    
    // Clear event queue
    clearEvents();
    
    initialized_ = false;
}

bool KeyboardManager::initHardware() {
    // Create keyboard object
    keyboard_ = new Adafruit_TCA8418();
    if (!keyboard_) {
        logError("KEYBOARD", "Failed to create keyboard object");
        return false;
    }
    
    // Initialize I2C communication
    if (!keyboard_->begin(BOARD_I2C_SDA, BOARD_I2C_SCL)) {
        logError("KEYBOARD", "Failed to initialize TCA8418");
        return false;
    }
    
    // Configure keyboard matrix
    keyboard_->matrix(3, 4); // 3x4 matrix configuration
    
    // Enable key scanning
    keyboard_->enableKeyScanning(true);
    
    // Set interrupt pin (if available)
    // keyboard_->enableInterrupt(BOARD_KEYBOARD_INT);
    
    logInfo("KEYBOARD", "Keyboard hardware initialized");
    return true;
}

void KeyboardManager::process() {
    if (!initialized_ || !keyboard_) {
        return;
    }
    
    // Process key events
    processKeyEvents();
}

void KeyboardManager::processKeyEvents() {
    uint8_t key_count = keyboard_->getKeyCount();
    
    if (key_count > 0) {
        // Get pressed keys
        uint8_t keys[8];
        uint8_t count = keyboard_->getKeys(keys, 8);
        
        for (uint8_t i = 0; i < count; i++) {
            uint8_t key_code = keys[i];
            
            if (!key_states_[key_code]) {
                // Key was just pressed
                handleKeyPress(key_code);
            } else {
                // Key is being held
                uint32_t now = millis();
                uint32_t press_time = key_press_times_[key_code];
                
                // Check for repeat
                if ((now - press_time) > repeat_delay_) {
                    uint32_t last_repeat = key_repeat_times_[key_code];
                    if ((now - last_repeat) > repeat_interval_) {
                        // Generate repeat event
                        KeyEvent event(key_code, KeyEventType::REPEAT, now);
                        event.is_modifier = isModifierKey(key_code);
                        event.is_special = isSpecialKey(key_code);
                        addEvent(event);
                        
                        key_repeat_times_[key_code] = now;
                    }
                }
            }
        }
        
        // Check for released keys
        for (uint8_t key = 0; key < 256; key++) {
            if (key_states_[key]) {
                bool still_pressed = false;
                for (uint8_t i = 0; i < count; i++) {
                    if (keys[i] == key) {
                        still_pressed = true;
                        break;
                    }
                }
                
                if (!still_pressed) {
                    // Key was released
                    handleKeyRelease(key);
                }
            }
        }
    }
}

void KeyboardManager::handleKeyPress(uint8_t key_code) {
    uint32_t now = millis();
    
    // Update key state
    key_states_[key_code] = true;
    key_press_times_[key_code] = now;
    key_repeat_times_[key_code] = now;
    
    // Update modifiers
    updateModifiers(key_code, true);
    
    // Create press event
    KeyEvent event(key_code, KeyEventType::PRESS, now);
    event.is_modifier = isModifierKey(key_code);
    event.is_special = isSpecialKey(key_code);
    addEvent(event);
    
    logDebug("KEYBOARD", "Key pressed: " + getKeyName(key_code) + " (0x" + String(key_code, HEX) + ")");
}

void KeyboardManager::handleKeyRelease(uint8_t key_code) {
    uint32_t now = millis();
    
    // Update key state
    key_states_[key_code] = false;
    
    // Update modifiers
    updateModifiers(key_code, false);
    
    // Create release event
    KeyEvent event(key_code, KeyEventType::RELEASE, now);
    event.is_modifier = isModifierKey(key_code);
    event.is_special = isSpecialKey(key_code);
    addEvent(event);
    
    logDebug("KEYBOARD", "Key released: " + getKeyName(key_code) + " (0x" + String(key_code, HEX) + ")");
}

void KeyboardManager::updateModifiers(uint8_t key_code, bool pressed) {
    switch (key_code) {
        case 0x01: // Shift
            modifiers_.shift = pressed;
            break;
        case 0x02: // Ctrl
            modifiers_.ctrl = pressed;
            break;
        case 0x03: // Alt
            modifiers_.alt = pressed;
            break;
        case 0x04: // Fn
            modifiers_.fn = pressed;
            break;
    }
}

void KeyboardManager::addEvent(const KeyEvent& event) {
    uint8_t next_head = (queue_head_ + 1) % KEYBOARD_QUEUE_SIZE;
    
    if (next_head != queue_tail_) {
        event_queue_[queue_head_] = event;
        queue_head_ = next_head;
        event_count_++;
    } else {
        logWarn("KEYBOARD", "Event queue full, dropping event");
    }
}

bool KeyboardManager::hasEvent() const {
    return queue_head_ != queue_tail_;
}

KeyEvent KeyboardManager::getNextEvent() {
    KeyEvent event;
    
    if (hasEvent()) {
        event = event_queue_[queue_tail_];
        queue_tail_ = (queue_tail_ + 1) % KEYBOARD_QUEUE_SIZE;
    }
    
    return event;
}

void KeyboardManager::clearEvents() {
    queue_head_ = 0;
    queue_tail_ = 0;
    event_count_ = 0;
    memset(event_queue_, 0, sizeof(event_queue_));
}

void KeyboardManager::setRepeatDelay(uint32_t delay) {
    repeat_delay_ = delay;
    logInfo("KEYBOARD", "Repeat delay set to: " + String(delay) + "ms");
}

void KeyboardManager::setRepeatInterval(uint32_t interval) {
    repeat_interval_ = interval;
    logInfo("KEYBOARD", "Repeat interval set to: " + String(interval) + "ms");
}

void KeyboardManager::setKeyMap(const uint8_t* key_map) {
    key_map_ = key_map;
    logInfo("KEYBOARD", "Key map updated");
}

bool KeyboardManager::isKeyPressed(uint8_t key_code) const {
    return key_states_[key_code];
}

bool KeyboardManager::isModifierActive(uint8_t modifier) const {
    switch (modifier) {
        case 0x01: return modifiers_.shift;
        case 0x02: return modifiers_.ctrl;
        case 0x03: return modifiers_.alt;
        case 0x04: return modifiers_.fn;
        default: return false;
    }
}

ModifierState KeyboardManager::getModifierState() const {
    return modifiers_;
}

String KeyboardManager::getKeyName(uint8_t key_code) const {
    switch (key_code) {
        case 0x00: return "NONE";
        case 0x01: return "SHIFT";
        case 0x02: return "CTRL";
        case 0x03: return "ALT";
        case 0x04: return "FN";
        case 0x05: return "ENTER";
        case 0x06: return "BACKSPACE";
        case 0x07: return "TAB";
        case 0x08: return "ESC";
        case 0x09: return "SPACE";
        case 0x0A: return "UP";
        case 0x0B: return "DOWN";
        case 0x0C: return "LEFT";
        case 0x0D: return "RIGHT";
        case 0x0E: return "HOME";
        case 0x0F: return "END";
        case 0x10: return "PAGE_UP";
        case 0x11: return "PAGE_DOWN";
        case 0x12: return "INSERT";
        case 0x13: return "DELETE";
        case 0x14: return "F1";
        case 0x15: return "F2";
        case 0x16: return "F3";
        case 0x17: return "F4";
        case 0x18: return "F5";
        case 0x19: return "F6";
        case 0x1A: return "F7";
        case 0x1B: return "F8";
        case 0x1C: return "F9";
        case 0x1D: return "F10";
        case 0x1E: return "F11";
        case 0x1F: return "F12";
        default:
            if (key_code >= 0x20 && key_code <= 0x7E) {
                return String((char)key_code);
            } else {
                return "KEY_" + String(key_code, HEX);
            }
    }
}

uint8_t KeyboardManager::getKeyCode(const String& key_name) const {
    if (key_name.length() == 1) {
        return key_name[0];
    }
    
    if (key_name.equalsIgnoreCase("SHIFT")) return 0x01;
    if (key_name.equalsIgnoreCase("CTRL")) return 0x02;
    if (key_name.equalsIgnoreCase("ALT")) return 0x03;
    if (key_name.equalsIgnoreCase("FN")) return 0x04;
    if (key_name.equalsIgnoreCase("ENTER")) return 0x05;
    if (key_name.equalsIgnoreCase("BACKSPACE")) return 0x06;
    if (key_name.equalsIgnoreCase("TAB")) return 0x07;
    if (key_name.equalsIgnoreCase("ESC")) return 0x08;
    if (key_name.equalsIgnoreCase("SPACE")) return 0x09;
    if (key_name.equalsIgnoreCase("UP")) return 0x0A;
    if (key_name.equalsIgnoreCase("DOWN")) return 0x0B;
    if (key_name.equalsIgnoreCase("LEFT")) return 0x0C;
    if (key_name.equalsIgnoreCase("RIGHT")) return 0x0D;
    if (key_name.equalsIgnoreCase("HOME")) return 0x0E;
    if (key_name.equalsIgnoreCase("END")) return 0x0F;
    if (key_name.equalsIgnoreCase("PAGE_UP")) return 0x10;
    if (key_name.equalsIgnoreCase("PAGE_DOWN")) return 0x11;
    if (key_name.equalsIgnoreCase("INSERT")) return 0x12;
    if (key_name.equalsIgnoreCase("DELETE")) return 0x13;
    
    return 0x00; // NONE
}

String KeyboardManager::getEventString(const KeyEvent& event) const {
    String type_str;
    switch (event.type) {
        case KeyEventType::PRESS: type_str = "PRESS"; break;
        case KeyEventType::RELEASE: type_str = "RELEASE"; break;
        case KeyEventType::HOLD: type_str = "HOLD"; break;
        case KeyEventType::REPEAT: type_str = "REPEAT"; break;
    }
    
    return getKeyName(event.key_code) + ":" + type_str + ":" + String(event.timestamp);
}

String KeyboardManager::getStatus() const {
    DynamicJsonDocument doc(1024);
    
    doc["initialized"] = initialized_;
    doc["event_count"] = event_count_;
    doc["queue_size"] = (queue_head_ - queue_tail_ + KEYBOARD_QUEUE_SIZE) % KEYBOARD_QUEUE_SIZE;
    doc["repeat_delay"] = repeat_delay_;
    doc["repeat_interval"] = repeat_interval_;
    
    JsonObject modifiers = doc.createNestedObject("modifiers");
    modifiers["shift"] = modifiers_.shift;
    modifiers["ctrl"] = modifiers_.ctrl;
    modifiers["alt"] = modifiers_.alt;
    modifiers["fn"] = modifiers_.fn;
    
    String output;
    serializeJson(doc, output);
    return output;
}

uint32_t KeyboardManager::getEventCount() const {
    return event_count_;
}

void KeyboardManager::resetEventCount() {
    event_count_ = 0;
}

bool KeyboardManager::isModifierKey(uint8_t key_code) const {
    return (key_code >= 0x01 && key_code <= 0x04);
}

bool KeyboardManager::isSpecialKey(uint8_t key_code) const {
    return (key_code >= 0x05 && key_code <= 0x1F);
}

// ===== GLOBAL KEYBOARD MANAGER FUNCTIONS =====

bool initializeKeyboardManager() {
    if (g_keyboard_manager) {
        return true;
    }
    
    g_keyboard_manager = new KeyboardManager();
    if (!g_keyboard_manager) {
        return false;
    }
    
    return g_keyboard_manager->initialize();
}

KeyboardManager* getKeyboardManager() {
    return g_keyboard_manager;
}

// ===== KEYBOARD UTILITY FUNCTIONS =====

String keyCodeToString(uint8_t key_code) {
    if (g_keyboard_manager) {
        return g_keyboard_manager->getKeyName(key_code);
    }
    return "UNKNOWN";
}

uint8_t stringToKeyCode(const String& key_string) {
    if (g_keyboard_manager) {
        return g_keyboard_manager->getKeyCode(key_string);
    }
    return 0x00;
}

bool isPrintableKey(uint8_t key_code) {
    return (key_code >= 0x20 && key_code <= 0x7E);
} 