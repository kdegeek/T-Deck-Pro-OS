/**
 * @file wifi_manager.h
 * @brief T-Deck-Pro WiFi Manager - Network connectivity and management
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Handles WiFi connectivity with power management and automatic reconnection
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>

#include "../hal/board_config_corrected.h"

// ===== WIFI STATES =====
enum class WiFiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR,
    POWER_SAVING
};

// ===== WIFI CONFIGURATION =====
#define WIFI_MAX_SSID_LENGTH 32
#define WIFI_MAX_PASSWORD_LENGTH 64
#define WIFI_CONNECT_TIMEOUT_MS 10000
#define WIFI_RETRY_COUNT 3
#define WIFI_SCAN_TIMEOUT_MS 5000

/**
 * @brief WiFi network information
 */
struct WiFiNetwork {
    String ssid;
    int32_t rssi;
    uint8_t encryption_type;
    uint8_t channel;
    bool is_connected;
};

/**
 * @brief WiFi configuration
 */
struct WiFiConfig {
    String ssid;
    String password;
    String hostname;
    bool auto_connect;
    bool power_saving;
    uint8_t max_retries;
    uint32_t connect_timeout;
};

/**
 * @brief WiFi manager for T-Deck-Pro
 * @note Handles WiFi connectivity with power management
 */
class WiFiManager {
public:
    WiFiManager();
    ~WiFiManager();
    
    /**
     * @brief Initialize WiFi manager
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Check if WiFi is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Get WiFi state
     * @return Current WiFi state
     */
    WiFiState getState() const { return current_state_; }
    
    /**
     * @brief Connect to WiFi network
     * @param ssid Network SSID
     * @param password Network password
     * @return true if successful
     */
    bool connect(const String& ssid, const String& password);
    
    /**
     * @brief Connect using stored configuration
     * @return true if successful
     */
    bool connect();
    
    /**
     * @brief Disconnect from WiFi
     */
    void disconnect();
    
    /**
     * @brief Check if connected to WiFi
     * @return true if connected
     */
    bool isConnected() const;
    
    /**
     * @brief Get current SSID
     * @return SSID string
     */
    String getSSID() const;
    
    /**
     * @brief Get local IP address
     * @return IP address string
     */
    String getLocalIP() const;
    
    /**
     * @brief Get RSSI (signal strength)
     * @return RSSI value
     */
    int32_t getRSSI() const;
    
    /**
     * @brief Get MAC address
     * @return MAC address string
     */
    String getMACAddress() const;
    
    /**
     * @brief Scan for available networks
     * @return Number of networks found
     */
    int scanNetworks();
    
    /**
     * @brief Get scanned networks
     * @return Vector of network information
     */
    std::vector<WiFiNetwork> getScannedNetworks() const;
    
    /**
     * @brief Set WiFi configuration
     * @param config WiFi configuration
     */
    void setConfig(const WiFiConfig& config);
    
    /**
     * @brief Get WiFi configuration
     * @return WiFi configuration
     */
    WiFiConfig getConfig() const { return config_; }
    
    /**
     * @brief Load configuration from storage
     * @return true if successful
     */
    bool loadConfig();
    
    /**
     * @brief Save configuration to storage
     * @return true if successful
     */
    bool saveConfig();
    
    /**
     * @brief Enable/disable power saving
     * @param enabled true to enable
     */
    void setPowerSaving(bool enabled);
    
    /**
     * @brief Check if power saving is enabled
     * @return true if enabled
     */
    bool isPowerSavingEnabled() const { return config_.power_saving; }
    
    /**
     * @brief Set hostname
     * @param hostname Hostname string
     */
    void setHostname(const String& hostname);
    
    /**
     * @brief Get hostname
     * @return Hostname string
     */
    String getHostname() const { return config_.hostname; }
    
    /**
     * @brief Enable/disable auto-connect
     * @param enabled true to enable
     */
    void setAutoConnect(bool enabled);
    
    /**
     * @brief Check if auto-connect is enabled
     * @return true if enabled
     */
    bool isAutoConnectEnabled() const { return config_.auto_connect; }
    
    /**
     * @brief Process WiFi events
     */
    void process();
    
    /**
     * @brief Get connection statistics
     * @return Statistics JSON string
     */
    String getStatistics();
    
    /**
     * @brief Reset connection statistics
     */
    void resetStatistics();
    
    /**
     * @brief Get WiFi status information
     * @return Status JSON string
     */
    String getStatus();

private:
    // ===== WIFI OPERATIONS =====
    
    /**
     * @brief Initialize WiFi hardware
     * @return true if successful
     */
    bool initHardware();
    
    /**
     * @brief Initialize WiFi event handlers
     * @return true if successful
     */
    bool initEventHandlers();
    
    /**
     * @brief Configure WiFi settings
     * @return true if successful
     */
    bool configureWiFi();
    
    /**
     * @brief Attempt connection to network
     * @param ssid Network SSID
     * @param password Network password
     * @return true if successful
     */
    bool attemptConnection(const String& ssid, const String& password);
    
    /**
     * @brief Handle WiFi connection success
     */
    void handleConnectionSuccess();
    
    /**
     * @brief Handle WiFi connection failure
     * @param reason Failure reason
     */
    void handleConnectionFailure(const String& reason);
    
    /**
     * @brief Handle WiFi disconnection
     * @param reason Disconnection reason
     */
    void handleDisconnection(const String& reason);
    
    /**
     * @brief Set WiFi state
     * @param state New state
     */
    void setState(WiFiState state);
    
    /**
     * @brief Log WiFi event
     * @param event Event description
     */
    void logEvent(const String& event);
    
    /**
     * @brief Update connection statistics
     * @param connected true if connected
     */
    void updateStatistics(bool connected);
    
    /**
     * @brief WiFi event handler
     * @param event WiFi event
     */
    void onWiFiEvent(WiFiEvent_t event);

private:
    // ===== MEMBER VARIABLES =====
    bool initialized_;
    WiFiState current_state_;
    WiFiConfig config_;
    
    // Scanned networks
    std::vector<WiFiNetwork> scanned_networks_;
    
    // Connection tracking
    uint32_t connection_attempts_;
    uint32_t successful_connections_;
    uint32_t failed_connections_;
    uint32_t total_connection_time_;
    uint32_t last_connection_time_;
    uint32_t last_disconnect_time_;
    
    // Timing
    uint32_t last_scan_time_;
    uint32_t last_check_time_;
    uint32_t reconnect_interval_;
    
    // Event handlers
    WiFiEventHandler station_connected_handler_;
    WiFiEventHandler station_disconnected_handler_;
    WiFiEventHandler station_got_ip_handler_;
    WiFiEventHandler station_lost_ip_handler_;
};

// ===== GLOBAL WIFI MANAGER INSTANCE =====
extern WiFiManager* g_wifi_manager;

// ===== WIFI UTILITY FUNCTIONS =====

/**
 * @brief Initialize global WiFi manager
 * @return true if successful
 */
bool initializeWiFiManager();

/**
 * @brief Get global WiFi manager instance
 * @return WiFi manager pointer
 */
WiFiManager* getWiFiManager();

/**
 * @brief WiFi event handler callback
 * @param event WiFi event
 * @param info Event information
 */
void wifi_event_handler(WiFiEvent_t event, WiFiEventInfo_t info);

/**
 * @brief Get WiFi encryption type string
 * @param encryption_type Encryption type
 * @return Encryption type string
 */
String getWiFiEncryptionTypeString(uint8_t encryption_type);

/**
 * @brief Get WiFi signal strength string
 * @param rssi RSSI value
 * @return Signal strength string
 */
String getWiFiSignalStrengthString(int32_t rssi);

#endif // WIFI_MANAGER_H 