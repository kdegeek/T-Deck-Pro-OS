/**
 * @file os_config.h
 * @brief T-Deck-Pro OS System Configuration
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Central configuration file for the entire operating system
 */

#ifndef OS_CONFIG_H
#define OS_CONFIG_H

#include <Arduino.h>

// ===== SYSTEM INFORMATION =====
#define OS_NAME "T-Deck-Pro OS"
#define OS_VERSION "1.0.0"
#define OS_BUILD_DATE __DATE__
#define OS_BUILD_TIME __TIME__

// ===== HARDWARE CONFIGURATION =====

// Display Configuration
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 320
#define DISPLAY_ROTATION 0
#define DISPLAY_REFRESH_RATE 100  // ms

// E-ink Display Pins
#define EINK_CS   34
#define EINK_DC   35
#define EINK_BUSY 37
#define EINK_RST  -1
#define EINK_SCK  36
#define EINK_MOSI 33

// Keyboard/Input Pins
#define KEYBOARD_SDA 18
#define KEYBOARD_SCL 8
#define KEYBOARD_INT 46

// LoRa Pins
#define LORA_CS   10
#define LORA_RST  5
#define LORA_DIO1 3
#define LORA_BUSY 4

// SD Card Pins
#define SD_CS     13
#define SD_MOSI   11
#define SD_MISO   2
#define SD_SCK    14

// Power Management
#define BATTERY_ADC_PIN 1
#define POWER_EN_PIN    15

// ===== SYSTEM TIMING =====
#define MAIN_LOOP_INTERVAL 100    // Main system loop interval (ms)
#define HEARTBEAT_INTERVAL 30000  // System heartbeat interval (ms)
#define DISPLAY_UPDATE_INTERVAL 1000  // Display refresh interval (ms)
#define PLUGIN_SCAN_INTERVAL 5000     // Plugin scan interval (ms)

// ===== MEMORY CONFIGURATION =====
#define MAX_LOG_MESSAGES 100
#define MAX_PLUGINS 10
#define MAX_MQTT_MESSAGE_SIZE 512
#define PLUGIN_STACK_SIZE 8192

// ===== NETWORK CONFIGURATION =====
#define WIFI_CONNECT_TIMEOUT 30000  // WiFi connection timeout (ms)
#define MQTT_CONNECT_TIMEOUT 10000  // MQTT connection timeout (ms)
#define MQTT_KEEPALIVE_INTERVAL 60  // MQTT keepalive interval (s)

// Default MQTT Topics
#define MQTT_TOPIC_HEARTBEAT "tdeckpro/%s/heartbeat"
#define MQTT_TOPIC_STATUS "tdeckpro/%s/status"
#define MQTT_TOPIC_LAUNCH "tdeckpro/%s/launch"
#define MQTT_TOPIC_TELEMETRY "tdeckpro/%s/telemetry"

// ===== APPLICATION CONFIGURATION =====
#define APP_GRID_COLS 3
#define APP_GRID_ROWS 3
#define APP_ICON_SIZE 64
#define APP_NAME_MAX_LENGTH 16

// SD Card App Paths
#define SD_APPS_PATH "/apps"
#define SD_CONFIG_PATH "/config"
#define SD_LOGS_PATH "/logs"

// ===== BOOT CONFIGURATION =====
#define BOOT_SPLASH_DURATION 2000   // Boot splash screen duration (ms)
#define BOOT_TIMEOUT 30000          // Maximum boot time (ms)
#define EMERGENCY_MODE_ACTIVATION_TIME 5000 // Emergency mode activation time (ms)

// ===== POWER MANAGEMENT =====
#define BATTERY_LOW_THRESHOLD 20    // Low battery percentage
#define BATTERY_CRITICAL_THRESHOLD 5  // Critical battery percentage
#define SLEEP_TIMEOUT 300000        // Auto-sleep timeout (ms) - 5 minutes
#define DEEP_SLEEP_TIMEOUT 1800000  // Deep sleep timeout (ms) - 30 minutes

// ===== LOGGING CONFIGURATION =====
#define LOG_LEVEL_DEFAULT LogLevel::INFO
#define LOG_SERIAL_ENABLED true
#define LOG_DISPLAY_ENABLED true
#define LOG_SD_ENABLED true
#define LOG_MQTT_ENABLED false

// ===== FEATURE FLAGS =====
#define FEATURE_MQTT_ENABLED true
#define FEATURE_TAILSCALE_ENABLED false  // Future feature
#define FEATURE_MESHTASTIC_ENABLED true
#define FEATURE_FILE_MANAGER_ENABLED true
#define FEATURE_SETTINGS_ENABLED true
#define FEATURE_HARDWARE_TEST_ENABLED true

// ===== DEBUG CONFIGURATION =====
#ifdef DEBUG
#define DEBUG_SERIAL_ENABLED true
#define DEBUG_VERBOSE_LOGGING true
#define DEBUG_MEMORY_TRACKING true
#else
#define DEBUG_SERIAL_ENABLED false
#define DEBUG_VERBOSE_LOGGING false
#define DEBUG_MEMORY_TRACKING false
#endif

// ===== SYSTEM LIMITS =====
#define MAX_FILENAME_LENGTH 64
#define MAX_PATH_LENGTH 256
#define MAX_WIFI_NETWORKS 10
#define MAX_MQTT_SUBSCRIPTIONS 20

// ===== UI CONFIGURATION =====
#define UI_FONT_SMALL 1
#define UI_FONT_MEDIUM 2
#define UI_FONT_LARGE 3
#define UI_COLOR_BLACK 0x0000
#define UI_COLOR_WHITE 0xFFFF
#define UI_MARGIN 5
#define UI_PADDING 3

// ===== DEVICE IDENTIFICATION =====
extern String DEVICE_ID;  // Generated at runtime from MAC address
extern String DEVICE_NAME; // User-configurable device name

// ===== CONFIGURATION VALIDATION =====
#if MAIN_LOOP_INTERVAL < 50
#error "Main loop interval too fast - minimum 50ms required"
#endif

#if MAX_PLUGINS > 20
#error "Too many plugins - maximum 20 supported"
#endif

#if PLUGIN_STACK_SIZE < 4096
#error "Plugin stack size too small - minimum 4KB required"
#endif

// ===== UTILITY MACROS =====
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) (MIN(MAX(x, min), max))

// ===== SYSTEM STATE ENUM =====
enum class SystemState {
    BOOTING,
    RUNNING,
    EMERGENCY,
    SLEEPING,
    DEEP_SLEEP,
    SHUTDOWN
};

// ===== BOOT STAGE ENUM =====
enum class BootStage {
    HARDWARE_INIT,
    DISPLAY_INIT,
    STORAGE_INIT,
    NETWORK_INIT,
    PLUGIN_SCAN,
    LAUNCHER_START,
    COMPLETE
};

#endif // OS_CONFIG_H
