/**
 * @file      simple_hardware.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Simplified Arduino-compatible hardware manager implementation
 */

#include "simple_hardware.h"
#include "simple_logger.h"

// Static instance
SimpleHardware* SimpleHardware::instance = nullptr;
SimpleHardware* Hardware = nullptr;

SimpleHardware::SimpleHardware() {
    display = nullptr;
    touch = nullptr;
    
    display_status = HW_NOT_INITIALIZED;
    touch_status = HW_NOT_INITIALIZED;
    wifi_status = HW_NOT_INITIALIZED;
    sd_status = HW_NOT_INITIALIZED;
    
    last_touch = {0, 0, false, 0};
    last_status_update = 0;
}

SimpleHardware* SimpleHardware::getInstance() {
    if (instance == nullptr) {
        instance = new SimpleHardware();
        Hardware = instance; // Set global pointer
    }
    return instance;
}

bool SimpleHardware::init() {
    LOG_INFO("Hardware", "Starting hardware initialization...");
    
    // Initialize I2C and SPI
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Wire.setClock(400000);
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
    
    bool success = true;
    
    // Initialize components in order
    success &= initPowerManagement();
    success &= initDisplay();
    success &= initTouch();
    success &= initWiFi();
    success &= initSD();
    
    if (success) {
        LOG_INFO("Hardware", "Hardware initialization completed successfully");
    } else {
        LOG_ERROR("Hardware", "Hardware initialization failed");
    }
    
    return success;
}

bool SimpleHardware::initPowerManagement() {
    LOG_INFO("Power", "Initializing power management...");
    
    // Enable power for various components
    pinMode(BOARD_GPS_EN, OUTPUT);
    pinMode(BOARD_1V8_EN, OUTPUT);
    pinMode(BOARD_6609_EN, OUTPUT);
    pinMode(BOARD_LORA_EN, OUTPUT);

    // Enable 1.8V rail for gyroscope
    digitalWrite(BOARD_1V8_EN, HIGH);
    delay(100);
    
    LOG_INFO("Power", "Power management initialized");
    return true;
}

bool SimpleHardware::initDisplay() {
    LOG_INFO("Display", "Initializing e-paper display...");
    display_status = HW_INITIALIZING;
    
    try {
        display = new GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>(
            GxEPD2_310_GDEQ031T10(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY)
        );
        
        if (!display) {
            LOG_ERROR("Display", "Failed to create display driver");
            display_status = HW_ERROR;
            return false;
        }
        
        display->init(115200);
        display->setRotation(0);
        display->setFullWindow();
        display->firstPage();
        
        do {
            display->fillScreen(GxEPD_WHITE);
            display->setCursor(10, 30);
            display->setTextColor(GxEPD_BLACK);
            display->println("T-Deck-Pro OS");
            display->println("Phase 1 Active");
            display->println("");
            display->println("Initializing...");
        } while (display->nextPage());
        
        display_status = HW_READY;
        LOG_INFO("Display", "E-paper display initialized successfully");
        return true;
    } catch (...) {
        LOG_ERROR("Display", "Exception during display initialization");
        display_status = HW_ERROR;
        return false;
    }
}

bool SimpleHardware::initTouch() {
    LOG_INFO("Touch", "Initializing touch controller...");
    touch_status = HW_INITIALIZING;
    
    touch = new TouchDrvCSTXXX();
    if (!touch) {
        LOG_ERROR("Touch", "Failed to create touch driver");
        touch_status = HW_ERROR;
        return false;
    }
    
    touch->setPins(BOARD_TOUCH_RST, BOARD_TOUCH_INT);
    
    if (touch->begin(Wire, BOARD_I2C_ADDR_TOUCH, BOARD_TOUCH_SDA, BOARD_TOUCH_SCL)) {
        touch_status = HW_READY;
        LOG_INFO("Touch", "Touch controller initialized successfully");
        return true;
    } else {
        LOG_ERROR("Touch", "Touch controller initialization failed");
        touch_status = HW_ERROR;
        return false;
    }
}

bool SimpleHardware::initWiFi() {
    LOG_INFO("WiFi", "Initializing WiFi...");
    wifi_status = HW_INITIALIZING;
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    wifi_status = HW_READY;
    LOG_INFO("WiFi", "WiFi initialized successfully");
    return true;
}

bool SimpleHardware::initSD() {
    LOG_INFO("SD", "Initializing SD card...");
    sd_status = HW_INITIALIZING;
    
    if (SD.begin(BOARD_SD_CS)) {
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        LOG_INFOF("SD", "SD card initialized successfully - Size: %lluMB", cardSize);
        sd_status = HW_READY;
        return true;
    } else {
        LOG_ERROR("SD", "SD card initialization failed");
        sd_status = HW_ERROR;
        return false;
    }
}

void SimpleHardware::update() {
    // Update touch state
    if (touch && touch_status == HW_READY) {
        if (touch->isPressed()) {
            int16_t x, y;
            if (touch->getPoint(&x, &y, 1)) {
                last_touch.x = x;
                last_touch.y = y;
                last_touch.pressed = true;
                last_touch.timestamp = millis();
            }
        } else {
            last_touch.pressed = false;
        }
    }
}

bool SimpleHardware::updateDisplay(const char* text, int16_t x, int16_t y) {
    if (!display || display_status != HW_READY) {
        return false;
    }
    
    display->setPartialWindow(x, y, display->width() - x, display->height() - y);
    display->firstPage();
    
    do {
        display->fillScreen(GxEPD_WHITE);
        display->setCursor(x, y);
        display->setTextColor(GxEPD_BLACK);
        display->print(text);
    } while (display->nextPage());
    
    return true;
}

bool SimpleHardware::refreshDisplay(bool full_refresh) {
    if (!display || display_status != HW_READY) {
        return false;
    }
    
    if (full_refresh) {
        display->setFullWindow();
    }
    
    display->display();
    return true;
}

TouchPoint SimpleHardware::getTouchInput() {
    return last_touch;
}

bool SimpleHardware::isTouchPressed() {
    return last_touch.pressed;
}

bool SimpleHardware::connectWiFi(const char* ssid, const char* password) {
    if (wifi_status != HW_READY) {
        return false;
    }
    
    LOG_INFOF("WiFi", "Attempting to connect to: %s", ssid);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    const int max_attempts = 20;
    
    while (WiFi.status() != WL_CONNECTED && attempts < max_attempts) {
        delay(500);
        attempts++;
        LOG_INFOF("WiFi", "Connection attempt %d/%d", attempts, max_attempts);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        LOG_INFOF("WiFi", "Connected! IP: %s", WiFi.localIP().toString().c_str());
        return true;
    } else {
        LOG_ERRORF("WiFi", "Connection failed after %d attempts", max_attempts);
        return false;
    }
}

bool SimpleHardware::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int SimpleHardware::scanWiFiNetworks(WiFiNetworkInfo* networks, int max_networks) {
    if (wifi_status != HW_READY) {
        return 0;
    }
    
    LOG_INFO("WiFi", "Scanning for networks...");
    int network_count = WiFi.scanNetworks();
    
    if (network_count <= 0) {
        LOG_WARN("WiFi", "No networks found");
        return 0;
    }
    
    LOG_INFOF("WiFi", "Found %d networks", network_count);
    
    int count = min(network_count, max_networks);
    for (int i = 0; i < count; i++) {
        networks[i].ssid = WiFi.SSID(i);
        networks[i].rssi = WiFi.RSSI(i);
        networks[i].channel = WiFi.channel(i);
        networks[i].encryption = WiFi.encryptionType(i);
    }
    
    return count;
}

String SimpleHardware::getSystemInfo() {
    String info = "T-Deck-Pro OS Phase 1\n";
    info += "Uptime: " + String(getUptime() / 1000) + "s\n";
    info += "Free Heap: " + String(getFreeHeap() / 1024) + "KB\n";
    info += "Free PSRAM: " + String(getFreePSRAM() / 1024) + "KB\n";
    
    if (isWiFiConnected()) {
        info += "WiFi: Connected\n";
        info += "IP: " + WiFi.localIP().toString() + "\n";
        info += "RSSI: " + String(WiFi.RSSI()) + "dBm\n";
    } else {
        info += "WiFi: Disconnected\n";
    }
    
    return info;
}

uint32_t SimpleHardware::getUptime() {
    return millis();
}

uint32_t SimpleHardware::getFreeHeap() {
    return ESP.getFreeHeap();
}

uint32_t SimpleHardware::getFreePSRAM() {
    return ESP.getFreePsram();
}

bool SimpleHardware::runDiagnostics() {
    LOG_INFO("Diagnostics", "Running hardware diagnostics...");
    
    bool all_ok = true;
    
    if (display_status != HW_READY) {
        LOG_ERROR("Diagnostics", "Display not ready");
        all_ok = false;
    }
    
    if (touch_status != HW_READY) {
        LOG_ERROR("Diagnostics", "Touch controller not ready");
        all_ok = false;
    }
    
    if (wifi_status != HW_READY) {
        LOG_ERROR("Diagnostics", "WiFi not ready");
        all_ok = false;
    }
    
    if (sd_status != HW_READY) {
        LOG_WARN("Diagnostics", "SD card not ready");
    }
    
    LOG_INFOF("Diagnostics", "Diagnostics complete - Status: %s", all_ok ? "PASS" : "FAIL");
    return all_ok;
}
