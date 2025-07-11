/**
 * @file      hardware_manager.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Hardware Abstraction Layer Implementation
 */

#include "hardware_manager.h"
#include "../core/logger.h"
#include <SPIFFS.h>
#include <SD.h>
#include <algorithm>

// Global hardware manager instance
HardwareManager& Hardware = HardwareManager::getInstance();

HardwareManager& HardwareManager::getInstance() {
    static HardwareManager instance;
    return instance;
}

bool HardwareManager::init() {
    if (initialized) {
        return true;
    }

    logInfo("HardwareManager", "Starting hardware initialization...");

    // Initialize component status array
    for (int i = 0; i < 13; i++) {
        component_status[i] = HardwareStatus::NOT_INITIALIZED;
    }

    // Initialize I2C bus
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Wire.setClock(400000); // 400kHz

    // Initialize SPI bus
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

    // Initialize Serial interfaces
    SerialMon.begin(115200);
    SerialAT.begin(115200, SERIAL_8N1, BOARD_A7682E_RXD, BOARD_A7682E_TXD);
    SerialGPS.begin(9600, SERIAL_8N1, BOARD_GPS_RXD, BOARD_GPS_TXD);

    // Initialize power management first
    if (!initPowerManagement()) {
        logError("HardwareManager", "Power management initialization failed");
        return false;
    }

    // Initialize core components
    bool success = true;
    success &= initDisplay();
    success &= initTouch();
    success &= initKeyboard();
    success &= initSDCard();
    success &= initBattery();
    success &= initGPS();
    success &= initGyroscope();
    success &= initLightSensor();
    success &= initWiFi();
    success &= initLoRa();
    success &= init4G();
    success &= initAudio();

    if (success) {
        initialized = true;
        logInfo("HardwareManager", "Hardware initialization completed successfully");
    } else {
        logError("HardwareManager", "Hardware initialization failed");
    }

    return success;
}

void HardwareManager::shutdown() {
    if (!initialized) {
        return;
    }

    logInfo("HardwareManager", "Shutting down hardware...");

    // Shutdown components in reverse order
    setDisplayPower(false);
    setComponentPower(HardwareComponent::LORA, false);
    setComponentPower(HardwareComponent::CELLULAR_4G, false);
    setComponentPower(HardwareComponent::GPS, false);
    setComponentPower(HardwareComponent::GYROSCOPE, false);

    // Clean up dynamic allocations
    if (display_driver) {
        delete display_driver;
        display_driver = nullptr;
    }
    if (touch_driver) {
        delete touch_driver;
        touch_driver = nullptr;
    }
    if (lora_radio) {
        delete lora_radio;
        lora_radio = nullptr;
    }
    if (gps_parser) {
        delete gps_parser;
        gps_parser = nullptr;
    }

    initialized = false;
    logInfo("HardwareManager", "Hardware shutdown completed");
}

void HardwareManager::update() {
    if (!initialized) {
        return;
    }

    uint32_t now = millis();
    if (now - last_update < 100) { // Update every 100ms
        return;
    }
    last_update = now;

    // Update component status
    updateComponentStatus();

    // Update GPS data
    while (SerialGPS.available() > 0) {
        if (gps_parser && gps_parser->encode(SerialGPS.read())) {
            // GPS data updated
        }
    }
}

// === DISPLAY MANAGEMENT ===

bool HardwareManager::initDisplay() {
    component_status[(int)HardwareComponent::EPD_DISPLAY] = HardwareStatus::INITIALIZING;
    
    try {
        // Create display driver instance
        display_driver = new GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>(
            GxEPD2_310_GDEQ031T10(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY)
        );

        if (!display_driver) {
            logError("Display", "Failed to create display driver");
            component_status[(int)HardwareComponent::EPD_DISPLAY] = HardwareStatus::ERROR;
            return false;
        }

        display_driver->init(115200);
        display_driver->setRotation(0);
        display_driver->setFullWindow();
        display_driver->firstPage();

        do {
            display_driver->fillScreen(GxEPD_WHITE);
        } while (display_driver->nextPage());

        component_status[(int)HardwareComponent::EPD_DISPLAY] = HardwareStatus::READY;
        logInfo("Display", "E-paper display initialized successfully");
        return true;
    } catch (...) {
        logError("Display", "Exception during display initialization");
        component_status[(int)HardwareComponent::EPD_DISPLAY] = HardwareStatus::ERROR;
        return false;
    }
}

bool HardwareManager::updateDisplay(const char* text, int16_t x, int16_t y) {
    if (!display_driver || component_status[(int)HardwareComponent::EPD_DISPLAY] != HardwareStatus::READY) {
        return false;
    }

    display_driver->setPartialWindow(x, y, display_driver->width() - x, display_driver->height() - y);
    display_driver->firstPage();
    
    do {
        display_driver->fillScreen(GxEPD_WHITE);
        display_driver->setCursor(x, y);
        display_driver->print(text);
    } while (display_driver->nextPage());

    return true;
}

bool HardwareManager::refreshDisplay(DisplayRefreshMode mode) {
    if (!display_driver || component_status[(int)HardwareComponent::EPD_DISPLAY] != HardwareStatus::READY) {
        return false;
    }

    switch (mode) {
        case DisplayRefreshMode::FULL:
            display_driver->refresh();
            break;
        case DisplayRefreshMode::PARTIAL:
            display_driver->refresh(true);
            break;
        case DisplayRefreshMode::FAST:
            // Fast refresh implementation
            display_driver->refresh(true);
            break;
    }

    return true;
}

void HardwareManager::clearDisplay() {
    if (!display_driver || component_status[(int)HardwareComponent::EPD_DISPLAY] != HardwareStatus::READY) {
        return;
    }

    display_driver->setFullWindow();
    display_driver->firstPage();
    
    do {
        display_driver->fillScreen(GxEPD_WHITE);
    } while (display_driver->nextPage());
}

void HardwareManager::setDisplayPower(bool enabled) {
    if (!display_driver) {
        return;
    }

    if (enabled) {
        // Power on display
        display_driver->init();
        component_status[(int)HardwareComponent::EPD_DISPLAY] = HardwareStatus::READY;
    } else {
        // Power off display
        display_driver->hibernate();
        component_status[(int)HardwareComponent::EPD_DISPLAY] = HardwareStatus::POWER_OFF;
    }
}

// === INPUT MANAGEMENT ===

char HardwareManager::getKeyInput() {
    // Implementation will be added when keyboard driver is integrated
    return 0;
}

TouchEvent HardwareManager::getTouchEvent() {
    TouchEvent event;
    
    if (!touch_driver || component_status[(int)HardwareComponent::TOUCH] != HardwareStatus::READY) {
        return event;
    }

    if (touch_driver->isPressed()) {
        auto point = touch_driver->getPoint();
        event.x = point.x;
        event.y = point.y;
        event.pressed = true;
    }

    return event;
}

bool HardwareManager::isTouchPressed() {
    if (!touch_driver || component_status[(int)HardwareComponent::TOUCH] != HardwareStatus::READY) {
        return false;
    }

    return touch_driver->isPressed();
}

// === SENSOR ACCESS ===

BatteryInfo HardwareManager::getBatteryInfo() {
    BatteryInfo info;
    
    if (!battery_monitor || component_status[(int)HardwareComponent::BATTERY] != HardwareStatus::READY) {
        return info;
    }

    // Implementation will be added when battery monitor is integrated
    info.voltage = 3.7f; // Placeholder
    info.percentage = 85; // Placeholder
    info.present = true;

    return info;
}

GPSData HardwareManager::getGPSData() {
    GPSData data;
    
    if (!gps_parser || component_status[(int)HardwareComponent::GPS] != HardwareStatus::READY) {
        return data;
    }

    if (gps_parser->location.isValid()) {
        data.latitude = gps_parser->location.lat();
        data.longitude = gps_parser->location.lng();
        data.valid = true;
    }

    if (gps_parser->altitude.isValid()) {
        data.altitude = gps_parser->altitude.meters();
    }

    if (gps_parser->speed.isValid()) {
        data.speed = gps_parser->speed.kmph();
    }

    if (gps_parser->satellites.isValid()) {
        data.satellites = gps_parser->satellites.value();
    }

    if (gps_parser->date.isValid() && gps_parser->time.isValid()) {
        data.year = gps_parser->date.year();
        data.month = gps_parser->date.month();
        data.day = gps_parser->date.day();
        data.hour = gps_parser->time.hour();
        data.minute = gps_parser->time.minute();
        data.second = gps_parser->time.second();
    }

    return data;
}

SensorData HardwareManager::getSensorData() {
    SensorData data;
    
    // Implementation will be added when sensor drivers are integrated
    return data;
}

// === PRIVATE METHODS ===

bool HardwareManager::initPowerManagement() {
    component_status[(int)HardwareComponent::POWER_MANAGEMENT] = HardwareStatus::INITIALIZING;
    
    // Enable power for various components
    pinMode(BOARD_GPS_EN, OUTPUT);
    pinMode(BOARD_1V8_EN, OUTPUT);
    pinMode(BOARD_6609_EN, OUTPUT);
    pinMode(BOARD_LORA_EN, OUTPUT);

    // Enable 1.8V rail for gyroscope
    digitalWrite(BOARD_1V8_EN, HIGH);
    delay(100);

    component_status[(int)HardwareComponent::POWER_MANAGEMENT] = HardwareStatus::READY;
    logInfo("PowerMgmt", "Power management initialized");
    return true;
}

bool HardwareManager::initTouch() {
    component_status[(int)HardwareComponent::TOUCH] = HardwareStatus::INITIALIZING;
    
    try {
        touch_driver = new TouchDrvCSTXXX();
        touch_driver->setPins(BOARD_TOUCH_RST, BOARD_TOUCH_INT);
        
        if (touch_driver->begin(Wire, BOARD_I2C_ADDR_TOUCH, BOARD_TOUCH_SDA, BOARD_TOUCH_SCL)) {
            component_status[(int)HardwareComponent::TOUCH] = HardwareStatus::READY;
            logInfo("Touch", "Touch controller initialized successfully");
            return true;
        } else {
            logError("Touch", "Failed to initialize touch controller");
            component_status[(int)HardwareComponent::TOUCH] = HardwareStatus::ERROR;
            return false;
        }
    } catch (...) {
        logError("Touch", "Exception during touch initialization");
        component_status[(int)HardwareComponent::TOUCH] = HardwareStatus::ERROR;
        return false;
    }
}

void HardwareManager::setLogger(Logger* logger_instance) {
    logger = logger_instance;
}

void HardwareManager::logError(const char* component, const char* message) {
    if (logger) {
        logger->error(component, "%s", message);
    } else {
        SerialMon.printf("[ERROR] %s: %s\n", component, message);
    }
}

void HardwareManager::logInfo(const char* component, const char* message) {
    if (logger) {
        logger->info(component, "%s", message);
    } else {
        SerialMon.printf("[INFO] %s: %s\n", component, message);
    }
}

void HardwareManager::logErrorf(const char* component, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    logError(component, buffer);
}

void HardwareManager::logInfof(const char* component, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    logInfo(component, buffer);
}

bool HardwareManager::initKeyboard() {
    component_status[(int)HardwareComponent::KEYBOARD] = HardwareStatus::INITIALIZING;

    // Initialize keyboard interrupt pin
    pinMode(BOARD_KEYBOARD_INT, INPUT_PULLUP);
    pinMode(BOARD_KEYBOARD_LED, OUTPUT);
    digitalWrite(BOARD_KEYBOARD_LED, LOW);

    // Check if keyboard is present on I2C bus
    Wire.beginTransmission(BOARD_I2C_ADDR_KEYBOARD);
    if (Wire.endTransmission() == 0) {
        component_status[(int)HardwareComponent::KEYBOARD] = HardwareStatus::READY;
        logInfo("Keyboard", "Keyboard controller initialized successfully");
        return true;
    } else {
        logError("Keyboard", "Keyboard controller not found");
        component_status[(int)HardwareComponent::KEYBOARD] = HardwareStatus::ERROR;
        return false;
    }
}

bool HardwareManager::initGPS() {
    component_status[(int)HardwareComponent::GPS] = HardwareStatus::INITIALIZING;

    // Enable GPS power
    digitalWrite(BOARD_GPS_EN, HIGH);
    delay(100);

    try {
        gps_parser = new TinyGPSPlus();

        if (gps_parser) {
            component_status[(int)HardwareComponent::GPS] = HardwareStatus::READY;
            logInfo("GPS", "GPS module initialized successfully");
            return true;
        } else {
            logError("GPS", "Failed to create GPS parser");
            component_status[(int)HardwareComponent::GPS] = HardwareStatus::ERROR;
            return false;
        }
    } catch (...) {
        logError("GPS", "Exception during GPS initialization");
        component_status[(int)HardwareComponent::GPS] = HardwareStatus::ERROR;
        return false;
    }
}

bool HardwareManager::initBattery() {
    component_status[(int)HardwareComponent::BATTERY] = HardwareStatus::INITIALIZING;

    // Check if battery monitor is present on I2C bus
    Wire.beginTransmission(BOARD_I2C_ADDR_BQ27220);
    if (Wire.endTransmission() == 0) {
        // BQ27220 battery monitor found
        component_status[(int)HardwareComponent::BATTERY] = HardwareStatus::READY;
        logInfo("Battery", "Battery monitor initialized successfully");
        return true;
    } else {
        logError("Battery", "Battery monitor not found");
        component_status[(int)HardwareComponent::BATTERY] = HardwareStatus::ERROR;
        return false;
    }
}

bool HardwareManager::initGyroscope() {
    component_status[(int)HardwareComponent::GYROSCOPE] = HardwareStatus::INITIALIZING;

    // Check if gyroscope is present on I2C bus
    Wire.beginTransmission(BOARD_I2C_ADDR_GYROSCOPDE);
    if (Wire.endTransmission() == 0) {
        component_status[(int)HardwareComponent::GYROSCOPE] = HardwareStatus::READY;
        logInfo("Gyroscope", "Gyroscope initialized successfully");
        return true;
    } else {
        logError("Gyroscope", "Gyroscope not found");
        component_status[(int)HardwareComponent::GYROSCOPE] = HardwareStatus::ERROR;
        return false;
    }
}

bool HardwareManager::initLightSensor() {
    component_status[(int)HardwareComponent::LIGHT_SENSOR] = HardwareStatus::INITIALIZING;

    // Initialize light sensor interrupt pin
    pinMode(BOARD_ALS_INT, INPUT_PULLUP);

    // Check if light sensor is present on I2C bus
    Wire.beginTransmission(BOARD_I2C_ADDR_LTR_553ALS);
    if (Wire.endTransmission() == 0) {
        component_status[(int)HardwareComponent::LIGHT_SENSOR] = HardwareStatus::READY;
        logInfo("LightSensor", "Light sensor initialized successfully");
        return true;
    } else {
        logError("LightSensor", "Light sensor not found");
        component_status[(int)HardwareComponent::LIGHT_SENSOR] = HardwareStatus::ERROR;
        return false;
    }
}

bool HardwareManager::initAudio() {
    component_status[(int)HardwareComponent::AUDIO] = HardwareStatus::INITIALIZING;

    // Initialize I2S pins for audio
    pinMode(BOARD_I2S_BCLK, OUTPUT);
    pinMode(BOARD_I2S_DOUT, OUTPUT);
    pinMode(BOARD_I2S_LRC, OUTPUT);

    component_status[(int)HardwareComponent::AUDIO] = HardwareStatus::READY;
    logInfo("Audio", "Audio system initialized successfully");
    return true;
}

// === COMMUNICATION INTERFACES ===

bool HardwareManager::initWiFi() {
    component_status[(int)HardwareComponent::WIFI] = HardwareStatus::INITIALIZING;

    // Configure WiFi for optimal ESP32-S3 performance
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);  // Clear any stored credentials

    // Set country code for regulatory compliance (default to US)
    // WiFi.setCountry("US"); // Not available in this ESP32 version

    // Configure WiFi power management
    WiFi.setSleep(WIFI_PS_MIN_MODEM);  // Minimum power save mode

    // Set hostname
    WiFi.setHostname("T-Deck-Pro");

    // Enable auto-reconnect
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    delay(100);

    component_status[(int)HardwareComponent::WIFI] = HardwareStatus::READY;
    logInfo("WiFi", "WiFi module initialized successfully");
    return true;
}

bool HardwareManager::connectWiFi(const char* ssid, const char* password) {
    if (component_status[(int)HardwareComponent::WIFI] != HardwareStatus::READY) {
        logError("WiFi", "WiFi not initialized");
        return false;
    }

    if (!ssid || strlen(ssid) == 0) {
        logError("WiFi", "Invalid SSID provided");
        return false;
    }

    logInfof("WiFi", "Attempting to connect to WiFi network: %s", ssid);

    // Disconnect from any existing connection
    if (WiFi.isConnected()) {
        WiFi.disconnect();
        delay(100);
    }

    // Scan for the network first to verify it exists
    int networks = WiFi.scanNetworks();
    bool network_found = false;
    int target_rssi = -100;

    for (int i = 0; i < networks; i++) {
        if (WiFi.SSID(i) == String(ssid)) {
            network_found = true;
            target_rssi = WiFi.RSSI(i);
            logInfo("WiFi", "Target network found with RSSI: %d dBm", target_rssi);
            break;
        }
    }

    if (!network_found) {
        logError("WiFi", "Network '%s' not found in scan", ssid);
        return false;
    }

    // Attempt connection with exponential backoff retry
    const int max_attempts = 5;
    const int base_delay = 1000; // 1 second base delay

    for (int attempt = 1; attempt <= max_attempts; attempt++) {
        logInfo("WiFi", "Connection attempt %d/%d", attempt, max_attempts);

        // Begin connection
        WiFi.begin(ssid, password);

        // Wait for connection with timeout
        int timeout_ms = 15000; // 15 second timeout per attempt
        int elapsed_ms = 0;
        const int check_interval = 500;

        while (WiFi.status() != WL_CONNECTED && elapsed_ms < timeout_ms) {
            delay(check_interval);
            elapsed_ms += check_interval;

            // Log progress every 2 seconds
            if (elapsed_ms % 2000 == 0) {
                logInfo("WiFi", "Connecting... Status: %d", WiFi.status());
            }
        }

        if (WiFi.status() == WL_CONNECTED) {
            // Connection successful
            logInfo("WiFi", "WiFi connected successfully!");
            logInfo("WiFi", "IP Address: %s", WiFi.localIP().toString().c_str());
            logInfo("WiFi", "Gateway: %s", WiFi.gatewayIP().toString().c_str());
            logInfo("WiFi", "DNS: %s", WiFi.dnsIP().toString().c_str());
            logInfo("WiFi", "RSSI: %d dBm", WiFi.RSSI());
            logInfo("WiFi", "Channel: %d", WiFi.channel());

            return true;
        } else {
            // Connection failed
            String error_msg = getWiFiStatusString(WiFi.status());
            logError("WiFi", "Connection attempt %d failed: %s", attempt, error_msg.c_str());

            // Disconnect and wait before retry
            WiFi.disconnect();

            if (attempt < max_attempts) {
                int delay_ms = base_delay * (1 << (attempt - 1)); // Exponential backoff
                logInfo("WiFi", "Retrying in %d ms...", delay_ms);
                delay(delay_ms);
            }
        }
    }

    logError("WiFi", "Failed to connect to WiFi after %d attempts", max_attempts);
    return false;
}

bool HardwareManager::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int32_t HardwareManager::getWiFiRSSI() {
    if (isWiFiConnected()) {
        return WiFi.RSSI();
    }
    return -100; // No signal
}

String HardwareManager::getWiFiStatusString(wl_status_t status) {
    switch (status) {
        case WL_IDLE_STATUS:
            return "Idle";
        case WL_NO_SSID_AVAIL:
            return "No SSID Available";
        case WL_SCAN_COMPLETED:
            return "Scan Completed";
        case WL_CONNECTED:
            return "Connected";
        case WL_CONNECT_FAILED:
            return "Connection Failed";
        case WL_CONNECTION_LOST:
            return "Connection Lost";
        case WL_DISCONNECTED:
            return "Disconnected";
        default:
            return "Unknown Status (" + String(status) + ")";
    }
}

bool HardwareManager::reconnectWiFi() {
    if (component_status[(int)HardwareComponent::WIFI] != HardwareStatus::READY) {
        return false;
    }

    logInfo("WiFi", "Attempting to reconnect to WiFi...");

    // Try to reconnect using stored credentials
    if (WiFi.getAutoReconnect()) {
        WiFi.reconnect();

        // Wait for reconnection
        int timeout_ms = 10000; // 10 second timeout
        int elapsed_ms = 0;
        const int check_interval = 500;

        while (WiFi.status() != WL_CONNECTED && elapsed_ms < timeout_ms) {
            delay(check_interval);
            elapsed_ms += check_interval;
        }

        if (WiFi.status() == WL_CONNECTED) {
            logInfo("WiFi", "WiFi reconnected successfully");
            return true;
        }
    }

    logError("WiFi", "WiFi reconnection failed");
    return false;
}

void HardwareManager::disconnectWiFi() {
    if (WiFi.isConnected()) {
        logInfo("WiFi", "Disconnecting from WiFi...");
        WiFi.disconnect(true);
        delay(100);
        logInfo("WiFi", "WiFi disconnected");
    }
}

std::vector<WiFiNetwork> HardwareManager::scanWiFiNetworks() {
    std::vector<WiFiNetwork> networks;

    if (component_status[(int)HardwareComponent::WIFI] != HardwareStatus::READY) {
        return networks;
    }

    logInfo("WiFi", "Scanning for WiFi networks...");

    int network_count = WiFi.scanNetworks();

    if (network_count == 0) {
        logInfo("WiFi", "No networks found");
        return networks;
    }

    logInfo("WiFi", "Found %d networks", network_count);

    for (int i = 0; i < network_count; i++) {
        WiFiNetwork network;
        network.ssid = WiFi.SSID(i);
        network.rssi = WiFi.RSSI(i);
        network.channel = WiFi.channel(i);
        network.encryption_type = WiFi.encryptionType(i);
        network.is_hidden = (network.ssid.length() == 0);

        networks.push_back(network);

        logInfo("WiFi", "Network %d: %s (RSSI: %d, Ch: %d, Enc: %d)",
                i + 1, network.ssid.c_str(), network.rssi, network.channel, network.encryption_type);
    }

    // Sort networks by signal strength (strongest first)
    std::sort(networks.begin(), networks.end(), [](const WiFiNetwork& a, const WiFiNetwork& b) {
        return a.rssi > b.rssi;
    });

    return networks;
}

void HardwareManager::setWiFiCountry(const char* country_code) {
    if (country_code && strlen(country_code) == 2) {
        WiFi.setCountry(country_code);
        logInfo("WiFi", "WiFi country code set to: %s", country_code);
    } else {
        logError("WiFi", "Invalid country code: %s", country_code ? country_code : "NULL");
    }
}

void HardwareManager::setWiFiPowerSave(bool enable) {
    if (enable) {
        WiFi.setSleep(WIFI_PS_MAX_MODEM);
        logInfo("WiFi", "WiFi power save enabled");
    } else {
        WiFi.setSleep(WIFI_PS_NONE);
        logInfo("WiFi", "WiFi power save disabled");
    }
}

String HardwareManager::getWiFiInfo() {
    String info = "WiFi Information:\n";

    if (isWiFiConnected()) {
        info += "Status: Connected\n";
        info += "SSID: " + WiFi.SSID() + "\n";
        info += "IP Address: " + WiFi.localIP().toString() + "\n";
        info += "Gateway: " + WiFi.gatewayIP().toString() + "\n";
        info += "DNS: " + WiFi.dnsIP().toString() + "\n";
        info += "Subnet Mask: " + WiFi.subnetMask().toString() + "\n";
        info += "MAC Address: " + WiFi.macAddress() + "\n";
        info += "RSSI: " + String(WiFi.RSSI()) + " dBm\n";
        info += "Channel: " + String(WiFi.channel()) + "\n";
        info += "TX Power: " + String(WiFi.getTxPower()) + " dBm\n";
    } else {
        info += "Status: " + getWiFiStatusString(WiFi.status()) + "\n";
        info += "MAC Address: " + WiFi.macAddress() + "\n";
    }

    return info;
}

bool HardwareManager::initLoRa() {
    component_status[(int)HardwareComponent::LORA] = HardwareStatus::INITIALIZING;

    // Enable LoRa power
    digitalWrite(BOARD_LORA_EN, HIGH);
    delay(100);

    try {
        lora_radio = new SX1262(new Module(BOARD_LORA_CS, BOARD_LORA_INT, BOARD_LORA_RST, BOARD_LORA_BUSY));

        if (lora_radio) {
            // Initialize LoRa with default settings
            int state = lora_radio->begin(868.0, 125.0, 9, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 10, 8, 0);

            if (state == RADIOLIB_ERR_NONE) {
                component_status[(int)HardwareComponent::LORA] = HardwareStatus::READY;
                logInfo("LoRa", "LoRa radio initialized successfully");
                return true;
            } else {
                logError("LoRa", "LoRa radio initialization failed");
                component_status[(int)HardwareComponent::LORA] = HardwareStatus::ERROR;
                return false;
            }
        } else {
            logError("LoRa", "Failed to create LoRa radio instance");
            component_status[(int)HardwareComponent::LORA] = HardwareStatus::ERROR;
            return false;
        }
    } catch (...) {
        logError("LoRa", "Exception during LoRa initialization");
        component_status[(int)HardwareComponent::LORA] = HardwareStatus::ERROR;
        return false;
    }
}

bool HardwareManager::sendLoRaMessage(const uint8_t* message, size_t length) {
    if (!lora_radio || component_status[(int)HardwareComponent::LORA] != HardwareStatus::READY) {
        return false;
    }

    int state = lora_radio->transmit(message, length);
    return (state == RADIOLIB_ERR_NONE);
}

size_t HardwareManager::receiveLoRaMessage(uint8_t* buffer, size_t max_length) {
    if (!lora_radio || component_status[(int)HardwareComponent::LORA] != HardwareStatus::READY) {
        return 0;
    }

    int state = lora_radio->receive(buffer, max_length);
    if (state == RADIOLIB_ERR_NONE) {
        return lora_radio->getPacketLength();
    }
    return 0;
}

bool HardwareManager::init4G() {
    component_status[(int)HardwareComponent::CELLULAR_4G] = HardwareStatus::INITIALIZING;

    // Enable 4G modem power
    digitalWrite(BOARD_6609_EN, HIGH);
    delay(100);

    // Initialize power key pin
    pinMode(BOARD_A7682E_PWRKEY, OUTPUT);
    digitalWrite(BOARD_A7682E_PWRKEY, HIGH);
    delay(1000);
    digitalWrite(BOARD_A7682E_PWRKEY, LOW);

    // Wait for modem to start
    delay(3000);

    // Test AT communication
    SerialAT.println("AT");
    delay(100);

    if (SerialAT.available()) {
        String response = SerialAT.readString();
        if (response.indexOf("OK") >= 0) {
            component_status[(int)HardwareComponent::CELLULAR_4G] = HardwareStatus::READY;
            logInfo("4G", "4G modem initialized successfully");
            return true;
        }
    }

    logError("4G", "4G modem initialization failed");
    component_status[(int)HardwareComponent::CELLULAR_4G] = HardwareStatus::ERROR;
    return false;
}

bool HardwareManager::is4GConnected() {
    if (component_status[(int)HardwareComponent::CELLULAR_4G] != HardwareStatus::READY) {
        return false;
    }

    // Check network registration status
    SerialAT.println("AT+CREG?");
    delay(100);

    if (SerialAT.available()) {
        String response = SerialAT.readString();
        return (response.indexOf("+CREG: 0,1") >= 0 || response.indexOf("+CREG: 0,5") >= 0);
    }

    return false;
}

// === STORAGE ===

bool HardwareManager::initSDCard() {
    component_status[(int)HardwareComponent::SD_CARD] = HardwareStatus::INITIALIZING;

    if (SD.begin(BOARD_SD_CS)) {
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        component_status[(int)HardwareComponent::SD_CARD] = HardwareStatus::READY;
        logInfo("SDCard", "SD card initialized successfully");
        SerialMon.printf("[INFO] SD Card Size: %lluMB\n", cardSize);
        return true;
    } else {
        logError("SDCard", "SD card initialization failed");
        component_status[(int)HardwareComponent::SD_CARD] = HardwareStatus::ERROR;
        return false;
    }
}

bool HardwareManager::isSDCardPresent() {
    return component_status[(int)HardwareComponent::SD_CARD] == HardwareStatus::READY;
}

uint64_t HardwareManager::getSDCardSize() {
    if (isSDCardPresent()) {
        return SD.cardSize() / (1024 * 1024); // Return size in MB
    }
    return 0;
}

// === POWER MANAGEMENT ===

void HardwareManager::setComponentPower(HardwareComponent component, bool enabled) {
    switch (component) {
        case HardwareComponent::GPS:
            digitalWrite(BOARD_GPS_EN, enabled ? HIGH : LOW);
            break;
        case HardwareComponent::LORA:
            digitalWrite(BOARD_LORA_EN, enabled ? HIGH : LOW);
            break;
        case HardwareComponent::CELLULAR_4G:
            digitalWrite(BOARD_6609_EN, enabled ? HIGH : LOW);
            break;
        case HardwareComponent::GYROSCOPE:
            digitalWrite(BOARD_1V8_EN, enabled ? HIGH : LOW);
            break;
        default:
            break;
    }

    if (enabled) {
        delay(100); // Allow component to power up
    }
}

void HardwareManager::enterLowPowerMode(uint32_t duration_ms) {
    logInfo("PowerMgmt", "Entering low power mode");

    // Disable non-essential components
    setDisplayPower(false);
    setComponentPower(HardwareComponent::LORA, false);
    setComponentPower(HardwareComponent::GPS, false);

    if (duration_ms > 0) {
        esp_sleep_enable_timer_wakeup(duration_ms * 1000); // Convert to microseconds
    }

    esp_light_sleep_start();
}

void HardwareManager::wakeUp() {
    logInfo("PowerMgmt", "Waking up from low power mode");

    // Re-enable components
    setDisplayPower(true);
    setComponentPower(HardwareComponent::LORA, true);
    setComponentPower(HardwareComponent::GPS, true);
}

// === STATUS AND DIAGNOSTICS ===

HardwareStatus HardwareManager::getComponentStatus(HardwareComponent component) {
    return component_status[(int)component];
}

bool HardwareManager::runDiagnostics() {
    logInfo("Diagnostics", "Running hardware diagnostics...");

    bool all_passed = true;

    // Check each component
    const char* component_names[] = {
        "Display", "Touch", "Keyboard", "LoRa", "GPS", "4G", "WiFi",
        "SD Card", "Battery", "Gyroscope", "Light Sensor", "Audio", "Power Mgmt"
    };

    for (int i = 0; i < 13; i++) {
        HardwareStatus status = component_status[i];
        const char* status_str = "UNKNOWN";

        switch (status) {
            case HardwareStatus::READY:
                status_str = "READY";
                break;
            case HardwareStatus::ERROR:
                status_str = "ERROR";
                all_passed = false;
                break;
            case HardwareStatus::POWER_OFF:
                status_str = "POWER_OFF";
                break;
            case HardwareStatus::NOT_INITIALIZED:
                status_str = "NOT_INIT";
                all_passed = false;
                break;
            case HardwareStatus::INITIALIZING:
                status_str = "INIT";
                break;
        }

        SerialMon.printf("[DIAG] %s: %s\n", component_names[i], status_str);
    }

    if (all_passed) {
        logInfo("Diagnostics", "All hardware diagnostics passed");
    } else {
        logError("Diagnostics", "Some hardware diagnostics failed");
    }

    return all_passed;
}

String HardwareManager::getHardwareInfo() {
    String info = "T-Deck-Pro Hardware Information:\n";
    info += "Software Version: " + String(UI_T_DECK_PRO_VERSION) + "\n";
    info += "Hardware Version: " + String(BOARD_T_DECK_PRO_VERSION) + "\n";
    info += "ESP32-S3 Chip: " + String(ESP.getChipModel()) + "\n";
    info += "Flash Size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + "MB\n";
    info += "PSRAM Size: " + String(ESP.getPsramSize() / 1024 / 1024) + "MB\n";
    info += "Free Heap: " + String(ESP.getFreeHeap() / 1024) + "KB\n";

    if (isSDCardPresent()) {
        info += "SD Card Size: " + String(getSDCardSize()) + "MB\n";
    }

    if (isWiFiConnected()) {
        info += "WiFi RSSI: " + String(getWiFiRSSI()) + "dBm\n";
        info += "WiFi IP: " + WiFi.localIP().toString() + "\n";
    }

    return info;
}

void HardwareManager::updateComponentStatus() {
    // Update WiFi status
    if (component_status[(int)HardwareComponent::WIFI] == HardwareStatus::READY) {
        // WiFi status is managed by connection state
    }

    // Update 4G status
    if (component_status[(int)HardwareComponent::CELLULAR_4G] == HardwareStatus::READY) {
        // 4G status could be updated based on network registration
    }

    // Update battery status
    if (component_status[(int)HardwareComponent::BATTERY] == HardwareStatus::READY) {
        // Battery status monitoring could be added here
    }

    // Other component status updates can be added as needed
}
