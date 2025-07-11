---
type: "always_apply"
---

# T-Deck-Pro ESP32-S3 Firmware Development Environment

You are helping develop firmware for the T-Deck-Pro, an ESP32-S3 portable device with:

## Hardware Specifications

### Core Components
- **MCU**: ESP32-S3 (240MHz, 16MB Flash, 8MB PSRAM)
- **Display**: 320x240 E-paper (GDEQ031T10) with slow refresh limitations
- **Communication**: LoRa SX1262, GPS MIA-M10Q, A7682E 4G modem, WiFi 802.11 b/g/n
- **Input**: TCA8418 keyboard matrix, CST328 touch controller
- **Sensors**: BHI260AP gyroscope, LTR-553ALS light sensor, BQ27220 battery gauge, BQ25896 power management
- **Storage**: MicroSD card slot, 18650 battery with USB-C charging

### Pin Configuration
- **I2C**: SDA=13, SCL=14 (shared by multiple devices)
- **E-paper**: CS=34, DC=35, BUSY=37, RST=-1 (no reset pin)
- **SPI**: Shared bus for E-paper, LoRa, SD card
- **4G Modem**: RXD=10, TXD=11, PWRKEY=40, RST=9
- **LoRa**: CS=3, RST=4, DIO1=5, BUSY=6

## Critical Development Constraints

### Power Management
- **Battery-powered device** requiring aggressive power management
- **Battery thresholds**: Low=3.3V, Critical=3.0V
- **Sleep modes**: Light sleep, deep sleep, hibernate
- **CPU frequency scaling**: 240MHz → 80MHz → sleep

### Display Limitations
- **Very slow refresh rate** (1-2 seconds per update)
- **Partial update optimization** required for responsive UI
- **1-bit color depth** (black/white only)
- **5-second LVGL refresh period** for E-paper compatibility

### Memory Management
- **Limited RAM** - must use PSRAM for large buffers
- **Dynamic app loading** from SD card to RAM
- **LRU app eviction** when memory pressure detected
- **Flash storage constraints** (~4MB available for apps)

### Real-time Communication
- **Multi-peripheral coordination** (WiFi, LoRa, Cellular, GPS)
- **MQTT-based server integration** with mesh networking
- **OTA updates** for firmware and applications
- **Hardware pin conflicts** (audio disabled on 4G version)

## Code Style & Architecture

### FreeRTOS Task Structure
```cpp
// Task priorities (higher = more important)
#define UI_TASK_PRIORITY           (configMAX_PRIORITIES - 1)
#define DISPLAY_TASK_PRIORITY      (configMAX_PRIORITIES - 2)
#define INPUT_TASK_PRIORITY        (configMAX_PRIORITIES - 3)
#define LORA_TASK_PRIORITY         (configMAX_PRIORITIES - 4)
#define WIFI_TASK_PRIORITY         (configMAX_PRIORITIES - 5)
#define CELLULAR_TASK_PRIORITY     (configMAX_PRIORITIES - 6)
#define SYSTEM_TASK_PRIORITY       (configMAX_PRIORITIES - 7)
```

### Error Handling & Timeouts
- **Comprehensive error recovery** with logging
- **Proper timeout handling** for all I/O operations
- **Watchdog timer integration** for system stability
- **Graceful degradation** when peripherals fail

### Memory Optimization
- **PSRAM utilization** for large buffers and app data
- **Stack size optimization** (2KB-8KB per task)
- **Heap management** with fragmentation prevention
- **Smart caching** of frequently used components

### Arduino/PlatformIO Conventions
- **PlatformIO build system** with ESP32-S3 toolchain
- **Arduino framework** with FreeRTOS integration
- **Library dependency management** via platformio.ini
- **Multiple build configurations** (debug, release, OTA)

## Key Libraries & Dependencies

### Core Libraries
- **GxEPD2**: E-paper display driver for GDEQ031T10
- **RadioLib**: LoRa SX1262 communication
- **TinyGSM**: A7682E 4G modem support
- **TinyGPSPlus**: MIA-M10Q GPS module
- **LVGL**: UI framework optimized for E-paper
- **PubSubClient**: MQTT communication

### Hardware Libraries
- **Adafruit TCA8418**: Keyboard matrix controller
- **XPowersLib**: BQ25896/BQ27220 power management
- **SensorLib**: BHI260AP gyroscope, CST328 touch
- **ArduinoJson**: Configuration and data serialization

### Development Tools
- **ESP32 Arduino Core**: Base framework
- **FreeRTOS**: Real-time operating system
- **SPIFFS/LittleFS**: File system management
- **SD**: SD card interface

## Development Guidelines

### Always Consider:
1. **Power consumption** - every operation affects battery life
2. **E-paper limitations** - slow refresh, partial update optimization
3. **Memory constraints** - use PSRAM, implement LRU eviction
4. **Real-time requirements** - proper task prioritization
5. **Hardware pin conflicts** - verify pin assignments
6. **Multi-peripheral coordination** - avoid resource conflicts

### Code Quality Standards:
- **Comprehensive error handling** with meaningful error messages
- **Proper resource cleanup** in destructors and error paths
- **Memory leak prevention** with smart pointers and RAII
- **Thread-safe operations** with mutexes and semaphores
- **Efficient algorithms** optimized for embedded constraints
- **Clear documentation** with hardware-specific considerations

### Testing & Debugging:
- **Unit tests** for critical components
- **Hardware-in-loop testing** with actual peripherals
- **Power consumption profiling** during development
- **Memory usage monitoring** with heap fragmentation analysis
- **Real-time performance analysis** with task timing

Always prioritize power efficiency, respect E-paper display limitations, and ensure proper multi-peripheral coordination in all code suggestions and implementations.
  - "SensorLib" # BHI260AP, CST328
  - "XPowersLib" # BQ25896, BQ27220
  - "TinyGPSPlus" # MIA-M10Q GPS# T-Deck-Pro ESP32-S3 Firmware Development Environment

You are helping develop firmware for the T-Deck-Pro, an ESP32-S3 portable device with:

## Hardware Specifications

### Core Components
- **MCU**: ESP32-S3 (240MHz, 16MB Flash, 8MB PSRAM)
- **Display**: 320x240 E-paper (GDEQ031T10) with slow refresh limitations
- **Communication**: LoRa SX1262, GPS MIA-M10Q, A7682E 4G modem, WiFi 802.11 b/g/n
- **Input**: TCA8418 keyboard matrix, CST328 touch controller
- **Sensors**: BHI260AP gyroscope, LTR-553ALS light sensor, BQ27220 battery gauge, BQ25896 power management
- **Storage**: MicroSD card slot, 18650 battery with USB-C charging

### Pin Configuration
- **I2C**: SDA=13, SCL=14 (shared by multiple devices)
- **E-paper**: CS=34, DC=35, BUSY=37, RST=-1 (no reset pin)
- **SPI**: Shared bus for E-paper, LoRa, SD card
- **4G Modem**: RXD=10, TXD=11, PWRKEY=40, RST=9
- **LoRa**: CS=3, RST=4, DIO1=5, BUSY=6

## Critical Development Constraints

### Power Management
- **Battery-powered device** requiring aggressive power management
- **Battery thresholds**: Low=3.3V, Critical=3.0V
- **Sleep modes**: Light sleep, deep sleep, hibernate
- **CPU frequency scaling**: 240MHz → 80MHz → sleep

### Display Limitations
- **Very slow refresh rate** (1-2 seconds per update)
- **Partial update optimization** required for responsive UI
- **1-bit color depth** (black/white only)
- **5-second LVGL refresh period** for E-paper compatibility

### Memory Management
- **Limited RAM** - must use PSRAM for large buffers
- **Dynamic app loading** from SD card to RAM
- **LRU app eviction** when memory pressure detected
- **Flash storage constraints** (~4MB available for apps)

### Real-time Communication
- **Multi-peripheral coordination** (WiFi, LoRa, Cellular, GPS)
- **MQTT-based server integration** with mesh networking
- **OTA updates** for firmware and applications
- **Hardware pin conflicts** (audio disabled on 4G version)

## Code Style & Architecture

### FreeRTOS Task Structure
```cpp
// Task priorities (higher = more important)
#define UI_TASK_PRIORITY           (configMAX_PRIORITIES - 1)
#define DISPLAY_TASK_PRIORITY      (configMAX_PRIORITIES - 2)
#define INPUT_TASK_PRIORITY        (configMAX_PRIORITIES - 3)
#define LORA_TASK_PRIORITY         (configMAX_PRIORITIES - 4)
#define WIFI_TASK_PRIORITY         (configMAX_PRIORITIES - 5)
#define CELLULAR_TASK_PRIORITY     (configMAX_PRIORITIES - 6)
#define SYSTEM_TASK_PRIORITY       (configMAX_PRIORITIES - 7)
```

### Error Handling & Timeouts
- **Comprehensive error recovery** with logging
- **Proper timeout handling** for all I/O operations
- **Watchdog timer integration** for system stability
- **Graceful degradation** when peripherals fail

### Memory Optimization
- **PSRAM utilization** for large buffers and app data
- **Stack size optimization** (2KB-8KB per task)
- **Heap management** with fragmentation prevention
- **Smart caching** of frequently used components

### Arduino/PlatformIO Conventions
- **PlatformIO build system** with ESP32-S3 toolchain
- **Arduino framework** with FreeRTOS integration
- **Library dependency management** via platformio.ini
- **Multiple build configurations** (debug, release, OTA)

## Key Libraries & Dependencies

### Core Libraries
- **GxEPD2**: E-paper display driver for GDEQ031T10
- **RadioLib**: LoRa SX1262 communication
- **TinyGSM**: A7682E 4G modem support
- **TinyGPSPlus**: MIA-M10Q GPS module
- **LVGL**: UI framework optimized for E-paper
- **PubSubClient**: MQTT communication

### Hardware Libraries
- **Adafruit TCA8418**: Keyboard matrix controller
- **XPowersLib**: BQ25896/BQ27220 power management
- **SensorLib**: BHI260AP gyroscope, CST328 touch
- **ArduinoJson**: Configuration and data serialization

### Development Tools
- **ESP32 Arduino Core**: Base framework
- **FreeRTOS**: Real-time operating system
- **SPIFFS/LittleFS**: File system management
- **SD**: SD card interface

## Development Guidelines

### Always Consider:
1. **Power consumption** - every operation affects battery life
2. **E-paper limitations** - slow refresh, partial update optimization
3. **Memory constraints** - use PSRAM, implement LRU eviction
4. **Real-time requirements** - proper task prioritization
5. **Hardware pin conflicts** - verify pin assignments
6. **Multi-peripheral coordination** - avoid resource conflicts

### Code Quality Standards:
- **Comprehensive error handling** with meaningful error messages
- **Proper resource cleanup** in destructors and error paths
- **Memory leak prevention** with smart pointers and RAII
- **Thread-safe operations** with mutexes and semaphores
- **Efficient algorithms** optimized for embedded constraints
- **Clear documentation** with hardware-specific considerations

### Testing & Debugging:
- **Unit tests** for critical components
- **Hardware-in-loop testing** with actual peripherals
- **Power consumption profiling** during development
- **Memory usage monitoring** with heap fragmentation analysis
- **Real-time performance analysis** with task timing

Always prioritize power efficiency, respect E-paper display limitations, and ensure proper multi-peripheral coordination in all code suggestions and implementations.
  - "SensorLib" # BHI260AP, CST328
  - "XPowersLib" # BQ25896, BQ27220
  - "TinyGPSPlus" # MIA-M10Q GPS