/**
 * @file board_config.cpp
 * @brief Hardware configuration implementation for LilyGo T-Deck-Pro
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "board_config.h"
#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

static bool board_initialized = false;
static esp_adc_cal_characteristics_t adc_chars;

bool board_init(void) {
    if (board_initialized) {
        return true;
    }
    
    // Initialize GPIO pins
    pinMode(BOARD_MODEM_PWR, OUTPUT);
    pinMode(BOARD_MODEM_DTR, OUTPUT);
    pinMode(BOARD_MODEM_PWRKEY, OUTPUT);
    pinMode(BOARD_AUDIO_PWR, OUTPUT);
    pinMode(BOARD_BAT_ADC, INPUT);
    pinMode(BOARD_VBUS_SENSE, INPUT);
    
    // Initialize ADC for battery monitoring
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11); // GPIO38 for battery
    
    // Characterize ADC
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    
    // Set initial states
    digitalWrite(BOARD_MODEM_PWR, LOW);     // Modem off initially
    digitalWrite(BOARD_MODEM_DTR, HIGH);    // DTR high
    digitalWrite(BOARD_MODEM_PWRKEY, HIGH); // Power key high
    digitalWrite(BOARD_AUDIO_PWR, LOW);     // Audio off initially
    
    board_initialized = true;
    return true;
}

bool board_set_power_state(board_power_state_t state) {
    if (!board_initialized) {
        return false;
    }
    
    switch (state) {
        case BOARD_POWER_ACTIVE:
            // Normal operation mode
            setCpuFrequencyMhz(240);
            break;
            
        case BOARD_POWER_LIGHT_SLEEP:
            // Reduce CPU frequency
            setCpuFrequencyMhz(80);
            // Turn off non-essential peripherals
            digitalWrite(BOARD_AUDIO_PWR, LOW);
            break;
            
        case BOARD_POWER_DEEP_SLEEP:
            // Prepare for deep sleep
            digitalWrite(BOARD_MODEM_PWR, LOW);
            digitalWrite(BOARD_AUDIO_PWR, LOW);
            esp_deep_sleep_start();
            break;
            
        case BOARD_POWER_HIBERNATE:
            // Ultra low power mode
            esp_deep_sleep_start();
            break;
            
        default:
            return false;
    }
    
    return true;
}

uint16_t board_get_battery_voltage(void) {
    if (!board_initialized) {
        return 0;
    }
    
    // Read ADC value
    uint32_t adc_reading = 0;
    for (int i = 0; i < 64; i++) {
        adc_reading += adc1_get_raw(ADC1_CHANNEL_7);
    }
    adc_reading /= 64;
    
    // Convert to voltage (mV)
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
    
    // Battery voltage divider compensation (assuming 2:1 divider)
    voltage *= 2;
    
    return (uint16_t)voltage;
}

bool board_is_usb_connected(void) {
    if (!board_initialized) {
        return false;
    }
    
    // Read VBUS sense pin
    return digitalRead(BOARD_VBUS_SENSE) == HIGH;
}