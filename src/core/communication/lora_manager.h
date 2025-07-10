/**
 * @file lora_manager.h
 * @brief T-Deck-Pro LoRa Manager - SX1262 Communication
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Handles SX1262 LoRa communication with mesh networking support
 */

#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include <Arduino.h>
#include <RadioLib.h>
#include "../utils/logger.h"

// ===== LORA CONSTANTS =====
#define LORA_FREQUENCY_DEFAULT 915000000
#define LORA_BANDWIDTH_DEFAULT 125000
#define LORA_SPREADING_FACTOR_DEFAULT 7
#define LORA_CODING_RATE_DEFAULT 5
#define LORA_OUTPUT_POWER_DEFAULT 10
#define LORA_PREAMBLE_LENGTH_DEFAULT 8
#define LORA_CRC_DEFAULT true

#define LORA_PACKET_SIZE_MAX 255
#define LORA_RECEIVE_TIMEOUT 1000
#define LORA_TRANSMIT_TIMEOUT 5000

// ===== LORA PACKET TYPES =====
enum class LoRaPacketType {
    DATA = 0x01,
    ACK = 0x02,
    HEARTBEAT = 0x03,
    DISCOVERY = 0x04,
    ROUTING = 0x05,
    CONTROL = 0x06
};

// ===== LORA PACKET STRUCTURE =====
struct LoRaPacket {
    uint8_t type;
    uint8_t source_id[4];
    uint8_t destination_id[4];
    uint8_t sequence;
    uint8_t length;
    uint8_t data[LORA_PACKET_SIZE_MAX - 12];
    uint8_t checksum;
    
    LoRaPacket() : type(0), sequence(0), length(0), checksum(0) {
        memset(source_id, 0, 4);
        memset(destination_id, 0, 4);
        memset(data, 0, sizeof(data));
    }
};

// ===== LORA CONFIGURATION =====
struct LoRaConfig {
    uint32_t frequency;
    uint32_t bandwidth;
    uint8_t spreading_factor;
    uint8_t coding_rate;
    int8_t output_power;
    uint8_t preamble_length;
    bool crc_enabled;
    uint8_t sync_word;
    
    LoRaConfig() : frequency(LORA_FREQUENCY_DEFAULT), bandwidth(LORA_BANDWIDTH_DEFAULT),
                   spreading_factor(LORA_SPREADING_FACTOR_DEFAULT), coding_rate(LORA_CODING_RATE_DEFAULT),
                   output_power(LORA_OUTPUT_POWER_DEFAULT), preamble_length(LORA_PREAMBLE_LENGTH_DEFAULT),
                   crc_enabled(LORA_CRC_DEFAULT), sync_word(0x12) {}
};

// ===== LORA MANAGER CLASS =====
class LoRaManager {
public:
    // Constructor/Destructor
    LoRaManager();
    ~LoRaManager();
    
    // Initialization
    bool initialize();
    void cleanup();
    
    // Processing
    void process();
    
    // Configuration
    bool setConfig(const LoRaConfig& config);
    LoRaConfig getConfig() const;
    bool setFrequency(uint32_t frequency);
    bool setBandwidth(uint32_t bandwidth);
    bool setSpreadingFactor(uint8_t sf);
    bool setCodingRate(uint8_t cr);
    bool setOutputPower(int8_t power);
    bool setPreambleLength(uint8_t length);
    bool setCRC(bool enabled);
    bool setSyncWord(uint8_t sync_word);
    
    // Communication
    bool transmit(const LoRaPacket& packet);
    bool transmitData(const uint8_t* data, uint8_t length, uint32_t destination_id = 0);
    bool transmitString(const String& message, uint32_t destination_id = 0);
    bool broadcast(const String& message);
    
    // Reception
    bool hasReceived() const;
    LoRaPacket getReceivedPacket();
    void clearReceived();
    
    // Status
    bool isInitialized() const;
    bool isTransmitting() const;
    bool isReceiving() const;
    int16_t getRSSI() const;
    float getSNR() const;
    uint32_t getPacketCount() const;
    String getStatus() const;
    
    // Utility
    uint32_t getNodeId() const;
    void setNodeId(uint32_t node_id);
    uint8_t getNextSequence();
    bool validatePacket(const LoRaPacket& packet) const;
    uint8_t calculateChecksum(const LoRaPacket& packet) const;

private:
    // Hardware
    SX1262* radio_;
    bool initialized_;
    
    // Configuration
    LoRaConfig config_;
    uint32_t node_id_;
    uint8_t sequence_counter_;
    
    // State
    bool transmitting_;
    bool receiving_;
    int16_t last_rssi_;
    float last_snr_;
    uint32_t packet_count_;
    
    // Reception
    LoRaPacket received_packet_;
    bool packet_received_;
    
    // Internal methods
    bool initHardware();
    bool configureRadio();
    void handleReceive();
    void handleTransmit();
    bool sendPacket(const LoRaPacket& packet);
    bool receivePacket(LoRaPacket& packet);
    void updateStatistics();
};

// ===== GLOBAL LORA MANAGER INSTANCE =====
extern LoRaManager* g_lora_manager;

// ===== GLOBAL LORA FUNCTIONS =====
bool initializeLoRaManager();
LoRaManager* getLoRaManager();

// ===== LORA UTILITY FUNCTIONS =====
uint32_t generateNodeId();
String frequencyToString(uint32_t frequency);
uint32_t stringToFrequency(const String& freq_string);
String spreadingFactorToString(uint8_t sf);
uint8_t stringToSpreadingFactor(const String& sf_string);

#endif // LORA_MANAGER_H 