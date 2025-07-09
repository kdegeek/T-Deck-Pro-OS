/**
 * @file communication_manager.cpp
 * @brief High-level communication manager implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "communication_manager.h"
#include "core/utils/logger.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>

static const char* TAG = "CommMgr";

namespace TDeckOS {
namespace Communication {

// Singleton instance
static CommunicationManager* s_instance = nullptr;

CommunicationManager* CommunicationManager::getInstance() {
    if (!s_instance) {
        s_instance = new CommunicationManager();
    }
    return s_instance;
}

CommunicationManager::CommunicationManager() :
    m_initialized(false),
    m_mutex(nullptr),
    m_eventCallback(nullptr),
    m_initTime(0),
    m_lastInterfaceSwitch(0)
{
    // Initialize statistics
    memset(&m_stats, 0, sizeof(m_stats));
    
    // Create mutex for thread safety
    m_mutex = xSemaphoreCreateMutex();
    if (!m_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
    }
    
    m_initTime = millis();
}

CommunicationManager::~CommunicationManager() {
    deinitialize();
    
    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
        m_mutex = nullptr;
    }
}

bool CommunicationManager::initialize(const NetworkConfig& config) {
    if (m_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }
    
    ESP_LOGI(TAG, "Initializing communication manager");
    
    // Store configuration
    m_config = config;
    
    // Initialize individual managers based on configuration
    bool success = true;
    
    if (m_config.enableLoRa) {
        if (!m_loraManager.initialize(m_config.loraConfig)) {
            ESP_LOGW(TAG, "LoRa initialization failed");
            success = false;
        }
    }
    
    if (m_config.enableWiFi) {
        if (!m_wifiManager.initialize()) {
            ESP_LOGW(TAG, "WiFi initialization failed");
            success = false;
        }
    }
    
    if (m_config.enableCellular) {
        if (!m_cellularManager.initialize(m_config.cellularConfig)) {
            ESP_LOGW(TAG, "Cellular initialization failed");
            success = false;
        }
    }
    
    if (!success) {
        ESP_LOGE(TAG, "Some communication interfaces failed to initialize");
        // Continue anyway - partial functionality is better than none
    }
    
    m_initialized = true;
    ESP_LOGI(TAG, "Communication manager initialized successfully");
    
    return true;
}

void CommunicationManager::deinitialize() {
    if (!m_initialized) {
        return;
    }
    
    ESP_LOGI(TAG, "Deinitializing communication manager");
    
    // Deinitialize all managers
    m_loraManager.deinitialize();
    m_wifiManager.deinitialize();
    m_cellularManager.deinitialize();
    
    m_initialized = false;
    
    ESP_LOGI(TAG, "Communication manager deinitialized");
}

bool CommunicationManager::startAllInterfaces() {
    if (!m_initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return false;
    }
    
    bool success = true;
    
    
    
    
    return success;
}

void CommunicationManager::stopAllInterfaces() {
    if (!m_initialized) {
        return;
    }
    
    
    ESP_LOGI(TAG, "All interfaces stopped");
}

CommStatus CommunicationManager::getStatus() const {
    CommStatus status;
    
    status.loraAvailable = m_loraManager.isInitialized();
    status.wifiAvailable = m_wifiManager.isConnected();
    status.cellularAvailable = m_cellularManager.isConnected();
    status.bluetoothAvailable = false; // Not implemented yet
    
    status.loraMode = m_loraManager.getMode();
    status.wifiStatus = m_wifiManager.getStatus();
    status.cellularStatus = m_cellularManager.getStatus();
    
    // Build active interfaces string
    String interfaces = "";
    if (status.loraAvailable) interfaces += "LoRa ";
    if (status.wifiAvailable) interfaces += "WiFi ";
    if (status.cellularAvailable) interfaces += "Cellular ";
    status.activeInterfaces = interfaces;
    
    return status;
}

bool CommunicationManager::isConnected() const {
    return m_wifiManager.isConnected() || 
           m_cellularManager.isConnected() || 
           m_loraManager.isInitialized();
}

CommInterface CommunicationManager::getBestInterface() const {
    // Priority: WiFi -> Cellular -> LoRa
    if (m_wifiManager.isConnected()) {
        return CommInterface::WIFI;
    } else if (m_cellularManager.isConnected()) {
        return CommInterface::CELLULAR;
    } else if (m_loraManager.isInitialized()) {
        return CommInterface::LORA;
    }
    
    return CommInterface::LORA; // Default fallback
}

bool CommunicationManager::sendData(const uint8_t* data, size_t length, CommInterface interface) {
    if (!m_initialized || !data || length == 0) {
        return false;
    }
    
    bool success = false;
    
    // Take mutex for thread safety
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        switch (interface) {
            case CommInterface::LORA:
                if (m_loraManager.isInitialized()) {
                    success = m_loraManager.transmit(data, length);
                    if (success) {
                        m_stats.loraStats.packetsTransmitted++;
                    }
                }
                break;
                
            case CommInterface::WIFI:
                if (m_wifiManager.isConnected()) {
                    // Placeholder - implement actual WiFi data sending
                    success = true;
                    m_stats.wifiStats.bytesTransmitted += length;
                }
                break;
                
            case CommInterface::CELLULAR:
                if (m_cellularManager.isConnected()) {
                    // Placeholder - implement actual cellular data sending
                    success = true;
                    m_stats.cellularStats.dataBytesSent += length;
                }
                break;
                
            default:
                ESP_LOGE(TAG, "Invalid interface");
                break;
        }
        
        if (success) {
            m_stats.totalBytesTransmitted += length;
        }
        
        xSemaphoreGive(m_mutex);
    }
    
    return success;
}

bool CommunicationManager::sendMessage(const String& message, CommInterface interface) {
    return sendData((const uint8_t*)message.c_str(), message.length(), interface);
}

bool CommunicationManager::broadcastMesh(const String& message) {
    return sendMessage(message, CommInterface::LORA);
}

bool CommunicationManager::connectWiFi(const String& ssid, const String& password) {
    if (!m_initialized) {
        return false;
    }
    
    WiFiStationConfig wifiConfig;
    wifiConfig.ssid = ssid;
    wifiConfig.password = password;
    return m_wifiManager.connect(wifiConfig);
}

bool CommunicationManager::startWiFiAP(const String& ssid, const String& password) {
    if (!m_initialized) {
        return false;
    }
    
    WiFiAPConfig apConfig;
    apConfig.ssid = ssid;
    apConfig.password = password;
    return m_wifiManager.startAP(apConfig);
}

bool CommunicationManager::connectCellular(const String& apn, const String& username, const String& password) {
    if (!m_initialized) {
        return false;
    }
    
    m_config.cellularConfig.apnConfig.apn = apn;
    m_config.cellularConfig.apnConfig.username = username;
    m_config.cellularConfig.apnConfig.password = password;
    return m_cellularManager.connect();
}

bool CommunicationManager::scanWiFi(WiFiScanCallback callback) {
    if (!m_initialized) {
        return false;
    }
    
    return m_wifiManager.scanNetworks(callback);
}

bool CommunicationManager::sendSMS(const String& number, const String& message) {
    if (!m_initialized) {
        return false;
    }
    
    return m_cellularManager.sendSMS(number, message);
}

bool CommunicationManager::setLoRaMode(LoRaMode mode) {
    if (!m_initialized) {
        return false;
    }
    
    return m_loraManager.setMode(mode);
}

bool CommunicationManager::startLoRaReceive(LoRaReceiveCallback callback) {
    if (!m_initialized) {
        return false;
    }
    
    return m_loraManager.startReceive(callback);
}

bool CommunicationManager::updateConfig(const NetworkConfig& config) {
    m_config = config;
    
    // Apply configuration changes
    bool success = true;
    
    if (!enableInterface(CommInterface::LORA, config.enableLoRa)) {
        success = false;
    }
    
    if (!enableInterface(CommInterface::WIFI, config.enableWiFi)) {
        success = false;
    }
    
    if (!enableInterface(CommInterface::CELLULAR, config.enableCellular)) {
        success = false;
    }
    
    return success;
}

CommStats CommunicationManager::getStats() const {
    CommStats statsCopy;
    
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        statsCopy = m_stats;
        statsCopy.uptime = millis() - m_initTime;
        xSemaphoreGive(m_mutex);
    } else {
        memset(&statsCopy, 0, sizeof(statsCopy));
    }
    
    return statsCopy;
}

void CommunicationManager::resetStats() {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        memset(&m_stats, 0, sizeof(m_stats));
        xSemaphoreGive(m_mutex);
        ESP_LOGI(TAG, "Statistics reset");
    }
}

void CommunicationManager::process() {
    if (!m_initialized) {
        return;
    }
    
    // Process each manager
    m_loraManager.process();
    m_wifiManager.process();
    m_cellularManager.process();
    
    // Update statistics
    updateStats();
}

bool CommunicationManager::enableInterface(CommInterface interface, bool enable) {
    if (!m_initialized) {
        return false;
    }
    
    bool success = true;
    
    switch (interface) {
        case CommInterface::LORA:
            m_config.enableLoRa = enable;
            break;
            
        case CommInterface::WIFI:
            m_config.enableWiFi = enable;
            break;
            
        case CommInterface::CELLULAR:
            m_config.enableCellular = enable;
            break;
            
        default:
            success = false;
            break;
    }
    
    return success;
}

bool CommunicationManager::isInterfaceEnabled(CommInterface interface) const {
    switch (interface) {
        case CommInterface::LORA:
            return m_config.enableLoRa;
        case CommInterface::WIFI:
            return m_config.enableWiFi;
        case CommInterface::CELLULAR:
            return m_config.enableCellular;
        default:
            return false;
    }
}

int16_t CommunicationManager::getSignalStrength(CommInterface interface) const {
    switch (interface) {
        case CommInterface::LORA:
            return m_loraManager.getLastRssi();
        case CommInterface::WIFI:
            return m_wifiManager.getRSSI();
        case CommInterface::CELLULAR:
            return const_cast<CellularManager&>(m_cellularManager).getSignalStrength();
        default:
            return -999; // Invalid
    }
}

void CommunicationManager::handleLoRaEvent(bool success, int errorCode) {
    if (m_eventCallback) {
        String event = success ? "success" : "error";
        String data = String(errorCode);
        m_eventCallback(CommInterface::LORA, event, data);
    }
}

void CommunicationManager::handleWiFiEvent(WiFiStatus status, const String& info) {
    if (m_eventCallback) {
        String event = "status_change";
        m_eventCallback(CommInterface::WIFI, event, info);
    }
}

void CommunicationManager::handleCellularEvent(CellularStatus status, const String& info) {
    if (m_eventCallback) {
        String event = "status_change";
        m_eventCallback(CommInterface::CELLULAR, event, info);
    }
}

void CommunicationManager::updateStats() {
    // Update interface switch count
    static CommInterface lastBestInterface = CommInterface::LORA;
    CommInterface currentBest = getBestInterface();
    
    if (currentBest != lastBestInterface) {
        m_stats.interfaceSwitches++;
        m_lastInterfaceSwitch = millis();
        lastBestInterface = currentBest;
    }
    
    // Get stats from individual managers
    m_stats.loraStats = m_loraManager.getStats();
    m_stats.wifiStats = m_wifiManager.getStats();
    m_stats.cellularStats = m_cellularManager.getStats();
    
    // Calculate totals
    m_stats.totalBytesTransmitted = m_stats.loraStats.packetsTransmitted +
                                   m_stats.wifiStats.bytesTransmitted +
                                   m_stats.cellularStats.dataBytesSent;
                                   
    m_stats.totalBytesReceived = m_stats.loraStats.packetsReceived +
                                 m_stats.wifiStats.bytesReceived +
                                 m_stats.cellularStats.dataBytesReceived;
}

bool CommunicationManager::initializeInterface(CommInterface interface) {
    switch (interface) {
        case CommInterface::LORA:
            return m_loraManager.initialize(m_config.loraConfig);
        case CommInterface::WIFI:
            return m_wifiManager.initialize();
        case CommInterface::CELLULAR:
            return m_cellularManager.initialize(m_config.cellularConfig);
        default:
            return false;
    }
}

void CommunicationManager::deinitializeInterface(CommInterface interface) {
    switch (interface) {
        case CommInterface::LORA:
            m_loraManager.deinitialize();
            break;
        case CommInterface::WIFI:
            m_wifiManager.deinitialize();
            break;
        case CommInterface::CELLULAR:
            m_cellularManager.deinitialize();
            break;
        default:
            break;
    }
}

CommInterface CommunicationManager::selectBestInterface() const {
    // Priority based on configuration
    if (m_wifiManager.isConnected() && m_config.enableWiFi) {
        return CommInterface::WIFI;
    } else if (m_cellularManager.isConnected() && m_config.enableCellular) {
        return CommInterface::CELLULAR;
    } else if (m_loraManager.isInitialized() && m_config.enableLoRa) {
        return CommInterface::LORA;
    }
    
    return CommInterface::LORA; // Default fallback
}

bool CommunicationManager::isWiFiConnected() const {
    return m_wifiManager.isConnected();
}

bool CommunicationManager::isCellularConnected() const {
    return m_cellularManager.isConnected();
}

bool CommunicationManager::isLoRaActive() const {
    return m_loraManager.isInitialized();
}

void CommunicationManager::setPreferredInterface(CommInterface interface) {
    m_config.primaryInterface = interface;
}

void CommunicationManager::setAutoFailover(bool enable) {
    // Store auto failover setting in config
    // This could be added to NetworkConfig if needed
}

} // namespace Communication
} // namespace TDeckOS