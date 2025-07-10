/**
 * @file board_config_corrected.h
 * @brief T-Deck-Pro Board Configuration - CORRECTED HARDWARE PINS
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note All pin definitions corrected to match hardware specifications
 */

#ifndef BOARD_CONFIG_CORRECTED_H
#define BOARD_CONFIG_CORRECTED_H

#include <Arduino.h>
#include "../config/os_config_corrected.h"

// ===== BOARD IDENTIFICATION =====
#define BOARD_NAME "T-Deck-Pro"
#define BOARD_VERSION "v1.0"
#define BOARD_MANUFACTURER "LilyGo"

// ===== MCU CONFIGURATION =====
#define BOARD_MCU "ESP32-S3"
#define BOARD_CPU_FREQ 240000000  // 240 MHz
#define BOARD_FLASH_SIZE 16777216  // 16 MB
#define BOARD_PSRAM_SIZE 8388608   // 8 MB

// ===== SERIAL CONFIGURATION =====
#define SerialMon   Serial      // Main debug serial
#define SerialAT    Serial1     // 4G modem serial
#define SerialGPS   Serial2     // GPS serial

// ===== I2C CONFIGURATION =====
#define BOARD_I2C_SDA  13   // I2C SDA pin
#define BOARD_I2C_SCL  14   // I2C SCL pin
#define BOARD_I2C_FREQ 400000  // 400 kHz

// ===== I2C DEVICE ADDRESSES =====
#define BOARD_I2C_ADDR_TOUCH      0x1A // Touch controller - CST328
#define BOARD_I2C_ADDR_LTR_553ALS 0x23 // Light sensor - LTR_553ALS
#define BOARD_I2C_ADDR_GYROSCOPE  0x28 // Gyroscope - BHI260AP
#define BOARD_I2C_ADDR_KEYBOARD   0x34 // Keyboard controller - TCA8418
#define BOARD_I2C_ADDR_BQ27220    0x55 // Battery gauge
#define BOARD_I2C_ADDR_BQ25896    0x6B // Power management

// ===== SPI CONFIGURATION =====
#define BOARD_SPI_SCK  36   // SPI Clock
#define BOARD_SPI_MOSI 33   // SPI MOSI
#define BOARD_SPI_MISO 47   // SPI MISO
#define BOARD_SPI_FREQ 40000000  // 40 MHz

// ===== E-INK DISPLAY CONFIGURATION =====
#define LCD_HOR_SIZE 240
#define LCD_VER_SIZE 320
#define DISP_BUF_SIZE (LCD_HOR_SIZE * LCD_VER_SIZE)

#define BOARD_EPD_SCK  BOARD_SPI_SCK
#define BOARD_EPD_MOSI BOARD_SPI_MOSI
#define BOARD_EPD_DC   35   // Display Data/Command
#define BOARD_EPD_CS   34   // Display Chip Select
#define BOARD_EPD_BUSY 37   // Display Busy
#define BOARD_EPD_RST  -1   // No reset pin
#define BOARD_EPD_MISO BOARD_SPI_MISO

// ===== SD CARD CONFIGURATION =====
#define BOARD_SD_CS   48    // SD Card Chip Select
#define BOARD_SD_SCK  BOARD_SPI_SCK
#define BOARD_SD_MOSI BOARD_SPI_MOSI
#define BOARD_SD_MISO BOARD_SPI_MISO

// ===== LORA CONFIGURATION =====
#define BOARD_LORA_SCK  BOARD_SPI_SCK
#define BOARD_LORA_MOSI BOARD_SPI_MOSI
#define BOARD_LORA_MISO BOARD_SPI_MISO
#define BOARD_LORA_CS   3    // LoRa Chip Select
#define BOARD_LORA_BUSY 6    // LoRa Busy
#define BOARD_LORA_RST  4    // LoRa Reset
#define BOARD_LORA_INT  5    // LoRa Interrupt

// ===== A7682E 4G MODEM CONFIGURATION =====
#define BOARD_A7682E_RI     7    // Ring Indicator
#define BOARD_A7682E_ITR    8    // Interrupt
#define BOARD_A7682E_RST    9    // Reset
#define BOARD_A7682E_RXD    10   // Receive Data
#define BOARD_A7682E_TXD    11   // Transmit Data
#define BOARD_A7682E_PWRKEY 40   // Power Key

// ===== GPS CONFIGURATION =====
#define BOARD_GPS_RXD 44    // GPS Receive Data
#define BOARD_GPS_TXD 43    // GPS Transmit Data
#define BOARD_GPS_PPS 1     // GPS PPS Signal

// ===== I2S AUDIO CONFIGURATION =====
#define BOARD_I2S_BCLK 7    // I2S Bit Clock
#define BOARD_I2S_DOUT 8    // I2S Data Out
#define BOARD_I2S_LRC  9    // I2S Left Right Clock

// ===== KEYBOARD CONFIGURATION =====
#define BOARD_KEYBOARD_SCL BOARD_I2C_SCL
#define BOARD_KEYBOARD_SDA BOARD_I2C_SDA
#define BOARD_KEYBOARD_INT 15    // Keyboard Interrupt
#define BOARD_KEYBOARD_LED 42    // Keyboard LED

// ===== TOUCHSCREEN CONFIGURATION =====
#define BOARD_TOUCH_SCL BOARD_I2C_SCL
#define BOARD_TOUCH_SDA BOARD_I2C_SDA
#define BOARD_TOUCH_INT 12   // Touch Interrupt
#define BOARD_TOUCH_RST 45   // Touch Reset

// ===== LIGHT SENSOR CONFIGURATION =====
#define BOARD_ALS_SCL BOARD_I2C_SCL
#define BOARD_ALS_SDA BOARD_I2C_SDA
#define BOARD_ALS_INT 16     // Light Sensor Interrupt

// ===== GYROSCOPE CONFIGURATION =====
#define BOARD_GYROSCOPE_SCL BOARD_I2C_SCL
#define BOARD_GYROSCOPE_SDA BOARD_I2C_SDA
#define BOARD_GYROSCOPE_INT 21    // Gyroscope Interrupt
#define BOARD_GYROSCOPE_RST -1    // No reset pin

// ===== MICROPHONE CONFIGURATION =====
#define BOARD_MIC_DATA  17   // Microphone Data
#define BOARD_MIC_CLOCK 18   // Microphone Clock

// ===== CONTROL PINS =====
#define BOARD_BOOT_PIN  0    // Boot Button
#define BOARD_MOTOR_PIN 2    // Vibration Motor

// ===== ENABLE PINS =====
#define BOARD_GPS_EN  39  // Enable GPS module
#define BOARD_1V8_EN  38  // Enable gyroscope module
#define BOARD_6609_EN 41  // Enable 4G module
#define BOARD_LORA_EN 46  // Enable LoRa module

// ===== LED CONFIGURATION =====
#define BOARD_LED_PIN 2      // Built-in LED
#define BOARD_LED_ON  HIGH
#define BOARD_LED_OFF LOW

// ===== BUTTON CONFIGURATION =====
#define BOARD_BUTTON_PIN 0   // Boot button
#define BOARD_BUTTON_ACTIVE LOW

// ===== POWER MANAGEMENT =====
#define BATTERY_LOW_THRESHOLD_MV    3300
#define BATTERY_CRITICAL_THRESHOLD_MV 3100
#define POWER_SAVE_TIMEOUT_MS      300000  // 5 minutes

// ===== DISPLAY REFRESH MODES =====
#define DISP_REFR_MODE_FULL 0
#define DISP_REFR_MODE_PART 1

// ===== TASK PRIORITIES =====
#define PRIORITY_CRITICAL 4
#define PRIORITY_HIGH     3
#define PRIORITY_NORMAL   2
#define PRIORITY_LOW      1

#define A7682E_PRIORITY   PRIORITY_NORMAL
#define MQTT_PRIORITY     PRIORITY_HIGH
#define UI_PRIORITY       PRIORITY_HIGH
#define PLUGIN_PRIORITY   PRIORITY_NORMAL

// ===== MEMORY CONFIGURATION =====
#define OS_STACK_SIZE_SMALL  2048
#define OS_STACK_SIZE_MEDIUM 4096
#define OS_STACK_SIZE_LARGE  8192

// ===== CONNECTIVITY =====
#define WIFI_MAX_RETRIES 3
#define CELLULAR_MAX_RETRIES 5
#define LORA_DEFAULT_FREQUENCY 915000000  // 915 MHz

// ===== PERIPHERAL STATE =====
typedef enum {
    BOARD_PERIPHERAL_OFF = 0,
    BOARD_PERIPHERAL_STANDBY = 1,
    BOARD_PERIPHERAL_ACTIVE = 2,
    BOARD_PERIPHERAL_ERROR = 3
} board_peripheral_state_t;

// ===== BOARD INITIALIZATION FUNCTIONS =====

/**
 * @brief Initialize board-specific hardware
 * @return true if successful
 */
bool board_init();

/**
 * @brief Initialize GPIO pins
 * @return true if successful
 */
bool board_init_gpio();

/**
 * @brief Initialize I2C bus
 * @return true if successful
 */
bool board_init_i2c();

/**
 * @brief Initialize SPI bus
 * @return true if successful
 */
bool board_init_spi();

/**
 * @brief Initialize UART interfaces
 * @return true if successful
 */
bool board_init_uart();

/**
 * @brief Check if I2C device is present
 * @param address I2C device address
 * @return true if device responds
 */
bool board_i2c_device_present(uint8_t address);

/**
 * @brief Get board information string
 * @return Board info string
 */
String board_get_info();

/**
 * @brief Get board version string
 * @return Board version string
 */
String board_get_version();

#endif // BOARD_CONFIG_CORRECTED_H 