/**
 * @file cellular_manager.cpp
 * @brief Cellular communication manager implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "cellular_manager.h"
#include "../utils/logger.h"
#include "core/hal/board_config.h"
#include <Arduino.h>

namespace TDeckOS {
namespace Communication {

CellularManager::CellularManager()
    : m_serial(nullptr)
    , m_initialized(false)
    , m_poweredOn(false)
    , m_status(CellularStatus::OFF)
    , m_eventCallback(nullptr)
    , m_smsCallback(nullptr)
    , m_callCallback(nullptr)
    , m_stats{}
    , m_initTime(0)
    , m_lastConnectAttempt(0)
    , m_retryCount(0)
    , m_taskHandle(nullptr)
    , m_commandQueue(nullptr)
    , m_mutex(nullptr)
    , m_lastActivity(0)
{
}

CellularManager::~CellularManager() {
    deinitialize();
}

bool CellularManager::initialize(const CellularConfig& config) {
    if (m_initialized) {
        Logger::warning("CellularManager", "Already initialized");
        return true;
    }

    Logger::info("CellularManager", "Initializing cellular manager...");
    
    m_config = config;
    m_initTime = millis();
    
    // Create mutex
    m_mutex = xSemaphoreCreateMutex();
    if (!m_mutex) {
        Logger::error("CellularManager", "Failed to create mutex");
        return false;
    }
    
    // Create command queue
    m_commandQueue = xQueueCreate(10, sizeof(String*));
    if (!m_commandQueue) {
        Logger::error("CellularManager", "Failed to create command queue");
        vSemaphoreDelete(m_mutex);
        return false;
    }
    
    // Initialize serial communication
    m_serial = &Serial1;
    m_serial->begin(m_config.baudRate, SERIAL_8N1, BOARD_MODEM_RX, BOARD_MODEM_TX);
    
    // Configure control pins
    pinMode(BOARD_MODEM_PWRKEY, OUTPUT);
    pinMode(BOARD_MODEM_DTR, OUTPUT);
    pinMode(BOARD_MODEM_PWR, OUTPUT);
    
    // Enable power supply
    digitalWrite(BOARD_MODEM_PWR, HIGH);
    delay(100);
    
    // Create cellular task
    BaseType_t result = xTaskCreate(
        cellularTask,
        "CellularTask",
        8192,
        this,
        CELLULAR_TASK_PRIORITY,
        &m_taskHandle
    );
    
    if (result != pdPASS) {
        Logger::error("CellularManager", "Failed to create cellular task");
        vQueueDelete(m_commandQueue);
        vSemaphoreDelete(m_mutex);
        return false;
    }
    
    m_initialized = true;
    m_status = CellularStatus::OFF;
    
    // Reset statistics
    resetStats();
    
    Logger::info("CellularManager", "Cellular manager initialized successfully");
    return true;
}

void CellularManager::deinitialize() {
    if (!m_initialized) {
        return;
    }
    
    Logger::info("CellularManager", "Deinitializing cellular manager...");
    
    // Power off modem
    powerOff();
    
    // Stop task
    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
    }
    
    // Disable power supply
    digitalWrite(BOARD_MODEM_PWR, LOW);
    
    // Clean up FreeRTOS objects
    if (m_commandQueue) {
        vQueueDelete(m_commandQueue);
        m_commandQueue = nullptr;
    }
    
    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
        m_mutex = nullptr;
    }
    
    m_initialized = false;
    m_poweredOn = false;
    m_status = CellularStatus::OFF;
    
    Logger::info("CellularManager", "Cellular manager deinitialized");
}

bool CellularManager::powerOn() {
    if (!m_initialized) {
        Logger::error("CellularManager", "Not initialized");
        return false;
    }
    
    if (m_poweredOn) {
        Logger::warning("CellularManager", "Already powered on");
        return true;
    }
    
    Logger::info("CellularManager", "Powering on A7682E modem...");
    
    // Reset modem
    digitalWrite(BOARD_MODEM_DTR, LOW);
    delay(100);
    digitalWrite(BOARD_MODEM_DTR, HIGH);
    delay(100);
    
    // Power on sequence
    digitalWrite(BOARD_MODEM_PWRKEY, LOW);
    delay(1000);
    digitalWrite(BOARD_MODEM_PWRKEY, HIGH);
    delay(2000);
    
    // Wait for modem to respond
    m_status = CellularStatus::INITIALIZING;
    
    // Try to communicate with modem
    for (int i = 0; i < 10; i++) {
        String response;
        if (sendATCommand("AT", response, 1000)) {
            if (response.indexOf("OK") >= 0) {
                m_poweredOn = true;
                Logger::info("CellularManager", "Modem powered on successfully");
                
                // Initialize modem
                if (initializeModem()) {
                    return true;
                } else {
                    Logger::error("CellularManager", "Failed to initialize modem");
                    powerOff();
                    return false;
                }
            }
        }
        delay(1000);
    }
    
    Logger::error("CellularManager", "Failed to power on modem");
    m_status = CellularStatus::ERROR;
    return false;
}

bool CellularManager::powerOff() {
    if (!m_poweredOn) {
        return true;
    }
    
    Logger::info("CellularManager", "Powering off A7682E modem...");
    
    // Send power off command
    String response;
    sendATCommand("AT+CPOF", response, 5000);
    
    // Force power off if needed
    digitalWrite(BOARD_MODEM_PWRKEY, LOW);
    delay(3000);
    digitalWrite(BOARD_MODEM_PWRKEY, HIGH);
    
    m_poweredOn = false;
    m_status = CellularStatus::OFF;
    
    Logger::info("CellularManager", "Modem powered off");
    return true;
}

bool CellularManager::connect(CellularEventCallback callback) {
    if (!m_poweredOn) {
        Logger::error("CellularManager", "Modem not powered on");
        return false;
    }
    
    m_eventCallback = callback;
    m_retryCount = 0;
    m_lastConnectAttempt = millis();
    m_stats.connectAttempts++;
    
    Logger::info("CellularManager", "Connecting to cellular network...");
    m_status = CellularStatus::SEARCHING;
    
    // Check SIM card
    if (getSIMStatus() != SIMStatus::READY) {
        Logger::error("CellularManager", "SIM card not ready");
        m_status = CellularStatus::ERROR;
        return false;
    }
    
    // Set up PDP context
    if (!setupPDP()) {
        Logger::error("CellularManager", "Failed to setup PDP context");
        m_status = CellularStatus::ERROR;
        return false;
    }
    
    // Wait for network registration
    for (int i = 0; i < 30; i++) {
        CellularNetworkInfo info = getNetworkInfo();
        if (info.registration == NetworkRegistration::REGISTERED_HOME ||
            info.registration == NetworkRegistration::REGISTERED_ROAMING) {
            
            m_status = CellularStatus::REGISTERED;
            Logger::info("Cellular", "Registered to network: " + info.operatorName);
            
            // Activate PDP context
            String response;
            if (sendATCommand("AT+CGACT=1,1", response, 30000)) {
                if (response.indexOf("OK") >= 0) {
                    m_status = CellularStatus::CONNECTED;
                    m_stats.successfulConnections++;
                    Logger::info("CellularManager", "Connected to cellular network");
                    
                    if (m_eventCallback) {
                        m_eventCallback(m_status, "Connected");
                    }
                    return true;
                }
            }
        }
        delay(1000);
    }
    
    Logger::error("CellularManager", "Failed to connect to network");
    m_status = CellularStatus::ERROR;
    return false;
}

void CellularManager::disconnect() {
    if (m_status == CellularStatus::CONNECTED) {
        Logger::info("CellularManager", "Disconnecting from cellular network...");
        
        String response;
        sendATCommand("AT+CGACT=0,1", response, 10000);
        
        m_status = CellularStatus::DISCONNECTED;
        m_stats.disconnections++;
        
        if (m_eventCallback) {
            m_eventCallback(m_status, "Disconnected");
        }
    }
}

SIMStatus CellularManager::getSIMStatus() {
    String response;
    if (sendATCommand("AT+CPIN?", response, 1000)) {
        if (response.indexOf("READY") >= 0) {
            return SIMStatus::READY;
        } else if (response.indexOf("SIM PIN") >= 0) {
            return SIMStatus::PIN_REQUIRED;
        } else if (response.indexOf("SIM PUK") >= 0) {
            return SIMStatus::PUK_REQUIRED;
        } else if (response.indexOf("NOT INSERTED") >= 0) {
            return SIMStatus::NOT_INSERTED;
        }
    }
    return SIMStatus::ERROR;
}

CellularNetworkInfo CellularManager::getNetworkInfo() {
    CellularNetworkInfo info;
    
    // Get operator name
    String response;
    if (sendATCommand("AT+COPS?", response, 1000)) {
        int start = response.indexOf("\"");
        if (start >= 0) {
            int end = response.indexOf("\"", start + 1);
            if (end >= 0) {
                info.operatorName = response.substring(start + 1, end);
            }
        }
    }
    
    // Get signal quality
    if (sendATCommand("AT+CSQ", response, 1000)) {
        parseSignalQuality(response);
        info.rssi = m_stats.lastRssi;
        info.signalQuality = m_stats.lastSignalQuality;
    }
    
    // Get network registration
    if (sendATCommand("AT+CREG?", response, 1000)) {
        parseNetworkRegistration(response);
    }
    
    return info;
}

int16_t CellularManager::getSignalStrength() {
    String response;
    if (sendATCommand("AT+CSQ", response, 1000)) {
        parseSignalQuality(response);
        return m_stats.lastRssi;
    }
    return -999;
}

uint8_t CellularManager::getSignalQuality() {
    String response;
    if (sendATCommand("AT+CSQ", response, 1000)) {
        parseSignalQuality(response);
        return m_stats.lastSignalQuality;
    }
    return 0;
}

bool CellularManager::sendSMS(const String& number, const String& message) {
    if (!isConnected()) {
        Logger::error("CellularManager", "Not connected to network");
        return false;
    }
    
    Logger::info("Cellular", "Sending SMS to " + number);
    
    // Set SMS text mode
    String response;
    if (!sendATCommand("AT+CMGF=1", response, 1000)) {
        return false;
    }
    
    // Set recipient
    String cmd = "AT+CMGS=\"" + number + "\"";
    m_serial->println(cmd);
    delay(1000);
    
    // Send message
    m_serial->print(message);
    m_serial->write(0x1A); // Ctrl+Z
    
    // Wait for response
    if (waitForResponse("OK", 30000)) {
        m_stats.smsMessagesSent++;
        Logger::info("CellularManager", "SMS sent successfully");
        return true;
    }
    
    Logger::error("CellularManager", "Failed to send SMS");
    return false;
}

std::vector<SMSMessage> CellularManager::readSMS(bool unreadOnly) {
    std::vector<SMSMessage> messages;
    
    // Set SMS text mode
    String response;
    if (!sendATCommand("AT+CMGF=1", response, 1000)) {
        return messages;
    }
    
    // List messages
    String cmd = unreadOnly ? "AT+CMGL=\"REC UNREAD\"" : "AT+CMGL=\"ALL\"";
    if (sendATCommand(cmd, response, 5000)) {
        // Parse SMS messages from response
        // This is a simplified parser - real implementation would be more robust
        int index = 0;
        while ((index = response.indexOf("+CMGL:", index)) >= 0) {
            SMSMessage msg;
            // Parse message details
            // Implementation would extract index, sender, timestamp, and message
            messages.push_back(msg);
            index++;
        }
    }
    
    return messages;
}

bool CellularManager::deleteSMS(uint16_t index) {
    String cmd = "AT+CMGD=" + String(index);
    String response;
    return sendATCommand(cmd, response, 1000) && response.indexOf("OK") >= 0;
}

bool CellularManager::makeCall(const String& number) {
    if (!isConnected()) {
        Logger::error("CellularManager", "Not connected to network");
        return false;
    }
    
    Logger::info("Cellular", "Making call to " + number);
    
    String cmd = "ATD" + number + ";";
    String response;
    return sendATCommand(cmd, response, 5000);
}

bool CellularManager::answerCall() {
    String response;
    return sendATCommand("ATA", response, 1000);
}

bool CellularManager::hangupCall() {
    String response;
    return sendATCommand("ATH", response, 1000);
}

bool CellularManager::sendATCommand(const String& command, String& response, uint32_t timeoutMs) {
    if (!m_serial) {
        return false;
    }
    
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }
    
    // Clear input buffer
    while (m_serial->available()) {
        m_serial->read();
    }
    
    // Send command
    m_serial->println(command);
    m_lastActivity = millis();
    
    // Wait for response
    response = "";
    uint32_t startTime = millis();
    
    while (millis() - startTime < timeoutMs) {
        if (m_serial->available()) {
            char c = m_serial->read();
            response += c;
            
            if (response.endsWith("OK\r\n") || response.endsWith("ERROR\r\n")) {
                break;
            }
        }
        delay(1);
    }
    
    xSemaphoreGive(m_mutex);
    
    Logger::debug("CellularManager", "AT: %s -> %s", command.c_str(), response.c_str());
    return response.length() > 0;
}

String CellularManager::getModemInfo() {
    String response;
    if (sendATCommand("ATI", response, 1000)) {
        return response;
    }
    return String();
}

String CellularManager::getIMEI() {
    String response;
    if (sendATCommand("AT+CGSN", response, 1000)) {
        // Extract IMEI from response
        int start = response.indexOf('\n');
        if (start >= 0) {
            int end = response.indexOf('\r', start);
            if (end >= 0) {
                return response.substring(start + 1, end);
            }
        }
    }
    return String();
}

String CellularManager::getICCID() {
    String response;
    if (sendATCommand("AT+CCID", response, 1000)) {
        // Extract ICCID from response
        int start = response.indexOf('+');
        if (start >= 0) {
            int end = response.indexOf('\r', start);
            if (end >= 0) {
                return response.substring(start, end);
            }
        }
    }
    return String();
}

bool CellularManager::updateConfig(const CellularConfig& config) {
    m_config = config;
    return true;
}

CellularStats CellularManager::getStats() const {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        m_stats.uptime = millis() - m_initTime;
        CellularStats stats = m_stats;
        xSemaphoreGive(m_mutex);
        return stats;
    }
    return CellularStats{};
}

void CellularManager::resetStats() {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        m_stats = CellularStats{};
        m_initTime = millis();
        xSemaphoreGive(m_mutex);
        Logger::info("CellularManager", "Statistics reset");
    }
}

void CellularManager::process() {
    // Most work is done in the FreeRTOS task
}

void CellularManager::cellularTask(void* parameter) {
    CellularManager* manager = static_cast<CellularManager*>(parameter);
    
    Logger::info("CellularManager", "Cellular task started");
    
    while (true) {
        // Handle incoming data
        manager->handleIncomingData();
        
        // Update statistics
        manager->updateStats();
        
        // Check connection status
        if (manager->m_status == CellularStatus::CONNECTED) {
            // Periodic keep-alive or status check
            if (millis() - manager->m_lastActivity > 60000) {
                String response;
                manager->sendATCommand("AT", response, 1000);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool CellularManager::initializeModem() {
    Logger::info("CellularManager", "Initializing modem...");
    
    String response;
    
    // Disable echo
    if (!sendATCommand("ATE0", response, 1000)) {
        return false;
    }
    
    // Set error reporting
    if (!sendATCommand("AT+CMEE=2", response, 1000)) {
        return false;
    }
    
    // Enable network registration notifications
    if (!sendATCommand("AT+CREG=2", response, 1000)) {
        return false;
    }
    
    // Enable SMS notifications
    if (!sendATCommand("AT+CNMI=2,2,0,0,0", response, 1000)) {
        return false;
    }
    
    Logger::info("CellularManager", "Modem initialized successfully");
    return true;
}

bool CellularManager::waitForResponse(const String& expected, uint32_t timeoutMs) {
    uint32_t startTime = millis();
    String response = "";
    
    while (millis() - startTime < timeoutMs) {
        if (m_serial->available()) {
            char c = m_serial->read();
            response += c;
            
            if (response.indexOf(expected) >= 0) {
                return true;
            }
        }
        delay(1);
    }
    
    return false;
}

bool CellularManager::setupPDP() {
    String response;
    
    // Set APN
    String cmd = "AT+CGDCONT=1,\"IP\",\"" + m_config.apnConfig.apn + "\"";
    if (!sendATCommand(cmd, response, 1000)) {
        return false;
    }
    
    // Set authentication if needed
    if (!m_config.apnConfig.username.isEmpty()) {
        cmd = "AT+CGAUTH=1,1,\"" + m_config.apnConfig.username + "\",\"" + m_config.apnConfig.password + "\"";
        if (!sendATCommand(cmd, response, 1000)) {
            return false;
        }
    }
    
    return true;
}

void CellularManager::handleIncomingData() {
    if (m_serial->available()) {
        String data = m_serial->readString();
        processATResponse(data);
    }
}

void CellularManager::parseNetworkRegistration(const String& response) {
    // Parse +CREG response
    // Implementation would extract registration status
}

void CellularManager::parseSignalQuality(const String& response) {
    // Parse +CSQ response
    int start = response.indexOf("+CSQ: ");
    if (start >= 0) {
        start += 6;
        int comma = response.indexOf(',', start);
        if (comma >= 0) {
            int rssi = response.substring(start, comma).toInt();
            if (rssi != 99) {
                m_stats.lastRssi = -113 + (rssi * 2);
                m_stats.lastSignalQuality = rssi;
            }
        }
    }
}

void CellularManager::parseSMSNotification(const String& response) {
    // Parse incoming SMS notification
    if (response.indexOf("+CMTI:") >= 0) {
        m_stats.smsMessagesReceived++;
        if (m_smsCallback) {
            // Parse and call callback
        }
    }
}

void CellularManager::parseCallNotification(const String& response) {
    // Parse incoming call notification
    if (response.indexOf("RING") >= 0) {
        if (m_callCallback) {
            m_callCallback("", true);
        }
    }
}

void CellularManager::updateStats() {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        m_stats.uptime = millis() - m_initTime;
        xSemaphoreGive(m_mutex);
    }
}

CellularNetworkType CellularManager::parseNetworkType(const String& response) {
    // Parse network type from AT response
    return CellularNetworkType::UNKNOWN;
}

void CellularManager::powerCycle() {
    powerOff();
    delay(5000);
    powerOn();
}

bool CellularManager::checkSIMCard() {
    return getSIMStatus() == SIMStatus::READY;
}

void CellularManager::processATResponse(const String& response) {
    // Process unsolicited responses
    if (response.indexOf("+CMTI:") >= 0) {
        parseSMSNotification(response);
    } else if (response.indexOf("RING") >= 0) {
        parseCallNotification(response);
    } else if (response.indexOf("+CREG:") >= 0) {
        parseNetworkRegistration(response);
    }
}

} // namespace Communication
} // namespace TDeckOS