/**
 * @file lora_driver.h
 * @brief LoRa Driver Header for SX1262 on T-Deck-Pro
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Defines SX1262 LoRa transceiver driver interface
 */

#ifndef LORA_DRIVER_H
#define LORA_DRIVER_H

#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

// ===== ENUMS FROM IMPLEMENTATION =====
enum LoRaPacketType {
    LORA_PACKET_TYPE_GFSK = 0x00,
    LORA_PACKET_TYPE_LORA = 0x01,
    LORA_PACKET_TYPE_LR_FHSS = 0x02
};

enum LoRaBandwidth {
    LORA_BW_7_8 = 0x00,
    LORA_BW_10_4 = 0x01,
    LORA_BW_15_6 = 0x02,
    LORA_BW_20_8 = 0x03,
    LORA_BW_31_25 = 0x04,
    LORA_BW_41_7 = 0x05,
    LORA_BW_62_5 = 0x06,
    LORA_BW_125 = 0x07,
    LORA_BW_250 = 0x08,
    LORA_BW_500 = 0x09
};

enum LoRaCodingRate {
    LORA_CR_4_5 = 0x01,
    LORA_CR_4_6 = 0x02,
    LORA_CR_4_7 = 0x03,
    LORA_CR_4_8 = 0x04
};

enum LoRaPARamp {
    LORA_PA_RAMP_10U = 0x00,
    LORA_PA_RAMP_20U = 0x01,
    LORA_PA_RAMP_40U = 0x02,
    LORA_PA_RAMP_80U = 0x03,
    LORA_PA_RAMP_160U = 0x04,
    LORA_PA_RAMP_200U = 0x05,
    LORA_PA_RAMP_400U = 0x06,
    LORA_PA_RAMP_800U = 0x07
};

enum LoRaRegulatorMode {
    LORA_REGULATOR_LDO = 0x00,
    LORA_REGULATOR_DC_DC = 0x01
};

enum LoRaStandbyMode {
    LORA_STANDBY_RC = 0x00,
    LORA_STANDBY_XOSC = 0x01
};

enum LoRaSleepConfig {
    LORA_SLEEP_COLD_START = 0x00,
    LORA_SLEEP_WARM_START = 0x04
};

// Callback types
typedef void (*LoRaTxCallback)(bool success);
typedef void (*LoRaRxCallback)(const uint8_t* data, uint8_t length, int16_t rssi, int8_t snr);
typedef void (*LoRaCADCallback)(bool detected);

class LoRaDriver {
private:
    bool initialized_;
    uint32_t frequency_;
    uint8_t spreading_factor_;
    LoRaBandwidth bandwidth_;
    LoRaCodingRate coding_rate_;
    int8_t power_;
    uint16_t sync_word_;
    bool implicit_header_;
    uint8_t packet_length_;
    
    // Callbacks
    LoRaTxCallback tx_callback_;
    LoRaRxCallback rx_callback_;
    LoRaCADCallback cad_callback_;

public:
    LoRaDriver();
    ~LoRaDriver();
    
    // ===== INITIALIZATION =====
    bool initialize();
    void deinitialize();
    bool isInitialized() const;
    bool reset();
    void waitOnBusy() const;
    
    // ===== SPI COMMUNICATION =====
    bool writeCommand(uint8_t command, const uint8_t* data, uint8_t length);
    bool readCommand(uint8_t command, uint8_t* data, uint8_t length) const;
    uint8_t readRegister(uint16_t address);
    bool writeRegister(uint16_t address, uint8_t value);
    
    // ===== CONFIGURATION =====
    bool setPacketType(LoRaPacketType type);
    bool setFrequency(uint32_t frequency);
    bool setModulationParams(uint8_t sf, LoRaBandwidth bw, LoRaCodingRate cr);
    bool setPacketParams(uint8_t preamble_length, bool implicit_header, 
                        uint8_t payload_length, bool crc_on, bool iq_inverted);
    bool setPAConfig(uint8_t pa_duty_cycle, uint8_t hp_max, uint8_t device_sel, uint8_t pa_lut);
    bool setTxParams(int8_t power, LoRaPARamp ramp_time);
    bool setDioIrqParams(uint16_t irq_mask, uint16_t dio1_mask, uint16_t dio2_mask, uint16_t dio3_mask);
    bool setRegulatorMode(LoRaRegulatorMode mode);
    bool calibrate(uint8_t calibrate_param);
    bool setStandby(LoRaStandbyMode mode);
    bool setSleep(LoRaSleepConfig config);
    
    // ===== TRANSMISSION =====
    bool transmit(const uint8_t* data, uint8_t length, uint32_t timeout_ms);
    bool receive(uint8_t* buffer, uint8_t max_length, uint8_t* received_length, uint32_t timeout_ms);
    bool startCAD(uint32_t timeout_ms);
    bool isChannelActivityDetected() const;
    
    // ===== STATUS =====
    uint8_t getStatus();
    int16_t getRSSI();
    int8_t getSNR();
    uint16_t getIrqStatus() const;
    bool clearIrqStatus(uint16_t irq_mask);
    
    // ===== CALLBACKS =====
    void setTxCallback(LoRaTxCallback callback);
    void setRxCallback(LoRaRxCallback callback);
    void setCADCallback(LoRaCADCallback callback);
    void handleInterrupt();
    
    // ===== GETTERS/SETTERS =====
    /**
     * @brief Get the current LoRa frequency in Hz
     */
    uint32_t getFrequency() const { return frequency_; }
    /**
     * @brief Get the LoRa spreading factor (SF)
     */
    uint8_t getSpreadingFactor() const { return spreading_factor_; }
    /**
     * @brief Set the LoRa spreading factor (SF)
     */
    void setSpreadingFactor(uint8_t sf) { spreading_factor_ = sf; }
    /**
     * @brief Get the LoRa bandwidth setting
     */
    LoRaBandwidth getBandwidth() const { return bandwidth_; }
    /**
     * @brief Set the LoRa bandwidth setting
     */
    void setBandwidth(LoRaBandwidth bw) { bandwidth_ = bw; }
    /**
     * @brief Get the LoRa coding rate
     */
    LoRaCodingRate getCodingRate() const { return coding_rate_; }
    /**
     * @brief Set the LoRa coding rate
     */
    void setCodingRate(LoRaCodingRate cr) { coding_rate_ = cr; }
    /**
     * @brief Get the LoRa TX power in dBm
     */
    int8_t getPower() const { return power_; }
    /**
     * @brief Set the LoRa TX power in dBm
     */
    void setPower(int8_t power) { power_ = power; }
    /**
     * @brief Get the LoRa sync word
     */
    uint16_t getSyncWord() const { return sync_word_; }
    /**
     * @brief Set the LoRa sync word
     */
    void setSyncWord(uint16_t sync_word) { sync_word_ = sync_word; }
};

#endif // LORA_DRIVER_H