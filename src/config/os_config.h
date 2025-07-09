/**
 * @file os_config.h
 * @brief T-Deck-Pro Simplified OS Configuration
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef OS_CONFIG_H
#define OS_CONFIG_H

#include <ArduinoJson.h>

// ===== OS VERSION =====
#define OS_VERSION "1.0.0-simplified"
#define OS_BUILD_DATE __DATE__
#define OS_BUILD_TIME __TIME__

// ===== HARDWARE VERSION =====
#define BOARD_T_DECK_PRO_VERSION "v1.0-241106"

// ===== SERIAL CONFIGURATION =====
#define SerialMon   Serial      // Main debug serial
#define SerialAT    Serial1     // 4G modem serial
#define SerialGPS   Serial2     // GPS serial

// ===== I2C ADDRESSES =====
#define BOARD_I2C_ADDR_TOUCH      0x1A // Touch controller - CST328
#define BOARD_I2C_ADDR_LTR_553ALS 0x23 // Light sensor - LTR_553ALS
#define BOARD_I2C_ADDR_GYROSCOPE  0x28 // Gyroscope - BHI260AP
#define BOARD_I2C_ADDR_KEYBOARD   0x34 // Keyboard controller - TCA8418
#define BOARD_I2C_ADDR_BQ27220    0x55 // Battery gauge
#define BOARD_I2C_ADDR_BQ25896    0x6B // Power management

// ===== I2C PINS =====
#define BOARD_I2C_SDA  13
#define BOARD_I2C_SCL  14

// ===== I2S AUDIO PINS =====
#define BOARD_I2S_BCLK 7
#define BOARD_I2S_DOUT 8
#define BOARD_I2S_LRC  9

// ===== KEYBOARD =====
#define BOARD_KEYBOARD_SCL BOARD_I2C_SCL
#define BOARD_KEYBOARD_SDA BOARD_I2C_SDA
#define BOARD_KEYBOARD_INT 15
#define BOARD_KEYBOARD_LED 42

// ===== TOUCHSCREEN =====
#define BOARD_TOUCH_SCL BOARD_I2C_SCL
#define BOARD_TOUCH_SDA BOARD_I2C_SDA
#define BOARD_TOUCH_INT 12
#define BOARD_TOUCH_RST 45

// ===== LIGHT SENSOR =====
#define BOARD_ALS_SCL BOARD_I2C_SCL
#define BOARD_ALS_SDA BOARD_I2C_SDA
#define BOARD_ALS_INT 16

// ===== GYROSCOPE =====
#define BOARD_GYROSCOPE_SCL BOARD_I2C_SCL
#define BOARD_GYROSCOPE_SDA BOARD_I2C_SDA
#define BOARD_GYROSCOPE_INT 21
#define BOARD_GYROSCOPE_RST -1

// ===== SPI PINS =====
#define BOARD_SPI_SCK  36
#define BOARD_SPI_MOSI 33
#define BOARD_SPI_MISO 47

// ===== E-INK DISPLAY =====
#define LCD_HOR_SIZE 240
#define LCD_VER_SIZE 320
#define DISP_BUF_SIZE (LCD_HOR_SIZE * LCD_VER_SIZE)

#define BOARD_EPD_SCK  BOARD_SPI_SCK
#define BOARD_EPD_MOSI BOARD_SPI_MOSI
#define BOARD_EPD_DC   35
#define BOARD_EPD_CS   34
#define BOARD_EPD_BUSY 37
#define BOARD_EPD_RST  -1

// ===== SD CARD =====
#define BOARD_SD_CS   48
#define BOARD_SD_SCK  BOARD_SPI_SCK
#define BOARD_SD_MOSI BOARD_SPI_MOSI
#define BOARD_SD_MISO BOARD_SPI_MISO

// ===== LORA =====
#define BOARD_LORA_SCK  BOARD_SPI_SCK
#define BOARD_LORA_MOSI BOARD_SPI_MOSI
#define BOARD_LORA_MISO BOARD_SPI_MISO
#define BOARD_LORA_CS   3
#define BOARD_LORA_BUSY 6
#define BOARD_LORA_RST  4
#define BOARD_LORA_INT  5

// ===== GPS =====
#define BOARD_GPS_RXD 44
#define BOARD_GPS_TXD 43
#define BOARD_GPS_PPS 1

// ===== 4G MODEM (A7682E) =====
#define BOARD_A7682E_RI     7
#define BOARD_A7682E_ITR    8
#define BOARD_A7682E_RST    9
#define BOARD_A7682E_RXD    10
#define BOARD_A7682E_TXD    11
#define BOARD_A7682E_PWRKEY 40

// ===== CONTROL PINS =====
#define BOARD_BOOT_PIN  0
#define BOARD_MOTOR_PIN 2

// ===== ENABLE PINS =====
#define BOARD_GPS_EN  39  // Enable GPS module
#define BOARD_1V8_EN  38  // Enable gyroscope module
#define BOARD_6609_EN 41  // Enable 4G module
#define BOARD_LORA_EN 46  // Enable LoRa module

// ===== MICROPHONE =====
#define BOARD_MIC_DATA  17
#define BOARD_MIC_CLOCK 18

// ===== OS CONFIGURATION =====
#define OS_MAX_APPS 16
#define OS_APP_TIMEOUT_MS 30000
#define OS_MQTT_KEEPALIVE 60
#define OS_WIFI_TIMEOUT_MS 10000
#define OS_BOOT_TIMEOUT_MS 30000
#define MAIN_LOOP_INTERVAL 100  // Main loop interval in milliseconds

// ===== MQTT TOPICS =====
#define MQTT_TOPIC_PREFIX "tdeck-pro"
#define MQTT_TOPIC_APPS MQTT_TOPIC_PREFIX "/apps"
#define MQTT_TOPIC_NOTIFICATIONS MQTT_TOPIC_PREFIX "/notifications"
#define MQTT_TOPIC_STATUS MQTT_TOPIC_PREFIX "/status"
#define MQTT_TOPIC_COMMANDS MQTT_TOPIC_PREFIX "/commands"
#define MQTT_TOPIC_TELEMETRY MQTT_TOPIC_PREFIX "/telemetry"

// ===== STORAGE PATHS =====
#define OS_SETTINGS_FILE "/settings.json"
#define OS_APPS_DIR "/apps"
#define OS_LOGS_DIR "/logs"
#define OS_CACHE_DIR "/cache"
#define OS_TAILSCALE_CONFIG "/tailscale.conf"

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

// ===== POWER MANAGEMENT =====
#define BATTERY_LOW_THRESHOLD_MV    3300
#define BATTERY_CRITICAL_THRESHOLD_MV 3100
#define POWER_SAVE_TIMEOUT_MS      300000  // 5 minutes

// ===== CONNECTIVITY =====
#define WIFI_MAX_RETRIES 3
#define CELLULAR_MAX_RETRIES 5
#define LORA_DEFAULT_FREQUENCY 915000000  // 915 MHz

// ===== DEBUG CONFIGURATION =====
#ifdef DEBUG
    #define OS_DEBUG_ENABLED 1
    #define OS_LOG_LEVEL 4  // Verbose
#else
    #define OS_DEBUG_ENABLED 0
    #define OS_LOG_LEVEL 2  // Warnings and errors only
#endif

// ===== FEATURE FLAGS =====
#define FEATURE_WIFI_ENABLED     1
#define FEATURE_CELLULAR_ENABLED 1
#define FEATURE_LORA_ENABLED     1
#define FEATURE_GPS_ENABLED      1
#define FEATURE_BLUETOOTH_ENABLED 1
#define FEATURE_AUDIO_ENABLED    1
#define FEATURE_TAILSCALE_ENABLED 1
#define FEATURE_MQTT_ENABLED     1
#define FEATURE_PLUGINS_ENABLED  1

#endif // OS_CONFIG_H