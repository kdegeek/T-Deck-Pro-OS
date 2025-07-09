/**
 * @file modem_4g_driver.h
 * @brief 4G Modem Driver Header for A7682E
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Defines A7682E 4G modem driver interface and functionality
 */

#ifndef MODEM_4G_DRIVER_H
#define MODEM_4G_DRIVER_H

#include <Arduino.h>
#include <HardwareSerial.h>

/**
 * @brief Modem power states
 */
typedef enum {
    MODEM_POWER_OFF,     ///< Modem is powered off
    MODEM_POWER_ON,      ///< Modem is powered on
    MODEM_POWER_SLEEP    ///< Modem is in sleep mode
} ModemPowerState;

/**
 * @brief Network registration states
 */
typedef enum {
    NETWORK_NOT_REGISTERED,  ///< Not registered to network
    NETWORK_SEARCHING,       ///< Searching for network
    NETWORK_REGISTERED,      ///< Registered to network
    NETWORK_CONNECTED,       ///< Connected with data session
    NETWORK_DENIED           ///< Registration denied
} ModemNetworkState;

/**
 * @brief 4G Modem Driver Class for A7682E
 * 
 * Provides comprehensive interface for A7682E 4G modem functionality including:
 * - Power management
 * - Network registration and connection
 * - HTTP requests
 * - SMS functionality
 * - AT command interface
 */
class Modem4GDriver {
public:
    /**
     * @brief Constructor
     */
    Modem4GDriver();
    
    /**
     * @brief Destructor
     */
    ~Modem4GDriver();
    
    // ===== CORE INITIALIZATION =====
    
    /**
     * @brief Initialize the 4G modem driver
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Deinitialize the 4G modem driver
     */
    void deinitialize();
    
    // ===== POWER MANAGEMENT =====
    
    /**
     * @brief Power on the modem
     * @return true if successful, false otherwise
     */
    bool powerOn();
    
    /**
     * @brief Power off the modem
     * @return true if successful, false otherwise
     */
    bool powerOff();
    
    /**
     * @brief Get current power state
     * @return Current power state
     */
    ModemPowerState getPowerState() const { return power_state_; }
    
    // ===== NETWORK OPERATIONS =====
    
    /**
     * @brief Connect to cellular network
     * @param apn Access Point Name
     * @param username Username (optional)
     * @param password Password (optional)
     * @return true if successful, false otherwise
     */
    bool connectToNetwork(const char* apn, const char* username = nullptr, const char* password = nullptr);
    
    /**
     * @brief Check if connected to network
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Get network registration state
     * @return Current network state
     */
    ModemNetworkState getNetworkState() const;
    
    /**
     * @brief Get signal strength percentage
     * @return Signal strength (0-100%)
     */
    int getSignalStrength();
    
    // ===== COMMUNICATION =====
    
    /**
     * @brief Send HTTP request
     * @param url Target URL
     * @param method HTTP method (GET, POST)
     * @param data Request data (for POST)
     * @param response Response data
     * @return true if successful, false otherwise
     */
    bool sendHTTPRequest(const char* url, const char* method, const char* data, String& response);
    
    /**
     * @brief Send SMS message
     * @param phone_number Target phone number
     * @param message Message text
     * @return true if successful, false otherwise
     */
    bool sendSMS(const char* phone_number, const char* message);
    
    // ===== INFORMATION =====
    
    /**
     * @brief Get modem IMEI
     * @return IMEI string
     */
    String getIMEI() const;
    
    /**
     * @brief Get assigned IP address
     * @return IP address string
     */
    String getIPAddress() const;
    
    // ===== EVENT PROCESSING =====
    
    /**
     * @brief Process modem events and unsolicited responses
     */
    void processEvents();
    
    // ===== AT COMMAND INTERFACE =====
    
    /**
     * @brief Send AT command with default timeout
     * @param command AT command string
     * @return Response string
     */
    String sendATCommand(const char* command);
    
    /**
     * @brief Send AT command with custom timeout
     * @param command AT command string
     * @param timeout Response timeout in milliseconds
     * @return Response string
     */
    String sendATCommand(const char* command, uint32_t timeout);

private:
    // ===== INTERNAL METHODS =====
    
    /**
     * @brief Initialize hardware pins
     * @return true if successful
     */
    bool initializeHardware();
    
    /**
     * @brief Initialize serial communication
     * @return true if successful
     */
    bool initializeSerial();
    
    /**
     * @brief Test AT command communication
     * @return true if successful
     */
    bool testATCommunication();
    
    /**
     * @brief Get modem information (IMEI, version, etc.)
     * @return true if successful
     */
    bool getModemInfo();
    
    /**
     * @brief Wait for network registration
     * @return true if registered successfully
     */
    bool waitForNetworkRegistration();
    
    // ===== MEMBER VARIABLES =====
    
    bool initialized_;                    ///< Driver initialization state
    HardwareSerial* serial_;              ///< Serial interface for AT commands
    ModemPowerState power_state_;         ///< Current power state
    ModemNetworkState network_state_;     ///< Current network state
    int signal_strength_;                 ///< Signal strength percentage
    uint32_t last_command_time_;          ///< Timestamp of last AT command
    uint32_t response_timeout_;           ///< Default response timeout
    String imei_;                         ///< Modem IMEI
    String ip_address_;                   ///< Assigned IP address
    String apn_;                          ///< Current APN
};

#endif // MODEM_4G_DRIVER_H