/**
 * @file lora_driver.cpp
 * @brief LoRa Driver Implementation for SX1262 on T-Deck-Pro
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Implements SX1262 LoRa transceiver driver with SPI communication
 */

#include "lora_driver.h"
#include <Arduino.h>
#include "config/os_config_corrected.h"
#include <SPI.h>

// SX1262 Register definitions
#define SX1262_REG_VERSION                0x0153
#define SX1262_REG_PA_CONFIG              0x095F
#define SX1262_REG_PA_EFFICIENCY          0x096E
#define SX1262_REG_TX_PARAMS              0x08E7
#define SX1262_REG_TX_MODULATION          0x0889
#define SX1262_REG_PACKET_PARAMS          0x08C1
#define SX1262_REG_PAYLOAD_LENGTH         0x0901
#define SX1262_REG_SYNC_WORD              0x08C7
#define SX1262_REG_NODE_ADDRESS           0x08CD
#define SX1262_REG_BROADCAST_ADDRESS      0x08CE

// SX1262 Commands
#define SX1262_CMD_GET_STATUS             0xC0
#define SX1262_CMD_WRITE_REGISTER         0x0D
#define SX1262_CMD_READ_REGISTER          0x1D
#define SX1262_CMD_WRITE_BUFFER           0x0E
#define SX1262_CMD_READ_BUFFER            0x1E
#define SX1262_CMD_SET_SLEEP              0x84
#define SX1262_CMD_SET_STANDBY            0x80
#define SX1262_CMD_SET_FS                 0xC1
#define SX1262_CMD_SET_TX                 0x83
#define SX1262_CMD_SET_RX                 0x82
#define SX1262_CMD_SET_RX_DUTY_CYCLE      0x94
#define SX1262_CMD_SET_CAD                0xC5
#define SX1262_CMD_SET_TX_CONTINUOUS      0xD1
#define SX1262_CMD_SET_TX_INFINITE_PREAMBLE 0xD2
#define SX1262_CMD_SET_REGULATOR_MODE     0x96
#define SX1262_CMD_CALIBRATE              0x89
#define SX1262_CMD_CALIBRATE_IMAGE        0x98
#define SX1262_CMD_SET_PA_CONFIG          0x95
#define SX1262_CMD_SET_RX_TX_FALLBACK     0x93
#define SX1262_CMD_WRITE_RADIO_REGISTER   0x18
#define SX1262_CMD_READ_RADIO_REGISTER    0x19
#define SX1262_CMD_WRITE_RADIO_BUFFER     0x1A
#define SX1262_CMD_READ_RADIO_BUFFER      0x1B
#define SX1262_CMD_SET_DIO_IRQ_PARAMS     0x08
#define SX1262_CMD_GET_IRQ_STATUS         0x12
#define SX1262_CMD_CLEAR_IRQ_STATUS       0x02
#define SX1262_CMD_SET_DIO2_AS_RF_SWITCH  0x9D
#define SX1262_CMD_SET_DIO3_AS_TCXO       0x97
#define SX1262_CMD_SET_RF_FREQUENCY       0x86
#define SX1262_CMD_SET_PACKET_TYPE        0x8A
#define SX1262_CMD_GET_PACKET_TYPE        0x11
#define SX1262_CMD_SET_TX_PARAMS          0x8E
#define SX1262_CMD_SET_MODULATION_PARAMS  0x8B
#define SX1262_CMD_SET_PACKET_PARAMS      0x8C
#define SX1262_CMD_GET_RX_BUFFER_STATUS   0x13
#define SX1262_CMD_GET_PACKET_STATUS      0x14
#define SX1262_CMD_GET_RSSI_INST          0x15
#define SX1262_CMD_GET_STATS              0x10
#define SX1262_CMD_RESET_STATS            0x00
#define SX1262_CMD_CFG_DIO_IRQ            0x08
#define SX1262_CMD_SET_RFSWITCHMODE       0x9D
#define SX1262_CMD_SET_STOPRXTIMERONPREAMBLE 0x9F

// Status values
#define SX1262_STATUS_MODE_SLEEP          0x00
#define SX1262_STATUS_MODE_STBY_RC        0x20
#define SX1262_STATUS_MODE_STBY_XOSC      0x30
#define SX1262_STATUS_MODE_FS             0x40
#define SX1262_STATUS_MODE_RX             0x50
#define SX1262_STATUS_MODE_TX             0x60

// IRQ flags
#define SX1262_IRQ_TX_DONE                0x0001
#define SX1262_IRQ_RX_DONE                0x0002
#define SX1262_IRQ_PREAMBLE_DETECTED      0x0004
#define SX1262_IRQ_SYNC_WORD_VALID        0x0008
#define SX1262_IRQ_HEADER_VALID           0x0010
#define SX1262_IRQ_HEADER_ERROR           0x0020
#define SX1262_IRQ_CRC_ERROR              0x0040
#define SX1262_IRQ_CAD_DONE               0x0080
#define SX1262_IRQ_CAD_DETECTED           0x0100
#define SX1262_IRQ_TIMEOUT                0x0200

LoRaDriver::LoRaDriver() : 
    initialized_(false),
    frequency_(915000000),
    spreading_factor_(7),
    bandwidth_(LORA_BW_125),
    coding_rate_(LORA_CR_4_5),
    power_(14),
    sync_word_(0x1424),
    implicit_header_(false),
    packet_length_(0),
    tx_callback_(nullptr),
    rx_callback_(nullptr),
    cad_callback_(nullptr) {
}

LoRaDriver::~LoRaDriver() {
    deinitialize();
}

bool LoRaDriver::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Initialize SPI if not already done
    SPI.begin(T_DECK_SPI_SCK, T_DECK_SPI_MISO, T_DECK_SPI_MOSI);
    SPI.setFrequency(8000000); // 8MHz for SX1262
    
    // Initialize CS pin
    pinMode(T_DECK_LORA_CS, OUTPUT);
    digitalWrite(T_DECK_LORA_CS, HIGH);
    
    // Initialize reset pin
    pinMode(T_DECK_LORA_RST, OUTPUT);
    
    // Initialize busy pin
    pinMode(T_DECK_LORA_BUSY, INPUT);
    
    // Initialize DIO pins
    pinMode(BOARD_LORA_INT, INPUT);
    pinMode(T_DECK_LORA_DIO3, INPUT);
    
    // Reset the module
    if (!reset()) {
        Serial.println("[LoRa] Reset failed");
        return false;
    }
    
    // Wait for busy to go low
    waitOnBusy();
    
    // Check version
    uint8_t version = readRegister(SX1262_REG_VERSION);
    Serial.printf("[LoRa] Version: 0x%02X\n", version);
    
    // Set packet type to LoRa
    if (!setPacketType(LoRaPacketType::LORA_PACKET_TYPE_LORA)) {
        Serial.println("[LoRa] Failed to set packet type");
        return false;
    }
    
    // Set RF frequency
    if (!setFrequency(frequency_)) {
        Serial.println("[LoRa] Failed to set frequency");
        return false;
    }
    
    // Set modulation parameters
    if (!setModulationParams(spreading_factor_, bandwidth_, coding_rate_)) {
        Serial.println("[LoRa] Failed to set modulation parameters");
        return false;
    }
    
    // Set packet parameters
    if (!setPacketParams(8, false, 255, true, false)) {
        Serial.println("[LoRa] Failed to set packet parameters");
        return false;
    }
    
    // Set PA configuration
    if (!setPAConfig(0x04, 0x07, 0x00, 0x01)) {
        Serial.println("[LoRa] Failed to set PA config");
        return false;
    }
    
    // Set TX parameters
    if (!setTxParams(power_, LoRaPARamp::LORA_PA_RAMP_200U)) {
        Serial.println("[LoRa] Failed to set TX parameters");
        return false;
    }
    
    // Set DIO IRQ parameters
    setDioIrqParams(SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE | SX1262_IRQ_TIMEOUT,
                    SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE | SX1262_IRQ_TIMEOUT,
                    0x0000, 0x0000);
    
    // Set regulator mode
    setRegulatorMode(LoRaRegulatorMode::LORA_REGULATOR_DC_DC);
    
    // Calibrate
    calibrate(0x7F);
    
    // Set standby mode
    setStandby(LoRaStandbyMode::LORA_STANDBY_RC);
    
    initialized_ = true;
    Serial.println("[LoRa] Initialization successful");
    
    return true;
}

void LoRaDriver::deinitialize() {
    if (!initialized_) {
        return;
    }
    
    // Set to sleep mode
    setSleep(LoRaSleepConfig::LORA_SLEEP_COLD_START);
    
    initialized_ = false;
    Serial.println("[LoRa] Deinitialized");
}

bool LoRaDriver::reset() {
    digitalWrite(T_DECK_LORA_RST, LOW);
    delay(10);
    digitalWrite(T_DECK_LORA_RST, HIGH);
    delay(20);
    
    return true;
}

void LoRaDriver::waitOnBusy() const {
    uint32_t timeout = millis() + 1000; // 1 second timeout
    while (digitalRead(T_DECK_LORA_BUSY) && millis() < timeout) {
        delay(1);
    }
}

bool LoRaDriver::writeCommand(uint8_t command, const uint8_t* data, uint8_t length) {
    waitOnBusy();
    
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(T_DECK_LORA_CS, LOW);
    
    SPI.transfer(command);
    
    for (uint8_t i = 0; i < length; i++) {
        SPI.transfer(data[i]);
    }
    
    digitalWrite(T_DECK_LORA_CS, HIGH);
    SPI.endTransaction();
    
    return true;
}

bool LoRaDriver::readCommand(uint8_t command, uint8_t* data, uint8_t length) const {
    waitOnBusy();
    
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(T_DECK_LORA_CS, LOW);
    
    SPI.transfer(command);
    SPI.transfer(0x00); // NOP
    
    for (uint8_t i = 0; i < length; i++) {
        data[i] = SPI.transfer(0x00);
    }
    
    digitalWrite(T_DECK_LORA_CS, HIGH);
    SPI.endTransaction();
    
    return true;
}

uint8_t LoRaDriver::readRegister(uint16_t address) {
    uint8_t data[3] = {(uint8_t)(address >> 8), (uint8_t)(address & 0xFF), 0x00};
    uint8_t result;
    
    writeCommand(SX1262_CMD_READ_REGISTER, data, 3);
    readCommand(SX1262_CMD_READ_REGISTER, &result, 1);
    
    return result;
}

bool LoRaDriver::writeRegister(uint16_t address, uint8_t value) {
    uint8_t data[3] = {(uint8_t)(address >> 8), (uint8_t)(address & 0xFF), value};
    return writeCommand(SX1262_CMD_WRITE_REGISTER, data, 3);
}

bool LoRaDriver::setPacketType(LoRaPacketType type) {
    uint8_t data = (uint8_t)type;
    return writeCommand(SX1262_CMD_SET_PACKET_TYPE, &data, 1);
}

bool LoRaDriver::setFrequency(uint32_t frequency) {
    frequency_ = frequency;
    
    // Convert frequency to register value
    uint32_t freq_reg = (uint32_t)((double)frequency / 32000000.0 * 33554432.0);
    
    uint8_t data[4] = {
        (uint8_t)(freq_reg >> 24),
        (uint8_t)(freq_reg >> 16),
        (uint8_t)(freq_reg >> 8),
        (uint8_t)(freq_reg & 0xFF)
    };
    
    return writeCommand(SX1262_CMD_SET_RF_FREQUENCY, data, 4);
}

bool LoRaDriver::setModulationParams(uint8_t sf, LoRaBandwidth bw, LoRaCodingRate cr) {
    spreading_factor_ = sf;
    bandwidth_ = bw;
    coding_rate_ = cr;
    
    uint8_t data[4] = {sf, (uint8_t)bw, (uint8_t)cr, 0x00}; // LDRO auto
    return writeCommand(SX1262_CMD_SET_MODULATION_PARAMS, data, 4);
}

bool LoRaDriver::setPacketParams(uint8_t preamble_length, bool implicit_header, 
                                uint8_t payload_length, bool crc_on, bool iq_inverted) {
    implicit_header_ = implicit_header;
    packet_length_ = payload_length;
    
    uint8_t data[6] = {
        0x00, preamble_length,           // Preamble length (MSB, LSB)
        implicit_header ? 0x01 : 0x00,   // Header type
        payload_length,                  // Payload length
        crc_on ? 0x01 : 0x00,           // CRC on
        iq_inverted ? 0x01 : 0x00       // IQ inverted
    };
    
    return writeCommand(SX1262_CMD_SET_PACKET_PARAMS, data, 6);
}

bool LoRaDriver::setPAConfig(uint8_t pa_duty_cycle, uint8_t hp_max, uint8_t device_sel, uint8_t pa_lut) {
    uint8_t data[4] = {pa_duty_cycle, hp_max, device_sel, pa_lut};
    return writeCommand(SX1262_CMD_SET_PA_CONFIG, data, 4);
}

bool LoRaDriver::setTxParams(int8_t power, LoRaPARamp ramp_time) {
    power_ = power;
    
    uint8_t data[2] = {(uint8_t)power, (uint8_t)ramp_time};
    return writeCommand(SX1262_CMD_SET_TX_PARAMS, data, 2);
}

bool LoRaDriver::setDioIrqParams(uint16_t irq_mask, uint16_t dio1_mask, uint16_t dio2_mask, uint16_t dio3_mask) {
    uint8_t data[8] = {
        (uint8_t)(irq_mask >> 8), (uint8_t)(irq_mask & 0xFF),
        (uint8_t)(dio1_mask >> 8), (uint8_t)(dio1_mask & 0xFF),
        (uint8_t)(dio2_mask >> 8), (uint8_t)(dio2_mask & 0xFF),
        (uint8_t)(dio3_mask >> 8), (uint8_t)(dio3_mask & 0xFF)
    };
    
    return writeCommand(SX1262_CMD_SET_DIO_IRQ_PARAMS, data, 8);
}

bool LoRaDriver::setRegulatorMode(LoRaRegulatorMode mode) {
    uint8_t data = (uint8_t)mode;
    return writeCommand(SX1262_CMD_SET_REGULATOR_MODE, &data, 1);
}

bool LoRaDriver::calibrate(uint8_t calibrate_param) {
    uint8_t data = calibrate_param;
    return writeCommand(SX1262_CMD_CALIBRATE, &data, 1);
}

bool LoRaDriver::setStandby(LoRaStandbyMode mode) {
    uint8_t data = (uint8_t)mode;
    return writeCommand(SX1262_CMD_SET_STANDBY, &data, 1);
}

bool LoRaDriver::setSleep(LoRaSleepConfig config) {
    uint8_t data = (uint8_t)config;
    return writeCommand(SX1262_CMD_SET_SLEEP, &data, 1);
}

bool LoRaDriver::transmit(const uint8_t* data, uint8_t length, uint32_t timeout_ms) {
    if (!initialized_ || length > 255) {
        return false;
    }
    
    // Set standby mode
    setStandby(LoRaStandbyMode::LORA_STANDBY_RC);
    
    // Set to TX mode
    uint8_t tx_params[] = {0x00, 0x00, 0x00, 0x00}; // Timeout
    writeCommand(SX1262_CMD_SET_TX, tx_params, 4);
    
    // Wait for busy
    waitOnBusy();
    
    // Write payload
    writeCommand(SX1262_CMD_WRITE_BUFFER, data, length);
    
    // Wait for busy
    waitOnBusy();
    
    // Start transmission
    uint8_t start_tx[] = {0x00, 0x00, 0x00, 0x00}; // Timeout
    writeCommand(SX1262_CMD_SET_TX, start_tx, 4);
    
    // Wait for transmission to complete
    uint32_t start_time = millis();
    while (millis() - start_time < timeout_ms) {
        uint16_t irq_status = getIrqStatus();
        if (irq_status & SX1262_IRQ_TX_DONE) {
            clearIrqStatus(SX1262_IRQ_TX_DONE);
            if (tx_callback_) {
                tx_callback_(true);
            }
            return true;
        }
        if (irq_status & SX1262_IRQ_TIMEOUT) {
            clearIrqStatus(SX1262_IRQ_TIMEOUT);
            if (tx_callback_) {
                tx_callback_(false);
            }
            return false;
        }
        delay(1);
    }
    
    if (tx_callback_) {
        tx_callback_(false);
    }
    return false;
}

bool LoRaDriver::receive(uint8_t* buffer, uint8_t max_length, uint8_t* received_length, uint32_t timeout_ms) {
    if (!initialized_ || !buffer || !received_length) {
        return false;
    }
    
    *received_length = 0;
    
    // Set standby mode
    setStandby(LoRaStandbyMode::LORA_STANDBY_RC);
    
    // Clear IRQ status
    clearIrqStatus(0xFFFF);
    
    // Set RX mode
    uint8_t rx_data[3] = {0x00, 0x00, 0x00}; // No timeout
    if (timeout_ms > 0) {
        uint32_t timeout_ticks = (timeout_ms * 64) / 1000; // Convert to ticks
        rx_data[0] = (uint8_t)(timeout_ticks >> 16);
        rx_data[1] = (uint8_t)(timeout_ticks >> 8);
        rx_data[2] = (uint8_t)(timeout_ticks & 0xFF);
    }
    
    writeCommand(SX1262_CMD_SET_RX, rx_data, 3);
    
    // Wait for RX done or timeout
    uint32_t start_time = millis();
    while (millis() - start_time < (timeout_ms > 0 ? timeout_ms : 30000)) {
        uint16_t irq_status = getIrqStatus();
        
        if (irq_status & SX1262_IRQ_RX_DONE) {
            // Get packet status
            uint8_t status_data[3];
            readCommand(SX1262_CMD_GET_RX_BUFFER_STATUS, status_data, 3);
            
            uint8_t payload_length = status_data[0];
            uint8_t rx_start_buffer_pointer = status_data[1];
            
            if (payload_length > max_length) {
                payload_length = max_length;
            }
            
            // Read data from buffer
            uint8_t read_data[256];
            read_data[0] = rx_start_buffer_pointer;
            read_data[1] = 0x00; // NOP
            
            writeCommand(SX1262_CMD_READ_BUFFER, read_data, 2);
            readCommand(SX1262_CMD_READ_BUFFER, buffer, payload_length);
            
            *received_length = payload_length;
            
            clearIrqStatus(SX1262_IRQ_RX_DONE);
            
            if (rx_callback_) {
                rx_callback_(buffer, payload_length, getRSSI(), getSNR());
            }
            
            return true;
        }
        
        if (irq_status & SX1262_IRQ_TIMEOUT) {
            clearIrqStatus(SX1262_IRQ_TIMEOUT);
            return false;
        }
        
        if (irq_status & SX1262_IRQ_CRC_ERROR) {
            clearIrqStatus(SX1262_IRQ_CRC_ERROR);
            return false;
        }
        
        delay(1);
    }
    
    return false;
}

bool LoRaDriver::startCAD(uint32_t timeout_ms) {
    if (!initialized_) {
        return false;
    }
    
    // Set standby mode
    setStandby(LoRaStandbyMode::LORA_STANDBY_RC);
    
    // Clear IRQ status
    clearIrqStatus(0xFFFF);
    
    // Start CAD
    writeCommand(SX1262_CMD_SET_CAD, nullptr, 0);
    
    return true;
}

uint16_t LoRaDriver::getIrqStatus() const {
    uint8_t data[2];
    readCommand(SX1262_CMD_GET_IRQ_STATUS, data, 2);
    return ((uint16_t)data[0] << 8) | data[1];
}

bool LoRaDriver::clearIrqStatus(uint16_t irq_mask) {
    uint8_t data[2] = {(uint8_t)(irq_mask >> 8), (uint8_t)(irq_mask & 0xFF)};
    return writeCommand(SX1262_CMD_CLEAR_IRQ_STATUS, data, 2);
}

int16_t LoRaDriver::getRSSI() {
    uint8_t data[3];
    readCommand(SX1262_CMD_GET_PACKET_STATUS, data, 3);
    return -((int16_t)data[0] / 2);
}

int8_t LoRaDriver::getSNR() {
    uint8_t data[3];
    readCommand(SX1262_CMD_GET_PACKET_STATUS, data, 3);
    return ((int8_t)data[1]) / 4;
}

bool LoRaDriver::isChannelActivityDetected() const {
    uint16_t irq_status = getIrqStatus();
    return (irq_status & SX1262_IRQ_CAD_DETECTED) != 0;
}

uint8_t LoRaDriver::getStatus() {
    uint8_t status;
    readCommand(SX1262_CMD_GET_STATUS, &status, 1);
    return status;
}

void LoRaDriver::setTxCallback(void (*callback)(bool success)) {
    tx_callback_ = callback;
}

void LoRaDriver::setRxCallback(void (*callback)(const uint8_t* data, uint8_t length, int16_t rssi, int8_t snr)) {
    rx_callback_ = callback;
}

void LoRaDriver::setCADCallback(void (*callback)(bool detected)) {
    cad_callback_ = callback;
}

void LoRaDriver::handleInterrupt() {
    if (!initialized_) {
        return;
    }
    
    uint16_t irq_status = getIrqStatus();
    
    if (irq_status & SX1262_IRQ_TX_DONE) {
        clearIrqStatus(SX1262_IRQ_TX_DONE);
        if (tx_callback_) {
            tx_callback_(true);
        }
    }
    
    if (irq_status & SX1262_IRQ_RX_DONE) {
        // Handle in receive function
    }
    
    if (irq_status & SX1262_IRQ_CAD_DONE) {
        clearIrqStatus(SX1262_IRQ_CAD_DONE);
        bool detected = (irq_status & SX1262_IRQ_CAD_DETECTED) != 0;
        if (cad_callback_) {
            cad_callback_(detected);
        }
    }
    
    if (irq_status & SX1262_IRQ_TIMEOUT) {
        clearIrqStatus(SX1262_IRQ_TIMEOUT);
        if (tx_callback_) {
            tx_callback_(false);
        }
    }
}