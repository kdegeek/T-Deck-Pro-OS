/**
 * @file gps_driver.cpp
 * @brief GPS Driver Implementation for T-Deck-Pro
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Implements GPS driver with NMEA parsing and position tracking
 */

#include "gps_driver.h"
#include "../config/os_config_corrected.h"
#include <Arduino.h>
#include <HardwareSerial.h>

// NMEA sentence types
#define NMEA_GGA "$GPGGA"
#define NMEA_RMC "$GPRMC"
#define NMEA_GSV "$GPGSV"
#define NMEA_GSA "$GPGSA"
#define NMEA_GLL "$GPGLL"
#define NMEA_VTG "$GPVTG"

// GPS commands
#define GPS_CMD_RESET "$PMTK104*37\r\n"
#define GPS_CMD_SET_BAUD_9600 "$PMTK251,9600*17\r\n"
#define GPS_CMD_SET_BAUD_38400 "$PMTK251,38400*27\r\n"
#define GPS_CMD_SET_BAUD_57600 "$PMTK251,57600*2C\r\n"
#define GPS_CMD_SET_BAUD_115200 "$PMTK251,115200*1F\r\n"
#define GPS_CMD_SET_UPDATE_1HZ "$PMTK220,1000*1F\r\n"
#define GPS_CMD_SET_UPDATE_5HZ "$PMTK220,200*2C\r\n"
#define GPS_CMD_SET_UPDATE_10HZ "$PMTK220,100*2F\r\n"
#define GPS_CMD_SET_NMEA_OUTPUT "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n"
#define GPS_CMD_STANDBY "$PMTK161,0*28\r\n"
#define GPS_CMD_WAKEUP "$PMTK010,001*2E\r\n"

// Static instance for interrupt handling
static GPSDriver* gps_instance = nullptr;

GPSDriver::GPSDriver() :
    initialized_(false),
    serial_(nullptr),
    baud_rate_(9600),
    update_rate_(1),
    fix_callback_(nullptr),
    satellite_callback_(nullptr),
    nmea_callback_(nullptr) {
    
    // Initialize position data
    memset(&position_, 0, sizeof(position_));
    position_.fix_quality = GPS_FIX_INVALID;
    
    // Initialize time data
    memset(&time_, 0, sizeof(time_));
    
    // Initialize satellite data
    memset(satellites_, 0, sizeof(satellites_));
    satellite_count_ = 0;
    
    // Initialize statistics
    total_sentences_ = 0;
    valid_sentences_ = 0;
    parse_errors_ = 0;
    
    // Clear buffers
    memset(nmea_buffer_, 0, sizeof(nmea_buffer_));
    buffer_index_ = 0;
    
    gps_instance = this;
}

GPSDriver::~GPSDriver() {
    deinitialize();
    gps_instance = nullptr;
}

bool GPSDriver::initialize(uint32_t baud_rate) {
    if (initialized_) {
        return true;
    }
    
    baud_rate_ = baud_rate;
    
    // Initialize serial port
    serial_ = &Serial1;
    serial_->begin(baud_rate_, SERIAL_8N1, T_DECK_GPS_RX, T_DECK_GPS_TX);
    
    delay(100);
    
    // Send configuration commands
    sendCommand(GPS_CMD_RESET);
    delay(1000);
    
    sendCommand(GPS_CMD_SET_NMEA_OUTPUT);
    delay(100);
    
    sendCommand(GPS_CMD_SET_UPDATE_1HZ);
    delay(100);
    
    initialized_ = true;
    
    Serial.println("[GPS] Initialization successful");
    return true;
}

void GPSDriver::deinitialize() {
    if (!initialized_) {
        return;
    }
    
    if (serial_) {
        serial_->end();
        serial_ = nullptr;
    }
    
    initialized_ = false;
    Serial.println("[GPS] Deinitialized");
}

bool GPSDriver::isInitialized() const {
    return initialized_;
}

void GPSDriver::update() {
    if (!initialized_ || !serial_) {
        return;
    }
    
    // Read available data
    while (serial_->available()) {
        char c = serial_->read();
        
        if (c == '\n' || c == '\r') {
            if (buffer_index_ > 0) {
                nmea_buffer_[buffer_index_] = '\0';
                
                // Parse the sentence
                if (parsNMEASentence(nmea_buffer_)) {
                    valid_sentences_++;
                    
                    // Call NMEA callback if set
                    if (nmea_callback_) {
                        nmea_callback_(nmea_buffer_);
                    }
                } else {
                    parse_errors_++;
                }
                
                total_sentences_++;
                buffer_index_ = 0;
            }
        } else if (buffer_index_ < sizeof(nmea_buffer_) - 1) {
            nmea_buffer_[buffer_index_++] = c;
        } else {
            // Buffer overflow, reset
            buffer_index_ = 0;
            parse_errors_++;
        }
    }
}

bool GPSDriver::getPosition(GPSPosition& position) {
    if (!initialized_) {
        return false;
    }
    
    position = position_;
    return position_.fix_quality != GPS_FIX_INVALID;
}

bool GPSDriver::getTime(GPSTime& time) {
    if (!initialized_) {
        return false;
    }
    
    time = time_;
    return time_.year > 0;
}

uint8_t GPSDriver::getSatellites(GPSSatellite* satellites, uint8_t max_satellites) {
    if (!initialized_ || !satellites) {
        return 0;
    }
    
    uint8_t count = (satellite_count_ < max_satellites) ? satellite_count_ : max_satellites;
    memcpy(satellites, satellites_, count * sizeof(GPSSatellite));
    
    return count;
}

bool GPSDriver::isFixValid() const {
    return position_.fix_quality != GPS_FIX_INVALID && position_.fix_quality != GPS_FIX_NONE;
}

GPS_FixQuality GPSDriver::getFixQuality() const {
    return position_.fix_quality;
}

uint8_t GPSDriver::getSatelliteCount() const {
    return position_.satellites;
}

float GPSDriver::getHDOP() const {
    return position_.hdop;
}

float GPSDriver::getAltitude() const {
    return position_.altitude;
}

uint32_t GPSDriver::getFixAge() const {
    if (position_.timestamp == 0) {
        return UINT32_MAX;
    }
    
    return millis() - position_.timestamp;
}

bool GPSDriver::setUpdateRate(uint8_t rate_hz) {
    if (!initialized_ || rate_hz == 0 || rate_hz > 10) {
        return false;
    }
    
    update_rate_ = rate_hz;
    
    switch (rate_hz) {
        case 1:
            sendCommand(GPS_CMD_SET_UPDATE_1HZ);
            break;
        case 5:
            sendCommand(GPS_CMD_SET_UPDATE_5HZ);
            break;
        case 10:
            sendCommand(GPS_CMD_SET_UPDATE_10HZ);
            break;
        default: {
            // Custom rate
            char cmd[64];
            uint16_t interval = 1000 / rate_hz;
            uint8_t checksum = calculateChecksum(cmd);
            snprintf(cmd, sizeof(cmd), "$PMTK220,%d*%02X\r\n", interval, checksum);
            sendCommand(cmd);
            break;
        }
    }
    
    delay(100);
    return true;
}

bool GPSDriver::setBaudRate(uint32_t baud_rate) {
    if (!initialized_) {
        return false;
    }
    
    const char* cmd = nullptr;
    
    switch (baud_rate) {
        case 9600:
            cmd = GPS_CMD_SET_BAUD_9600;
            break;
        case 38400:
            cmd = GPS_CMD_SET_BAUD_38400;
            break;
        case 57600:
            cmd = GPS_CMD_SET_BAUD_57600;
            break;
        case 115200:
            cmd = GPS_CMD_SET_BAUD_115200;
            break;
        default:
            return false;
    }
    
    sendCommand(cmd);
    delay(100);
    
    // Reinitialize serial with new baud rate
    serial_->end();
    delay(100);
    serial_->begin(baud_rate, SERIAL_8N1, T_DECK_GPS_RX, T_DECK_GPS_TX);
    
    baud_rate_ = baud_rate;
    return true;
}

bool GPSDriver::enterStandby() {
    if (!initialized_) {
        return false;
    }
    
    sendCommand(GPS_CMD_STANDBY);
    delay(100);
    return true;
}

bool GPSDriver::wakeup() {
    if (!initialized_) {
        return false;
    }
    
    sendCommand(GPS_CMD_WAKEUP);
    delay(100);
    return true;
}

void GPSDriver::setFixCallback(void (*callback)(const GPSPosition&)) {
    fix_callback_ = callback;
}

void GPSDriver::setSatelliteCallback(void (*callback)(uint8_t count)) {
    satellite_callback_ = callback;
}

void GPSDriver::setNMEACallback(void (*callback)(const char* sentence)) {
    nmea_callback_ = callback;
}

uint32_t GPSDriver::getTotalSentences() const {
    return total_sentences_;
}

uint32_t GPSDriver::getValidSentences() const {
    return valid_sentences_;
}

uint32_t GPSDriver::getParseErrors() const {
    return parse_errors_;
}

float GPSDriver::getParseSuccessRate() const {
    if (total_sentences_ == 0) {
        return 0.0f;
    }
    
    return (float)valid_sentences_ / (float)total_sentences_ * 100.0f;
}

bool GPSDriver::sendCommand(const char* command) {
    if (!initialized_ || !serial_ || !command) {
        return false;
    }
    
    serial_->print(command);
    return true;
}

bool GPSDriver::parsNMEASentence(const char* sentence) {
    if (!sentence || sentence[0] != '$') {
        return false;
    }
    
    // Verify checksum
    if (!verifyChecksum(sentence)) {
        return false;
    }
    
    // Parse different sentence types
    if (strncmp(sentence, NMEA_GGA, 6) == 0) {
        return parseGGA(sentence);
    } else if (strncmp(sentence, NMEA_RMC, 6) == 0) {
        return parseRMC(sentence);
    } else if (strncmp(sentence, NMEA_GSV, 6) == 0) {
        return parseGSV(sentence);
    } else if (strncmp(sentence, NMEA_GSA, 6) == 0) {
        return parseGSA(sentence);
    }
    
    return true; // Unknown sentence type but valid format
}

bool GPSDriver::parseGGA(const char* sentence) {
    // $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
    
    char* tokens[15];
    int token_count = tokenize(sentence, tokens, 15);
    
    if (token_count < 14) {
        return false;
    }
    
    // Parse time
    if (strlen(tokens[1]) >= 6) {
        time_.hour = atoi(String(tokens[1]).substring(0, 2).c_str());
        time_.minute = atoi(String(tokens[1]).substring(2, 4).c_str());
        time_.second = atoi(String(tokens[1]).substring(4, 6).c_str());
        if (strlen(tokens[1]) > 6) {
            time_.millisecond = atoi(String(tokens[1]).substring(7).c_str());
        }
    }
    
    // Parse latitude
    if (strlen(tokens[2]) > 0 && strlen(tokens[3]) > 0) {
        float lat_deg = atof(String(tokens[2]).substring(0, 2).c_str());
        float lat_min = atof(String(tokens[2]).substring(2).c_str());
        position_.latitude = lat_deg + lat_min / 60.0f;
        if (tokens[3][0] == 'S') {
            position_.latitude = -position_.latitude;
        }
    }
    
    // Parse longitude
    if (strlen(tokens[4]) > 0 && strlen(tokens[5]) > 0) {
        float lon_deg = atof(String(tokens[4]).substring(0, 3).c_str());
        float lon_min = atof(String(tokens[4]).substring(3).c_str());
        position_.longitude = lon_deg + lon_min / 60.0f;
        if (tokens[5][0] == 'W') {
            position_.longitude = -position_.longitude;
        }
    }
    
    // Parse fix quality
    if (strlen(tokens[6]) > 0) {
        int quality = atoi(tokens[6]);
        switch (quality) {
            case 0: position_.fix_quality = GPS_FIX_INVALID; break;
            case 1: position_.fix_quality = GPS_FIX_GPS; break;
            case 2: position_.fix_quality = GPS_FIX_DGPS; break;
            case 3: position_.fix_quality = GPS_FIX_PPS; break;
            case 4: position_.fix_quality = GPS_FIX_RTK; break;
            case 5: position_.fix_quality = GPS_FIX_FLOAT_RTK; break;
            case 6: position_.fix_quality = GPS_FIX_ESTIMATED; break;
            default: position_.fix_quality = GPS_FIX_INVALID; break;
        }
    }
    
    // Parse satellite count
    if (strlen(tokens[7]) > 0) {
        position_.satellites = atoi(tokens[7]);
    }
    
    // Parse HDOP
    if (strlen(tokens[8]) > 0) {
        position_.hdop = atof(tokens[8]);
    }
    
    // Parse altitude
    if (strlen(tokens[9]) > 0) {
        position_.altitude = atof(tokens[9]);
    }
    
    position_.timestamp = millis();
    position_.valid = (position_.fix_quality != GPS_FIX_INVALID);
    
    // Call fix callback if set
    if (fix_callback_ && position_.valid) {
        fix_callback_(position_);
    }
    
    return true;
}

bool GPSDriver::parseRMC(const char* sentence) {
    // $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
    
    char* tokens[12];
    int token_count = tokenize(sentence, tokens, 12);
    
    if (token_count < 11) {
        return false;
    }
    
    // Parse status
    if (strlen(tokens[2]) > 0 && tokens[2][0] != 'A') {
        return true; // Invalid fix
    }
    
    // Parse speed
    if (strlen(tokens[7]) > 0) {
        position_.speed_knots = atof(tokens[7]);
        position_.speed_kmh = position_.speed_knots * 1.852f;
    }
    
    // Parse course
    if (strlen(tokens[8]) > 0) {
        position_.course = atof(tokens[8]);
    }
    
    // Parse date
    if (strlen(tokens[9]) >= 6) {
        time_.day = atoi(String(tokens[9]).substring(0, 2).c_str());
        time_.month = atoi(String(tokens[9]).substring(2, 4).c_str());
        time_.year = 2000 + atoi(String(tokens[9]).substring(4, 6).c_str());
    }
    
    return true;
}

bool GPSDriver::parseGSV(const char* sentence) {
    // $GPGSV,2,1,08,01,40,083,46,02,17,308,41,12,07,344,39,14,22,228,45*75
    
    char* tokens[20];
    int token_count = tokenize(sentence, tokens, 20);
    
    if (token_count < 4) {
        return false;
    }
    
    int total_messages = atoi(tokens[1]);
    int message_number = atoi(tokens[2]);
    int total_satellites = atoi(tokens[3]);
    
    // Parse satellite info (up to 4 satellites per message)
    int sat_index = (message_number - 1) * 4;
    
    for (int i = 0; i < 4 && (4 + i * 4 + 3) < token_count && sat_index < GPS_MAX_SATELLITES; i++) {
        if (strlen(tokens[4 + i * 4]) > 0) {
            satellites_[sat_index].id = atoi(tokens[4 + i * 4]);
            satellites_[sat_index].elevation = atoi(tokens[4 + i * 4 + 1]);
            satellites_[sat_index].azimuth = atoi(tokens[4 + i * 4 + 2]);
            satellites_[sat_index].snr = atoi(tokens[4 + i * 4 + 3]);
            satellites_[sat_index].used = (satellites_[sat_index].snr > 0);
            sat_index++;
        }
    }
    
    if (message_number == total_messages) {
        satellite_count_ = (sat_index < total_satellites) ? sat_index : total_satellites;
        
        // Call satellite callback if set
        if (satellite_callback_) {
            satellite_callback_(satellite_count_);
        }
    }
    
    return true;
}

bool GPSDriver::parseGSA(const char* sentence) {
    // $GPGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.0,1.0,1.0*30
    
    char* tokens[18];
    int token_count = tokenize(sentence, tokens, 18);
    
    if (token_count < 17) {
        return false;
    }
    
    // Parse fix type
    if (strlen(tokens[2]) > 0) {
        int fix_type = atoi(tokens[2]);
        switch (fix_type) {
            case 1: position_.fix_quality = GPS_FIX_NONE; break;
            case 2: position_.fix_quality = GPS_FIX_2D; break;
            case 3: position_.fix_quality = GPS_FIX_3D; break;
        }
    }
    
    // Parse DOP values
    if (strlen(tokens[15]) > 0) {
        position_.pdop = atof(tokens[15]);
    }
    if (strlen(tokens[16]) > 0) {
        position_.hdop = atof(tokens[16]);
    }
    if (strlen(tokens[17]) > 0) {
        position_.vdop = atof(tokens[17]);
    }
    
    return true;
}

int GPSDriver::tokenize(const char* sentence, char** tokens, int max_tokens) {
    static char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    int count = 0;
    char* token = strtok(buffer, ",*");
    
    while (token && count < max_tokens) {
        tokens[count++] = token;
        token = strtok(nullptr, ",*");
    }
    
    return count;
}

bool GPSDriver::verifyChecksum(const char* sentence) {
    if (!sentence) {
        return false;
    }
    
    const char* checksum_pos = strchr(sentence, '*');
    if (!checksum_pos) {
        return false; // No checksum
    }
    
    // Calculate checksum
    uint8_t calculated = 0;
    for (const char* p = sentence + 1; p < checksum_pos; p++) {
        calculated ^= *p;
    }
    
    // Parse provided checksum
    uint8_t provided = strtol(checksum_pos + 1, nullptr, 16);
    
    return calculated == provided;
}

uint8_t GPSDriver::calculateChecksum(const char* sentence) {
    if (!sentence || sentence[0] != '$') {
        return 0;
    }
    
    uint8_t checksum = 0;
    for (const char* p = sentence + 1; *p && *p != '*'; p++) {
        checksum ^= *p;
    }
    
    return checksum;
}