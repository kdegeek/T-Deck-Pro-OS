/**
 * @file lora_manager.cpp
 * @brief T-Deck-Pro LoRa Manager - SX1262 Implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Handles SX1262 LoRa communication with mesh networking support
 */

#include "lora_manager.h"
#include "../utils/logger.h"
#include "../hal/board_config_corrected.h"

// ===== GLOBAL LORA MANAGER INSTANCE =====
LoRaManager* g_lora_manager = nullptr;

// ===== LORA MANAGER IMPLEMENTATION =====

LoRaManager::LoRaManager() : radio_(nullptr), initialized_(false),
                             node_id_(0), sequence_counter_(0),
                             transmitting_(false), receiving_(false),
                             last_rssi_(0), last_snr_(0.0f), packet_count_(0),
                             packet_received_(false) {
    
    // Generate unique node ID
    node_id_ = generateNodeId();
}

LoRaManager::~LoRaManager() {
    if (initialized_) {
        cleanup();
    }
    
    if (radio_) {
        delete radio_;
        radio_ = nullptr;
    }
}

bool LoRaManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    logInfo("LORA", "Initializing LoRa manager");
    
    // Initialize hardware
    if (!initHardware()) {
        logError("LORA", "Failed to initialize LoRa hardware");
        return false;
    }
    
    // Configure radio
    if (!configureRadio()) {
        logError("LORA", "Failed to configure LoRa radio");
        return false;
    }
    
    initialized_ = true;
    
    logInfo("LORA", "LoRa manager initialized successfully. Node ID: " + String(node_id_, HEX));
    return true;
}

void LoRaManager::cleanup() {
    if (!initialized_) {
        return;
    }
    
    logInfo("LORA", "Cleaning up LoRa manager");
    
    // Stop radio operations
    if (radio_) {
        radio_->standby();
    }
    
    initialized_ = false;
}

bool LoRaManager::initHardware() {
    // Create radio object
    radio_ = new SX1262(new Module(BOARD_LORA_CS, BOARD_LORA_DIO1, BOARD_LORA_RST, BOARD_LORA_BUSY));
    if (!radio_) {
        logError("LORA", "Failed to create SX1262 radio object");
        return false;
    }
    
    // Initialize SPI for LoRa
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
    
    // Initialize radio
    int state = radio_->begin();
    if (state != RADIOLIB_ERR_NONE) {
        logError("LORA", "Failed to initialize SX1262. Error: " + String(state));
        return false;
    }
    
    logInfo("LORA", "LoRa hardware initialized");
    return true;
}

bool LoRaManager::configureRadio() {
    if (!radio_) {
        return false;
    }
    
    // Set frequency
    int state = radio_->setFrequency(config_.frequency);
    if (state != RADIOLIB_ERR_NONE) {
        logError("LORA", "Failed to set frequency. Error: " + String(state));
        return false;
    }
    
    // Set bandwidth
    state = radio_->setBandwidth(config_.bandwidth);
    if (state != RADIOLIB_ERR_NONE) {
        logError("LORA", "Failed to set bandwidth. Error: " + String(state));
        return false;
    }
    
    // Set spreading factor
    state = radio_->setSpreadingFactor(config_.spreading_factor);
    if (state != RADIOLIB_ERR_NONE) {
        logError("LORA", "Failed to set spreading factor. Error: " + String(state));
        return false;
    }
    
    // Set coding rate
    state = radio_->setCodingRate(config_.coding_rate);
    if (state != RADIOLIB_ERR_NONE) {
        logError("LORA", "Failed to set coding rate. Error: " + String(state));
        return false;
    }
    
    // Set output power
    state = radio_->setOutputPower(config_.output_power);
    if (state != RADIOLIB_ERR_NONE) {
        logError("LORA", "Failed to set output power. Error: " + String(state));
        return false;
    }
    
    // Set preamble length
    state = radio_->setPreambleLength(config_.preamble_length);
    if (state != RADIOLIB_ERR_NONE) {
        logError("LORA", "Failed to set preamble length. Error: " + String(state));
        return false;
    }
    
    // Set CRC
    state = radio_->setCRC(config_.crc_enabled);
    if (state != RADIOLIB_ERR_NONE) {
        logError("LORA", "Failed to set CRC. Error: " + String(state));
        return false;
    }
    
    // Set sync word
    state = radio_->setSyncWord(config_.sync_word);
    if (state != RADIOLIB_ERR_NONE) {
        logError("LORA", "Failed to set sync word. Error: " + String(state));
        return false;
    }
    
    logInfo("LORA", "Radio configured successfully");
    return true;
}

void LoRaManager::process() {
    if (!initialized_ || !radio_) {
        return;
    }
    
    // Handle reception
    handleReceive();
    
    // Handle transmission
    handleTransmit();
    
    // Update statistics
    updateStatistics();
}

void LoRaManager::handleReceive() {
    if (receiving_) {
        return;
    }
    
    // Start receiving
    int state = radio_->startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        receiving_ = true;
        logDebug("LORA", "Started receiving");
    }
    
    // Check if packet received
    if (radio_->available()) {
        LoRaPacket packet;
        if (receivePacket(packet)) {
            if (validatePacket(packet)) {
                received_packet_ = packet;
                packet_received_ = true;
                packet_count_++;
                
                logInfo("LORA", "Received packet from " + String(packet.source_id[0], HEX) + 
                        " type: " + String(packet.type) + " length: " + String(packet.length));
            } else {
                logWarn("LORA", "Received invalid packet");
            }
        }
        
        // Stop receiving
        radio_->standby();
        receiving_ = false;
    }
}

void LoRaManager::handleTransmit() {
    if (transmitting_) {
        // Check if transmission is complete
        if (radio_->isTransmitting()) {
            return;
        }
        
        // Transmission complete
        transmitting_ = false;
        logDebug("LORA", "Transmission complete");
    }
}

bool LoRaManager::sendPacket(const LoRaPacket& packet) {
    if (!radio_) {
        return false;
    }
    
    // Prepare packet data
    uint8_t packet_data[LORA_PACKET_SIZE_MAX];
    uint8_t data_length = 0;
    
    // Packet header
    packet_data[data_length++] = packet.type;
    
    // Source ID
    for (int i = 0; i < 4; i++) {
        packet_data[data_length++] = packet.source_id[i];
    }
    
    // Destination ID
    for (int i = 0; i < 4; i++) {
        packet_data[data_length++] = packet.destination_id[i];
    }
    
    // Sequence number
    packet_data[data_length++] = packet.sequence;
    
    // Data length
    packet_data[data_length++] = packet.length;
    
    // Data
    for (int i = 0; i < packet.length; i++) {
        packet_data[data_length++] = packet.data[i];
    }
    
    // Checksum
    packet_data[data_length++] = packet.checksum;
    
    // Transmit packet
    int state = radio_->transmit(packet_data, data_length);
    if (state != RADIOLIB_ERR_NONE) {
        logError("LORA", "Failed to transmit packet. Error: " + String(state));
        return false;
    }
    
    transmitting_ = true;
    logDebug("LORA", "Started transmission, length: " + String(data_length));
    return true;
}

bool LoRaManager::receivePacket(LoRaPacket& packet) {
    if (!radio_) {
        return false;
    }
    
    // Get packet data
    uint8_t packet_data[LORA_PACKET_SIZE_MAX];
    int state = radio_->readData(packet_data, LORA_PACKET_SIZE_MAX);
    if (state < 0) {
        logError("LORA", "Failed to read packet data. Error: " + String(state));
        return false;
    }
    
    if (state < 12) { // Minimum packet size
        logWarn("LORA", "Received packet too small: " + String(state));
        return false;
    }
    
    // Parse packet
    uint8_t index = 0;
    
    // Packet type
    packet.type = packet_data[index++];
    
    // Source ID
    for (int i = 0; i < 4; i++) {
        packet.source_id[i] = packet_data[index++];
    }
    
    // Destination ID
    for (int i = 0; i < 4; i++) {
        packet.destination_id[i] = packet_data[index++];
    }
    
    // Sequence number
    packet.sequence = packet_data[index++];
    
    // Data length
    packet.length = packet_data[index++];
    
    // Data
    for (int i = 0; i < packet.length && index < state; i++) {
        packet.data[i] = packet_data[index++];
    }
    
    // Checksum
    if (index < state) {
        packet.checksum = packet_data[index++];
    }
    
    return true;
}

void LoRaManager::updateStatistics() {
    if (radio_) {
        last_rssi_ = radio_->getRSSI();
        last_snr_ = radio_->getSNR();
    }
}

bool LoRaManager::setConfig(const LoRaConfig& config) {
    config_ = config;
    
    if (initialized_) {
        return configureRadio();
    }
    
    return true;
}

LoRaConfig LoRaManager::getConfig() const {
    return config_;
}

bool LoRaManager::setFrequency(uint32_t frequency) {
    config_.frequency = frequency;
    
    if (initialized_ && radio_) {
        int state = radio_->setFrequency(frequency);
        if (state != RADIOLIB_ERR_NONE) {
            logError("LORA", "Failed to set frequency. Error: " + String(state));
            return false;
        }
        logInfo("LORA", "Frequency set to: " + String(frequency));
    }
    
    return true;
}

bool LoRaManager::setBandwidth(uint32_t bandwidth) {
    config_.bandwidth = bandwidth;
    
    if (initialized_ && radio_) {
        int state = radio_->setBandwidth(bandwidth);
        if (state != RADIOLIB_ERR_NONE) {
            logError("LORA", "Failed to set bandwidth. Error: " + String(state));
            return false;
        }
        logInfo("LORA", "Bandwidth set to: " + String(bandwidth));
    }
    
    return true;
}

bool LoRaManager::setSpreadingFactor(uint8_t sf) {
    config_.spreading_factor = sf;
    
    if (initialized_ && radio_) {
        int state = radio_->setSpreadingFactor(sf);
        if (state != RADIOLIB_ERR_NONE) {
            logError("LORA", "Failed to set spreading factor. Error: " + String(state));
            return false;
        }
        logInfo("LORA", "Spreading factor set to: " + String(sf));
    }
    
    return true;
}

bool LoRaManager::setCodingRate(uint8_t cr) {
    config_.coding_rate = cr;
    
    if (initialized_ && radio_) {
        int state = radio_->setCodingRate(cr);
        if (state != RADIOLIB_ERR_NONE) {
            logError("LORA", "Failed to set coding rate. Error: " + String(state));
            return false;
        }
        logInfo("LORA", "Coding rate set to: " + String(cr));
    }
    
    return true;
}

bool LoRaManager::setOutputPower(int8_t power) {
    config_.output_power = power;
    
    if (initialized_ && radio_) {
        int state = radio_->setOutputPower(power);
        if (state != RADIOLIB_ERR_NONE) {
            logError("LORA", "Failed to set output power. Error: " + String(state));
            return false;
        }
        logInfo("LORA", "Output power set to: " + String(power) + " dBm");
    }
    
    return true;
}

bool LoRaManager::setPreambleLength(uint8_t length) {
    config_.preamble_length = length;
    
    if (initialized_ && radio_) {
        int state = radio_->setPreambleLength(length);
        if (state != RADIOLIB_ERR_NONE) {
            logError("LORA", "Failed to set preamble length. Error: " + String(state));
            return false;
        }
        logInfo("LORA", "Preamble length set to: " + String(length));
    }
    
    return true;
}

bool LoRaManager::setCRC(bool enabled) {
    config_.crc_enabled = enabled;
    
    if (initialized_ && radio_) {
        int state = radio_->setCRC(enabled);
        if (state != RADIOLIB_ERR_NONE) {
            logError("LORA", "Failed to set CRC. Error: " + String(state));
            return false;
        }
        logInfo("LORA", "CRC " + String(enabled ? "enabled" : "disabled"));
    }
    
    return true;
}

bool LoRaManager::setSyncWord(uint8_t sync_word) {
    config_.sync_word = sync_word;
    
    if (initialized_ && radio_) {
        int state = radio_->setSyncWord(sync_word);
        if (state != RADIOLIB_ERR_NONE) {
            logError("LORA", "Failed to set sync word. Error: " + String(state));
            return false;
        }
        logInfo("LORA", "Sync word set to: 0x" + String(sync_word, HEX));
    }
    
    return true;
}

bool LoRaManager::transmit(const LoRaPacket& packet) {
    if (!initialized_ || transmitting_) {
        return false;
    }
    
    return sendPacket(packet);
}

bool LoRaManager::transmitData(const uint8_t* data, uint8_t length, uint32_t destination_id) {
    if (length > LORA_PACKET_SIZE_MAX - 12) {
        logError("LORA", "Data too large for packet");
        return false;
    }
    
    LoRaPacket packet;
    packet.type = (uint8_t)LoRaPacketType::DATA;
    packet.sequence = getNextSequence();
    packet.length = length;
    
    // Set source ID
    for (int i = 0; i < 4; i++) {
        packet.source_id[i] = (node_id_ >> (8 * (3 - i))) & 0xFF;
    }
    
    // Set destination ID
    for (int i = 0; i < 4; i++) {
        packet.destination_id[i] = (destination_id >> (8 * (3 - i))) & 0xFF;
    }
    
    // Copy data
    memcpy(packet.data, data, length);
    
    // Calculate checksum
    packet.checksum = calculateChecksum(packet);
    
    return transmit(packet);
}

bool LoRaManager::transmitString(const String& message, uint32_t destination_id) {
    return transmitData((const uint8_t*)message.c_str(), message.length(), destination_id);
}

bool LoRaManager::broadcast(const String& message) {
    return transmitString(message, 0xFFFFFFFF); // Broadcast to all nodes
}

bool LoRaManager::hasReceived() const {
    return packet_received_;
}

LoRaPacket LoRaManager::getReceivedPacket() {
    LoRaPacket packet = received_packet_;
    packet_received_ = false;
    return packet;
}

void LoRaManager::clearReceived() {
    packet_received_ = false;
}

bool LoRaManager::isInitialized() const {
    return initialized_;
}

bool LoRaManager::isTransmitting() const {
    return transmitting_;
}

bool LoRaManager::isReceiving() const {
    return receiving_;
}

int16_t LoRaManager::getRSSI() const {
    return last_rssi_;
}

float LoRaManager::getSNR() const {
    return last_snr_;
}

uint32_t LoRaManager::getPacketCount() const {
    return packet_count_;
}

String LoRaManager::getStatus() const {
    DynamicJsonDocument doc(1024);
    
    doc["initialized"] = initialized_;
    doc["transmitting"] = transmitting_;
    doc["receiving"] = receiving_;
    doc["packet_count"] = packet_count_;
    doc["node_id"] = node_id_;
    doc["rssi"] = last_rssi_;
    doc["snr"] = last_snr_;
    
    JsonObject config = doc.createNestedObject("config");
    config["frequency"] = config_.frequency;
    config["bandwidth"] = config_.bandwidth;
    config["spreading_factor"] = config_.spreading_factor;
    config["coding_rate"] = config_.coding_rate;
    config["output_power"] = config_.output_power;
    config["preamble_length"] = config_.preamble_length;
    config["crc_enabled"] = config_.crc_enabled;
    config["sync_word"] = config_.sync_word;
    
    String output;
    serializeJson(doc, output);
    return output;
}

uint32_t LoRaManager::getNodeId() const {
    return node_id_;
}

void LoRaManager::setNodeId(uint32_t node_id) {
    node_id_ = node_id;
    logInfo("LORA", "Node ID set to: " + String(node_id, HEX));
}

uint8_t LoRaManager::getNextSequence() {
    return ++sequence_counter_;
}

bool LoRaManager::validatePacket(const LoRaPacket& packet) const {
    // Check checksum
    uint8_t calculated_checksum = calculateChecksum(packet);
    if (calculated_checksum != packet.checksum) {
        return false;
    }
    
    // Check if packet is for this node or broadcast
    uint32_t dest_id = 0;
    for (int i = 0; i < 4; i++) {
        dest_id = (dest_id << 8) | packet.destination_id[i];
    }
    
    return (dest_id == node_id_ || dest_id == 0xFFFFFFFF);
}

uint8_t LoRaManager::calculateChecksum(const LoRaPacket& packet) const {
    uint8_t checksum = 0;
    
    checksum ^= packet.type;
    checksum ^= packet.sequence;
    checksum ^= packet.length;
    
    for (int i = 0; i < 4; i++) {
        checksum ^= packet.source_id[i];
        checksum ^= packet.destination_id[i];
    }
    
    for (int i = 0; i < packet.length; i++) {
        checksum ^= packet.data[i];
    }
    
    return checksum;
}

// ===== GLOBAL LORA MANAGER FUNCTIONS =====

bool initializeLoRaManager() {
    if (g_lora_manager) {
        return true;
    }
    
    g_lora_manager = new LoRaManager();
    if (!g_lora_manager) {
        return false;
    }
    
    return g_lora_manager->initialize();
}

LoRaManager* getLoRaManager() {
    return g_lora_manager;
}

// ===== LORA UTILITY FUNCTIONS =====

uint32_t generateNodeId() {
    // Generate a pseudo-random node ID based on hardware
    uint32_t id = 0;
    
    // Use ESP32 unique ID if available
    uint64_t chip_id = ESP.getEfuseMac();
    id = (uint32_t)(chip_id & 0xFFFFFFFF);
    
    // Add some randomness
    id ^= (uint32_t)millis();
    id ^= (uint32_t)random(0xFFFFFFFF);
    
    return id;
}

String frequencyToString(uint32_t frequency) {
    return String(frequency / 1000000.0, 1) + " MHz";
}

uint32_t stringToFrequency(const String& freq_string) {
    // Parse frequency string like "915.0 MHz"
    String clean = freq_string;
    clean.replace(" MHz", "");
    clean.replace("MHz", "");
    clean.trim();
    
    float freq_mhz = clean.toFloat();
    return (uint32_t)(freq_mhz * 1000000);
}

String spreadingFactorToString(uint8_t sf) {
    return "SF" + String(sf);
}

uint8_t stringToSpreadingFactor(const String& sf_string) {
    // Parse spreading factor string like "SF7"
    String clean = sf_string;
    clean.replace("SF", "");
    clean.trim();
    
    return clean.toInt();
} 