/**
 * @file lora_driver.h
 * @brief LoRa Driver Header for SX1262
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Defines SX1262 LoRa driver interface and functionality
 */

#ifndef LORA_DRIVER_H
#define LORA_DRIVER_H

#include <Arduino.h>
#include <SPI.h>

/**
 * @brief LoRa spreading factors
 */
typedef enum {
    LORA_SF_7 = 7,
    LORA_SF_8 = 8,
    LORA_SF_9 = 9,
    LORA_SF_10 = 10,
    LORA_SF_11 = 11,
    LORA_SF_12 = 12
} LoRaSpreadingFactor;

/**
 * @brief LoRa bandwidth settings
 */
typedef enum {
    LORA_BW_125 = 0,    ///< 125 kHz
    LORA_BW_250 = 1,    ///< 250 kHz
    LORA_BW_500 = 2     ///< 500 kHz
} LoRaBandwidth;

/**
 * @brief LoRa coding rates
 */
typedef enum {
    LORA_CR_4_5 = 1,    ///< 4/5
    LORA_CR_4_6 = 2,    ///< 4/6
    LORA_CR_4_7 = 3,    ///< 4/7
    LORA_CR_4_8 = 4     ///< 4/8
} LoRaCodingRate;

/**
 * @brief LoRa power levels
 */
typedef enum {
    LORA_POWER_MIN = 2,     ///< Minimum power (2 dBm)
    LORA_POWER_LOW = 10,    ///< Low power (10 dBm)
    LORA_POWER_MED = 14,    ///< Medium power (14 dBm)
    LORA_POWER_HIGH = 17,   ///< High power (17 dBm)
    LORA_POWER_MAX = 22     ///< Maximum power (22 dBm)
} LoRaPowerLevel;

/**
 * @brief LoRa module states
 */
typedef enum {
    LORA_STATE_IDLE,
    LORA_STATE_TX,
    LORA_STATE_RX,
    LORA_STATE_SLEEP,
    LORA_STATE_ERROR
} LoRaState;

/**
 * @brief LoRa packet structure
 */
struct LoRaPacket {
    uint8_t* data;          ///< Packet data
    uint16_t length;        ///< Data length
    int16_t rssi;           ///< Received signal strength indicator
    int8_t snr;             ///< Signal-to-noise ratio
    uint32_t frequency;     ///< Transmission frequency
    uint32_t timestamp;     ///< Packet timestamp
};

/**
 * @brief LoRa Driver Class for SX1262
 * 
 * Provides comprehensive interface for SX1262 LoRa transceiver functionality including:
 * - LoRa modulation configuration
 * - Packet transmission and reception
 * - Power management
 * - Frequency and spreading factor control
 * - RSSI and SNR measurement
 */
class LoRaDriver {
public:
    /**
     * @brief Constructor
     */
    LoRaDriver();
    
    /**
     * @brief Destructor
     */
    ~LoRaDriver();
    
    // ===== CORE INITIALIZATION =====
    
    /**
     * @brief Initialize the LoRa driver
     * @param frequency Operating frequency in Hz
     * @return true if successful, false otherwise
     */
    bool initialize(uint32_t frequency = 915000000);
    
    /**
     * @brief Deinitialize the LoRa driver
     */
    void deinitialize();
    
    /**
     * @brief Check if driver is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    // ===== CONFIGURATION =====
    
    /**
     * @brief Set transmission frequency
     * @param frequency Frequency in Hz
     * @return true if successful
     */
    bool setFrequency(uint32_t frequency);
    
    /**
     * @brief Set spreading factor
     * @param sf Spreading factor (7-12)
     * @return true if successful
     */
    bool setSpreadingFactor(LoRaSpreadingFactor sf);
    
    /**
     * @brief Set bandwidth
     * @param bw Bandwidth setting
     * @return true if successful
     */
    bool setBandwidth(LoRaBandwidth bw);
    
    /**
     * @brief Set coding rate
     * @param cr Coding rate
     * @return true if successful
     */
    bool setCodingRate(LoRaCodingRate cr);
    
    /**
     * @brief Set transmission power
     * @param power Power level in dBm
     * @return true if successful
     */
    bool setTxPower(LoRaPowerLevel power);
    
    /**
     * @brief Set sync word
     * @param sync_word Sync word (0x00-0xFF)
     * @return true if successful
     */
    bool setSyncWord(uint8_t sync_word);
    
    // ===== TRANSMISSION =====
    
    /**
     * @brief Send packet data
     * @param data Data buffer
     * @param length Data length
     * @return true if successful
     */
    bool sendPacket(const uint8_t* data, uint16_t length);
    
    /**
     * @brief Send string message
     * @param message String message
     * @return true if successful
     */
    bool sendMessage(const char* message);
    
    /**
     * @brief Check if transmission is in progress
     * @return true if transmitting
     */
    bool isTransmitting();
    
    // ===== RECEPTION =====
    
    /**
     * @brief Enable continuous receive mode
     * @return true if successful
     */
    bool startReceive();
    
    /**
     * @brief Disable receive mode
     * @return true if successful
     */
    bool stopReceive();
    
    /**
     * @brief Check if packet is available
     * @return true if packet received
     */
    bool packetAvailable();
    
    /**
     * @brief Receive packet data
     * @param packet Packet structure to fill
     * @return true if successful
     */
    bool receivePacket(LoRaPacket& packet);
    
    /**
     * @brief Get last received RSSI
     * @return RSSI in dBm
     */
    int16_t getLastRSSI();
    
    /**
     * @brief Get last received SNR
     * @return SNR in dB
     */
    int8_t getLastSNR();
    
    // ===== POWER MANAGEMENT =====
    
    /**
     * @brief Put module into sleep mode
     * @return true if successful
     */
    bool sleep();
    
    /**
     * @brief Wake up module from sleep
     * @return true if successful
     */
    bool wakeup();
    
    /**
     * @brief Set module to idle mode
     * @return true if successful
     */
    bool setIdle();
    
    // ===== STATUS AND DIAGNOSTICS =====
    
    /**
     * @brief Get current module state
     * @return Current state
     */
    LoRaState getState();
    
    /**
     * @brief Get module version
     * @return Version register value
     */
    uint8_t getVersion();
    
    /**
     * @brief Get current frequency
     * @return Frequency in Hz
     */
    uint32_t getFrequency() const { return frequency_; }
    
    /**
     * @brief Get current spreading factor
     * @return Spreading factor
     */
    LoRaSpreadingFactor getSpreadingFactor() const { return spreading_factor_; }
    
    /**
     * @brief Get transmission statistics
     * @param tx_count Reference to store TX count
     * @param rx_count Reference to store RX count
     * @param error_count Reference to store error count
     */
    void getStatistics(uint32_t& tx_count, uint32_t& rx_count, uint32_t& error_count);
    
    // ===== EVENT PROCESSING =====
    
    /**
     * @brief Process LoRa events and interrupts
     */
    void processEvents();
    
    /**
     * @brief Set callback for packet reception
     * @param callback Function to call when packet received
     */
    void setReceiveCallback(void (*callback)(LoRaPacket&));

private:
    // ===== INTERNAL METHODS =====
    
    /**
     * @brief Initialize hardware pins
     * @return true if successful
     */
    bool initializeHardware();
    
    /**
     * @brief Initialize SPI communication
     * @return true if successful
     */
    bool initializeSPI();
    
    /**
     * @brief Reset the SX1262 module
     * @return true if successful
     */
    bool resetModule();
    
    /**
     * @brief Configure SX1262 for LoRa mode
     * @return true if successful
     */
    bool configureLoRaMode();
    
    /**
     * @brief Write to SX1262 register
     * @param address Register address
     * @param value Value to write
     */
    void writeRegister(uint16_t address, uint8_t value);
    
    /**
     * @brief Read from SX1262 register
     * @param address Register address
     * @return Register value
     */
    uint8_t readRegister(uint16_t address);
    
    /**
     * @brief Send command to SX1262
     * @param command Command code
     * @param params Parameters array
     * @param param_count Number of parameters
     */
    void sendCommand(uint8_t command, const uint8_t* params = nullptr, uint8_t param_count = 0);
    
    /**
     * @brief Wait for command completion
     * @param timeout Timeout in milliseconds
     * @return true if completed successfully
     */
    bool waitCommandComplete(uint32_t timeout = 1000);
    
    /**
     * @brief Handle DIO1 interrupt
     */
    void handleDIO1Interrupt();
    
    // ===== MEMBER VARIABLES =====
    
    bool initialized_;                    ///< Driver initialization state
    LoRaState state_;                     ///< Current module state
    uint32_t frequency_;                  ///< Operating frequency
    LoRaSpreadingFactor spreading_factor_; ///< Spreading factor
    LoRaBandwidth bandwidth_;             ///< Bandwidth setting
    LoRaCodingRate coding_rate_;          ///< Coding rate
    LoRaPowerLevel tx_power_;             ///< Transmission power
    uint8_t sync_word_;                   ///< Sync word
    
    // Statistics
    uint32_t tx_count_;                   ///< Transmission count
    uint32_t rx_count_;                   ///< Reception count
    uint32_t error_count_;                ///< Error count
    
    // Last packet info
    int16_t last_rssi_;                   ///< Last received RSSI
    int8_t last_snr_;                     ///< Last received SNR
    
    // Callback function
    void (*receive_callback_)(LoRaPacket&); ///< Packet receive callback
    
    // Hardware state
    bool dio1_triggered_;                 ///< DIO1 interrupt flag
};

#endif // LORA_DRIVER_H