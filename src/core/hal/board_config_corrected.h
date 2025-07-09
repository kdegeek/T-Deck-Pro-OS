/**
 * @file board_config.h
 * @brief Hardware configuration for LilyGo T-Deck-Pro - CORRECTED VERSION
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note All pin definitions corrected to match actual T-Deck-Pro hardware
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// ===== CRITICAL NOTICE =====
// This file has been COMPLETELY CORRECTED to match the actual T-Deck-Pro hardware
// Previous version was for a different device entirely

// ===== E-INK DISPLAY PINS - CORRECTED FROM REFERENCE =====
// Commented out macros that are duplicated in os_config_corrected.h to prevent redefinition warnings.
// #define BOARD_EPD_CS        34  // CORRECTED: was 10, now matches .REFERENCE
// #define BOARD_EPD_DC        35  // CORRECTED: was 11, now matches .REFERENCE
// #define BOARD_EPD_RST       -1  // CORRECTED: was 12, now matches .REFERENCE (no reset pin)
// #define BOARD_EPD_BUSY      37  // CORRECTED: was 13, now matches .REFERENCE
// #define BOARD_EPD_SCK       36  // Shared SPI SCK
// #define BOARD_EPD_MOSI      33  // Shared SPI MOSI

// ===== LORA PINS (SX1262) - VERIFIED CORRECT =====
// #define BOARD_LORA_CS       3
// #define BOARD_LORA_RST      4
// #define BOARD_LORA_DIO1     5   // INT pin
// #define BOARD_LORA_BUSY     6
// #define BOARD_LORA_SCK      36  // Shared with EPD
// #define BOARD_LORA_MOSI     33  // Shared with EPD
// #define BOARD_LORA_MISO     47

// ===== 4G MODEM PINS (A7682E) - VERIFIED CORRECT =====
// #define BOARD_MODEM_RI      7
// #define BOARD_MODEM_ITR     8
// #define BOARD_MODEM_RST     9
// #define BOARD_MODEM_RXD     10
// #define BOARD_MODEM_TXD     11
// #define BOARD_MODEM_PWRKEY  40

// ===== I2C PINS - CORRECTED FROM REFERENCE =====
// #define BOARD_I2C_SDA       13  // CORRECTED: was 18
// #define BOARD_I2C_SCL       14  // CORRECTED: was 8

// ===== I2C DEVICE ADDRESSES - VERIFIED CORRECT =====
// #define BOARD_TOUCH_ADDR      0x1A // CST328 Touch controller
// #define BOARD_KEYBOARD_ADDR   0x34 // TCA8418 Keyboard controller
// #define BOARD_LIGHT_ADDR      0x23 // LTR_553ALS Light sensor
// #define BOARD_GYRO_ADDR       0x28 // BHI260AP Gyroscope
// #define BOARD_BATTERY_ADDR    0x55 // BQ27220 Battery gauge
// #define BOARD_POWER_ADDR      0x6B // BQ25896 Power management

// ===== KEYBOARD PINS - CORRECTED =====
// #define BOARD_KB_INT        15  // Keyboard interrupt
// #define BOARD_KB_LED        42  // Keyboard LED

// ===== TOUCHSCREEN PINS - CORRECTED =====
// #define BOARD_TOUCH_INT     12  // Touch interrupt
// #define BOARD_TOUCH_RST     45  // Touch reset

// ===== SENSORS PINS - CORRECTED =====
// #define BOARD_ALS_INT       16  // Light sensor interrupt
// #define BOARD_GYRO_INT      21  // Gyroscope interrupt

// ===== AUDIO PINS - CORRECTED FROM REFERENCE =====
// NOTE: Audio not available on 4G version, but pins defined for reference
// #define BOARD_AUDIO_BCLK    7   // I2S bit clock (conflicts with modem on 4G version)
// #define BOARD_AUDIO_DOUT    8   // I2S data out (conflicts with modem on 4G version)
// #define BOARD_AUDIO_LRC     9   // I2S left/right clock (conflicts with modem on 4G version)

// ===== MICROPHONE PINS - CORRECTED FROM REFERENCE =====
// #define BOARD_MIC_DATA      17  // CORRECTED: was 20
// #define BOARD_MIC_CLOCK     18  // CORRECTED: was 22

// ===== SD CARD PINS - CORRECTED =====
// #define BOARD_SD_CS         48  // SD card chip select
// #define BOARD_SD_SCK        36  // Shared SPI SCK
// #define BOARD_SD_MOSI       33  // Shared SPI MOSI
// #define BOARD_SD_MISO       47  // Shared SPI MISO

// ===== GPS PINS - VERIFIED CORRECT =====
// #define BOARD_GPS_RXD       44
// #define BOARD_GPS_TXD       43
// #define BOARD_GPS_PPS       1

// ===== CONTROL PINS - VERIFIED CORRECT =====
// #define BOARD_BOOT_PIN      0   // Boot button
// #define BOARD_MOTOR_PIN     2   // Vibration motor

// ===== ENABLE PINS - VERIFIED CORRECT =====
// #define BOARD_GPS_EN        39  // GPS module enable
// #define BOARD_GYRO_EN       38  // Gyroscope 1.8V enable
// #define BOARD_MODEM_EN      41  // 4G modem enable
// #define BOARD_LORA_EN       46  // LoRa module enable

// ===== DISPLAY SPECIFICATIONS - CORRECTED =====
// #define BOARD_EPD_WIDTH     240 // Display width
// #define BOARD_EPD_HEIGHT    320 // Display height
// #define BOARD_EPD_ROTATION  0   // Display rotation

// ===== SYSTEM CONFIGURATION =====
// #define BOARD_CPU_FREQ      240  // MHz
// #define BOARD_FLASH_SIZE    16   // MB
// #define BOARD_PSRAM_SIZE    8    // MB

// ===== SPI BUS CONFIGURATION =====
// #define BOARD_SPI_SCK       36
// #define BOARD_SPI_MOSI      33
// #define BOARD_SPI_MISO      47
// #define BOARD_SPI_FREQ      80000000  // 80MHz

// ===== I2C BUS CONFIGURATION =====
// #define BOARD_I2C_FREQ      400000 // 400kHz

// ===== POWER MANAGEMENT THRESHOLDS =====
// #define BOARD_BAT_LOW_MV    3300  // Low battery threshold
// #define BOARD_BAT_CRIT_MV   3000  // Critical battery threshold
// #define BOARD_BAT_FULL_MV   4200  // Full battery voltage

// ===== COMMUNICATION FREQUENCIES =====
// #define BOARD_LORA_FREQ     915.0  // MHz (US frequency)

// ===== TIMING CONSTANTS =====
// #define BOARD_BOOT_DELAY_MS     1000
// #define BOARD_MODEM_INIT_MS     5000
// #define BOARD_EPD_INIT_MS       2000

// ===== MEMORY ALLOCATION - OPTIMIZED FOR ESP32-S3 =====
// #define BOARD_HEAP_SIZE         (200 * 1024)  // 200KB for heap
// #define BOARD_STACK_SIZE        (8 * 1024)    // 8KB default stack
// #define BOARD_EPD_BUFFER_SIZE   ((BOARD_EPD_WIDTH * BOARD_EPD_HEIGHT) / 8)

// ===== FEATURE ENABLES - CORRECTED FOR T-DECK-PRO 4G =====
// #define BOARD_ENABLE_PSRAM      1
// #define BOARD_ENABLE_BLUETOOTH  1
// #define BOARD_ENABLE_WIFI       1
// #define BOARD_ENABLE_4G         1  // 4G version
// #define BOARD_ENABLE_LORA       1
// #define BOARD_ENABLE_GPS        1
// #define BOARD_ENABLE_SENSORS    1
// #define BOARD_ENABLE_AUDIO      0  // Disabled on 4G version
// #define BOARD_ENABLE_SD         1

// ===== HARDWARE VALIDATION MACROS =====
// #define BOARD_VALIDATE_PIN(pin) ((pin) >= 0 && (pin) <= 48)
// #define BOARD_VALIDATE_I2C_ADDR(addr) ((addr) >= 0x08 && (addr) <= 0x77)

// ===== HARDWARE CONFLICT DETECTION =====
// #define BOARD_CHECK_PIN_CONFLICTS() do { \
//     static_assert(BOARD_EPD_CS != BOARD_SD_CS, "EPD and SD CS pins conflict!"); \
//     static_assert(BOARD_EPD_CS != BOARD_LORA_CS, "EPD and LoRa CS pins conflict!"); \
//     static_assert(BOARD_SD_CS != BOARD_LORA_CS, "SD and LoRa CS pins conflict!"); \
// } while(0)

// ===== UTILITY MACROS =====
// #define BOARD_PIN_HIGH(pin)     digitalWrite(pin, HIGH)
// #define BOARD_PIN_LOW(pin)      digitalWrite(pin, LOW)
// #define BOARD_PIN_READ(pin)     digitalRead(pin)
// #define BOARD_PIN_MODE(pin, mode) pinMode(pin, mode)

// ===== POWER STATES =====
typedef enum {
    BOARD_POWER_ACTIVE,
    BOARD_POWER_LIGHT_SLEEP,
    BOARD_POWER_DEEP_SLEEP,
    BOARD_POWER_HIBERNATE
} board_power_state_t;

// ===== PERIPHERAL STATES =====
// typedef enum {
//     BOARD_PERIPHERAL_OFF,
//     BOARD_PERIPHERAL_STANDBY,
//     BOARD_PERIPHERAL_ACTIVE,
//     BOARD_PERIPHERAL_ERROR
// } board_peripheral_state_t;

// ===== BOARD INITIALIZATION FUNCTIONS =====
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize board hardware with corrected pin configuration
 * @return true if successful, false otherwise
 */
bool board_init(void);

/**
 * @brief Validate hardware pin configuration
 * @return true if all pins are correctly configured
 */
bool board_validate_pins(void);

/**
 * @brief Configure power management with corrected settings
 * @param state Power state to enter
 * @return true if successful, false otherwise
 */
bool board_set_power_state(board_power_state_t state);

/**
 * @brief Get battery voltage in millivolts using correct ADC
 * @return Battery voltage in mV
 */
uint16_t board_get_battery_voltage(void);

/**
 * @brief Check if USB power is connected using correct pin
 * @return true if USB connected, false otherwise
 */
bool board_is_usb_connected(void);

/**
 * @brief Initialize SPI bus with correct pins
 * @return true if successful
 */
bool board_init_spi(void);

/**
 * @brief Initialize I2C bus with correct pins
 * @return true if successful
 */
bool board_init_i2c(void);

/**
 * @brief Enable/disable peripheral power
 * @param peripheral Which peripheral to control
 * @param enable true to enable, false to disable
 * @return true if successful
 */
bool board_set_peripheral_power(const char* peripheral, bool enable);

#ifdef __cplusplus
}
#endif

#endif // BOARD_CONFIG_H