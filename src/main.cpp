/**
 * @file main.cpp
 * @brief T-Deck-Pro Minimal LED Test Firmware
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include <Arduino.h>

// Use built-in LED or a common GPIO pin
#define LED_PIN 2

void setup() {
    // Initialize LED pin
    pinMode(LED_PIN, OUTPUT);
    
    // Also try serial in case it works
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== LED TEST FIRMWARE ===");
    Serial.println("Device booted - LED should blink");
}

void loop() {
    // Blink LED to show device is running
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
    
    // Also print to serial
    Serial.println("LED blink cycle");
}