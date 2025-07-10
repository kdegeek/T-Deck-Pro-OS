/**
 * @file keyboard_manager.h
 * @brief T-Deck-Pro Keyboard Manager - TCA8418 Keyboard Matrix
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Handles TCA8418 keyboard matrix input with event system
 */

#ifndef KEYBOARD_MANAGER_H
#define KEYBOARD_MANAGER_H

#include <Arduino.h>
#include <Adafruit_TCA8418.h>
#include "../utils/logger.h"

// ===== KEYBOARD CONSTANTS =====
#define KEYBOARD_QUEUE_SIZE 32
#define KEY_REPEAT_DELAY 500
#define KEY_REPEAT_INTERVAL 100

// ===== KEYBOARD EVENT TYPES =====
enum class KeyEventType {
    PRESS,
    RELEASE,
    HOLD,
    REPEAT
};

// ===== KEYBOARD EVENT STRUCTURE =====
struct KeyEvent {
    uint8_t key_code;
    KeyEventType type;
    uint32_t timestamp;
    bool is_modifier;
    bool is_special;
    
    KeyEvent() : key_code(0), type(KeyEventType::PRESS), timestamp(0), 
                 is_modifier(false), is_special(false) {}
    
    KeyEvent(uint8_t code, KeyEventType t, uint32_t time) : 
             key_code(code), type(t), timestamp(time), 
             is_modifier(false), is_special(false) {}
};

// ===== KEYBOARD MODIFIER STATES =====
struct ModifierState {
    bool shift;
    bool ctrl;
    bool alt;
    bool fn;
    
    ModifierState() : shift(false), ctrl(false), alt(false), fn(false) {}
};

// ===== KEYBOARD MANAGER CLASS =====
class KeyboardManager {
public:
    // Constructor/Destructor
    KeyboardManager();
    ~KeyboardManager();
    
    // Initialization
    bool initialize();
    void cleanup();
    
    // Processing
    void process();
    
    // Event handling
    bool hasEvent() const;
    KeyEvent getNextEvent();
    void clearEvents();
    
    // Configuration
    void setRepeatDelay(uint32_t delay);
    void setRepeatInterval(uint32_t interval);
    void setKeyMap(const uint8_t* key_map);
    
    // State queries
    bool isKeyPressed(uint8_t key_code) const;
    bool isModifierActive(uint8_t modifier) const;
    ModifierState getModifierState() const;
    
    // Utility functions
    String getKeyName(uint8_t key_code) const;
    uint8_t getKeyCode(const String& key_name) const;
    String getEventString(const KeyEvent& event) const;
    
    // Status
    String getStatus() const;
    uint32_t getEventCount() const;
    void resetEventCount();

private:
    // Hardware
    Adafruit_TCA8418* keyboard_;
    bool initialized_;
    
    // Event queue
    KeyEvent event_queue_[KEYBOARD_QUEUE_SIZE];
    uint8_t queue_head_;
    uint8_t queue_tail_;
    uint32_t event_count_;
    
    // Key state tracking
    bool key_states_[256];
    uint32_t key_press_times_[256];
    uint32_t key_repeat_times_[256];
    
    // Modifier state
    ModifierState modifiers_;
    
    // Configuration
    uint32_t repeat_delay_;
    uint32_t repeat_interval_;
    const uint8_t* key_map_;
    
    // Internal methods
    bool initHardware();
    void processKeyEvents();
    void handleKeyPress(uint8_t key_code);
    void handleKeyRelease(uint8_t key_code);
    void updateModifiers(uint8_t key_code, bool pressed);
    void addEvent(const KeyEvent& event);
    bool isModifierKey(uint8_t key_code) const;
    bool isSpecialKey(uint8_t key_code) const;
};

// ===== GLOBAL KEYBOARD MANAGER INSTANCE =====
extern KeyboardManager* g_keyboard_manager;

// ===== GLOBAL KEYBOARD FUNCTIONS =====
bool initializeKeyboardManager();
KeyboardManager* getKeyboardManager();

// ===== KEYBOARD UTILITY FUNCTIONS =====
String keyCodeToString(uint8_t key_code);
uint8_t stringToKeyCode(const String& key_string);
bool isPrintableKey(uint8_t key_code);

#endif // KEYBOARD_MANAGER_H 