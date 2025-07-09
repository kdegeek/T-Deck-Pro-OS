/**
 * @file os_config.h
 * @brief T-Deck-Pro Simplified OS Configuration - CORRECTED HARDWARE PINS
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note All pin definitions corrected to match .REFERENCE/README.md specifications
 */

#ifndef OS_CONFIG_H
#define OS_CONFIG_H

#include <ArduinoJson.h>

// ===== OS VERSION =====
#define OS_VERSION "1.0.0-simplified-fixed"
#define OS_BUILD_DATE __DATE__
#define OS_BUILD_TIME __TIME__

// ===== HARDWARE VERSION =====
#define BOARD_T_DECK_PRO_VERSION "v1.0-241106"

// ===== SERIAL CONFIGURATION =====
#define SerialMon   Serial      // Main debug serial
#define SerialAT    Serial1     // 4G modem serial
#define SerialGPS   Serial2     // GPS serial

// ===== I2C ADDRESSES - CORRECTED FROM REFERENCE =====
#define BOARD_I2C_ADDR_TOUCH      0x1A // Touch controller - CST328
#define BOARD_I2C_ADDR_LTR_553ALS 0x23 // Light sensor - LTR_553ALS
#define BOARD_I2C_ADDR_GYROSCOPE  0x28 // Gyroscope - BHI260AP (fixed typo)
#define BOARD_I2C_ADDR_KEYBOARD   0x34 // Keyboard controller - TCA8418
#define BOARD_I2C_ADDR_BQ27220    0x55 // Battery gauge
#define BOARD_I2C_ADDR_BQ25896    0x6B // Power management

// ===== I2C PINS - CORRECTED FROM REFERENCE =====
#define BOARD_I2C_SDA  13   // CORRECTED: was 18, now matches reference
#define BOARD_I2C_SCL  14   // CORRECTED: was 8, now matches reference

// ===== A7682E 4G MODEM - VERIFIED CORRECT =====
#define BOARD_A7682E_RI     7
#define BOARD_A7682E_ITR    8
#define BOARD_A7682E_RST    9
#define BOARD_A7682E_RXD    10
#define BOARD_A7682E_TXD    11
#define BOARD_A7682E_PWRKEY 40

// ===== I2S AUDIO PINS - CORRECTED FROM REFERENCE =====
// CRITICAL FIX: These were completely wrong before
#define BOARD_I2S_BCLK 7    // CORRECTED: was 17, now matches reference
#define BOARD_I2S_DOUT 8    // CORRECTED: was 18, now matches reference  
#define BOARD_I2S_LRC  9    // CORRECTED: was 19, now matches reference

// ===== KEYBOARD - CORRECTED =====
#define BOARD_KEYBOARD_SCL BOARD_I2C_SCL
#define BOARD_KEYBOARD_SDA BOARD_I2C_SDA
#define BOARD_KEYBOARD_INT 15
#define BOARD_KEYBOARD_LED 42

// ===== TOUCHSCREEN - CORRECTED =====
#define BOARD_TOUCH_SCL BOARD_I2C_SCL
#define BOARD_TOUCH_SDA BOARD_I2C_SDA
#define BOARD_TOUCH_INT 12
#define BOARD_TOUCH_RST 45

// ===== LIGHT SENSOR - CORRECTED =====
#define BOARD_ALS_SCL BOARD_I2C_SCL
#define BOARD_ALS_SDA BOARD_I2C_SDA
#define BOARD_ALS_INT 16

// ===== GYROSCOPE - CORRECTED =====
#define BOARD_GYROSCOPE_SCL BOARD_I2C_SCL
#define BOARD_GYROSCOPE_SDA BOARD_I2C_SDA
#define BOARD_GYROSCOPE_INT 21
#define BOARD_GYROSCOPE_RST -1

// ===== SPI PINS - VERIFIED CORRECT =====
#define BOARD_SPI_SCK  36
#define BOARD_SPI_MOSI 33
#define BOARD_SPI_MISO 47

// ===== E-INK DISPLAY - CORRECTED FROM REFERENCE =====
#define LCD_HOR_SIZE 240
#define LCD_VER_SIZE 320
#define DISP_BUF_SIZE (LCD_HOR_SIZE * LCD_VER_SIZE)

// CRITICAL FIXES: Display pins were completely wrong
#define BOARD_EPD_SCK  BOARD_SPI_SCK
#define BOARD_EPD_MOSI BOARD_SPI_MOSI
#define BOARD_EPD_DC   35   // CORRECTED: was 11, now matches reference
#define BOARD_EPD_CS   34   // CORRECTED: was 10, now matches reference
#define BOARD_EPD_BUSY 37   // CORRECTED: was 13, now matches reference
#define BOARD_EPD_RST  -1   // CORRECTED: was 12, now matches reference

// ===== SD CARD - CORRECTED =====
#define BOARD_SD_CS   48    // CORRECTED: matches reference
#define BOARD_SD_SCK  BOARD_SPI_SCK
#define BOARD_SD_MOSI BOARD_SPI_MOSI
#define BOARD_SD_MISO BOARD_SPI_MISO

// ===== LORA - VERIFIED CORRECT =====
#define BOARD_LORA_SCK  BOARD_SPI_SCK
#define BOARD_LORA_MOSI BOARD_SPI_MOSI
#define BOARD_LORA_MISO BOARD_SPI_MISO
#define BOARD_LORA_CS   3
#define BOARD_LORA_BUSY 6
#define BOARD_LORA_RST  4
#define BOARD_LORA_INT  5

// ===== GPS - VERIFIED CORRECT =====
#define BOARD_GPS_RXD 44
#define BOARD_GPS_TXD 43
#define BOARD_GPS_PPS 1

// ===== CONTROL PINS - VERIFIED CORRECT =====
#define BOARD_BOOT_PIN  0
#define BOARD_MOTOR_PIN 2

// ===== ENABLE PINS - VERIFIED CORRECT =====
#define BOARD_GPS_EN  39  // Enable GPS module
#define BOARD_1V8_EN  38  // Enable gyroscope module
#define BOARD_6609_EN 41  // Enable 4G module
#define BOARD_LORA_EN 46  // Enable LoRa module

// ===== MICROPHONE - CORRECTED FROM REFERENCE =====
#define BOARD_MIC_DATA  17  // CORRECTED: was 20, now matches reference
#define BOARD_MIC_CLOCK 18  // CORRECTED: was 22, now matches reference

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
#define MQTT_TOPIC_APP_LAUNCH MQTT_TOPIC_PREFIX "/app_launch"
#define MQTT_TOPIC_NOTIFICATION MQTT_TOPIC_PREFIX "/notification"
#define MQTT_TOPIC_SYSTEM_CMD MQTT_TOPIC_PREFIX "/system_cmd"
#define MQTT_TOPIC_APP_RESULT MQTT_TOPIC_PREFIX "/app_result"
#define MQTT_TOPIC_BROADCAST_NOTIFICATION MQTT_TOPIC_PREFIX "/broadcast/notification"
#define MQTT_TOPIC_BROADCAST_SYSTEM MQTT_TOPIC_PREFIX "/broadcast/system"

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
#define FEATURE_AUDIO_ENABLED    0  // Disabled for 4G version without audio chip
#define FEATURE_TAILSCALE_ENABLED 1
#define FEATURE_MQTT_ENABLED     1
#define FEATURE_PLUGINS_ENABLED  1

// ===== HARDWARE VALIDATION MACROS =====
#define VALIDATE_PIN_ASSIGNMENT() do { \
    static_assert(BOARD_I2S_BCLK != BOARD_MIC_DATA, "I2S pins conflict with microphone!"); \
    static_assert(BOARD_I2S_DOUT != BOARD_MIC_CLOCK, "I2S pins conflict with microphone!"); \
    static_assert(BOARD_EPD_CS != BOARD_SD_CS, "Display and SD card CS pins conflict!"); \
    static_assert(BOARD_EPD_CS != BOARD_LORA_CS, "Display and LoRa CS pins conflict!"); \
} while(0)

#endif // OS_CONFIG_H