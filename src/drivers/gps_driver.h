/**
 * @file gps_driver.h
 * @brief GPS Driver Header for T-Deck-Pro GPS Module
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Defines GPS driver interface with NMEA parsing and position tracking
 */

#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <cstdint>

/**
 * @brief GPS fix types
 */
typedef enum {
    GPS_FIX_NONE = 0,       ///< No fix available
    GPS_FIX_2D = 2,         ///< 2D fix (lat/lon only)
    GPS_FIX_3D = 3          ///< 3D fix (lat/lon/altitude)
} GPSFixType;

/**
 * @brief GPS fix quality indicators
 */
typedef enum {
    GPS_FIX_INVALID = 0,    ///< Invalid fix
    GPS_FIX_GPS = 1,        ///< GPS fix
    GPS_FIX_DGPS = 2,       ///< Differential GPS fix
    GPS_FIX_PPS = 3,        ///< PPS fix
    GPS_FIX_RTK = 4,        ///< RTK fix
    GPS_FIX_FLOAT_RTK = 5,  ///< Float RTK fix
    GPS_FIX_ESTIMATED = 6  ///< Estimated fix
} GPSFixQuality;

/**
 * @brief GPS position structure
 */
struct GPSTime {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int millisecond;
};

#define GPS_MAX_SATELLITES 12
struct GPSSatellite {
    int id;
    int elevation;
    int azimuth;
    int snr;
    bool used;
};

struct GPSPosition {
    double latitude;
    double longitude;
    double altitude;
    double speed_knots;
    double speed_kmh;
    double course;
    uint8_t satellites;
    GPSFixQuality fix_quality;
    GPSFixType fix_type;
    float hdop;
    float vdop;
    float pdop;
    uint32_t timestamp;
    bool valid;
};

/**
 * @brief GPS Driver Class for T-Deck-Pro GPS Module
 * 
 * Provides comprehensive interface for GPS functionality including:
 * - NMEA sentence parsing
 * - Position and navigation data
 * - Satellite information
 * - Power management
 * - Configuration and calibration
 */
class GPSDriver {
private:
    // ===== CORE STATE =====
    bool initialized_;                    ///< Driver initialization state
    HardwareSerial* serial_;             ///< Serial interface
    uint32_t baud_rate_;                 ///< Serial baud rate
    uint8_t update_rate_;                ///< GPS update rate in Hz
    
    // ===== POSITION DATA =====
    GPSPosition position_;               ///< Current position data
    GPSTime time_;                       ///< Current time data
    GPSSatellite satellites_[GPS_MAX_SATELLITES]; ///< Satellite data
    uint8_t satellite_count_;            ///< Number of satellites used
    uint32_t last_fix_time_;             ///< Last valid fix timestamp
    uint32_t fix_timeout_;               ///< Fix timeout in milliseconds
    
    // ===== SATELLITE DATA =====
    uint8_t visible_satellites_;         ///< Number of visible satellites
    uint8_t used_satellites_;            ///< Number of satellites used in fix
    float satellite_snr_[GPS_MAX_SATELLITES];            ///< Satellite signal-to-noise ratios
    
    // ===== NMEA PARSING =====
    char nmea_buffer_[256];              ///< NMEA sentence buffer
    uint8_t buffer_index_;               ///< Buffer index
    uint32_t total_sentences_;           ///< Total sentences received
    uint32_t valid_sentences_;           ///< Valid sentences parsed
    uint32_t parse_errors_;              ///< Parse errors encountered
    uint32_t checksum_errors_;           ///< Checksum errors
    
    // ===== CALLBACKS =====
    void (*fix_callback_)(const GPSPosition&);      ///< Position fix callback
    void (*satellite_callback_)(uint8_t count);     ///< Satellite count callback
    void (*nmea_callback_)(const char* sentence);   ///< NMEA sentence callback
    
    // ===== POWER MANAGEMENT =====
    bool standby_mode_;                  ///< Standby mode state
    uint32_t last_activity_time_;        ///< Last activity timestamp
    
    // ===== PRIVATE METHODS =====
    bool parseNMEASentence(const char* sentence);
    bool parseGGA(const char* sentence);
    bool parseRMC(const char* sentence);
    bool parseGSV(const char* sentence);
    bool parseGSA(const char* sentence);
    bool verifyChecksum(const char* sentence);
    uint8_t calculateChecksum(const char* sentence);
    bool sendCommand(const char* command);
    int tokenize(const char* sentence, char** tokens, int max_tokens);
    bool parseGGA(char** tokens, uint8_t token_count);
    bool parseRMC(char** tokens, uint8_t token_count);
    bool parseGSV(char** tokens, uint8_t token_count);
    bool parseGSA(char** tokens, uint8_t token_count);

public:
    /**
     * @brief Constructor
     */
    GPSDriver();
    
    /**
     * @brief Destructor
     */
    ~GPSDriver();
    
    // ===== CORE INITIALIZATION =====
    
    /**
     * @brief Initialize the GPS driver
     * @param baud_rate Serial baud rate (default: 9600)
     * @return true if successful, false otherwise
     */
    bool initialize(uint32_t baud_rate = 9600);
    
    /**
     * @brief Deinitialize the GPS driver
     */
    void deinitialize();
    
    /**
     * @brief Check if driver is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    // ===== POSITION AND NAVIGATION =====
    
    /**
     * @brief Get current position
     * @param position Reference to position structure
     * @return true if valid position available
     */
    bool getPosition(GPSPosition& position);
    
    /**
     * @brief Get current time
     * @param time Reference to time structure
     * @return true if time data is valid
     */
    bool getTime(GPSTime& time);

    /**
     * @brief Get satellite data
     * @param satellites Array to fill with satellite data
     * @param max_satellites Maximum number of satellites to return
     * @return Number of satellites returned
     */
    uint8_t getSatellites(GPSSatellite* satellites, uint8_t max_satellites);
    
    /**
     * @brief Check if position fix is valid
     * @return true if valid fix
     */
    bool isFixValid() const;
    
    /**
     * @brief Get fix quality
     * @return Fix quality indicator
     */
    GPSFixQuality getFixQuality() const;
    
    /**
     * @brief Get number of satellites used
     * @return Satellite count
     */
    uint8_t getSatelliteCount() const { return position_.satellites; }
    
    /**
     * @brief Get horizontal dilution of precision
     * @return HDOP value
     */
    float getHDOP() const { return position_.hdop; }
    
    /**
     * @brief Get altitude
     * @return Altitude in meters
     */
    float getAltitude() const;
    
    /**
     * @brief Get speed
     * @return Speed in knots
     */
    double getSpeed() const { return position_.speed_knots; }
    
    /**
     * @brief Get course
     * @return Course in degrees
     */
    double getCourse() const { return position_.course; }

    /**
     * @brief Get fix age in milliseconds
     * @return Fix age in milliseconds
     */
    uint32_t getFixAge() const;
    
    // ===== CONFIGURATION =====
    
    /**
     * @brief Set GPS update rate
     * @param rate_hz Update rate in Hz (1, 5, 10)
     * @return true if successful
     */
    bool setUpdateRate(uint8_t rate_hz);
    
    /**
     * @brief Set serial baud rate
     * @param baud_rate Baud rate
     * @return true if successful
     */
    bool setBaudRate(uint32_t baud_rate);
    
    /**
     * @brief Get current baud rate
     * @return Baud rate
     */
    uint32_t getBaudRate() const { return baud_rate_; }
    
    /**
     * @brief Get current update rate
     * @return Update rate in Hz
     */
    uint8_t getUpdateRate() const { return update_rate_; }
    
    // ===== POWER MANAGEMENT =====
    
    /**
     * @brief Enter standby mode
     * @return true if successful
     */
    bool enterStandby();
    
    /**
     * @brief Wake up from standby
     * @return true if successful
     */
    bool wakeup();
    
    /**
     * @brief Check if in standby mode
     * @return true if in standby
     */
    bool isStandby() const { return standby_mode_; }
    
    // ===== CALLBACKS =====
    
    /**
     * @brief Set position fix callback
     * @param callback Function to call when position updated
     */
    void setFixCallback(void (*callback)(const GPSPosition&));
    
    /**
     * @brief Set satellite count callback
     * @param callback Function to call when satellite count changes
     */
    void setSatelliteCallback(void (*callback)(uint8_t count));
    
    /**
     * @brief Set NMEA sentence callback
     * @param callback Function to call when NMEA sentence received
     */
    void setNMEACallback(void (*callback)(const char* sentence));
    
    // ===== STATISTICS =====
    
    /**
     * @brief Get total sentences received
     * @return Total sentence count
     */
    uint32_t getTotalSentences() const;
    
    /**
     * @brief Get valid sentences parsed
     * @return Valid sentence count
     */
    uint32_t getValidSentences() const;
    
    /**
     * @brief Get parse errors
     * @return Parse error count
     */
    uint32_t getParseErrors() const;
    
    /**
     * @brief Get parse success rate
     * @return Success rate as percentage
     */
    float getParseSuccessRate() const;
    
    // ===== LOW-LEVEL COMMUNICATION =====
    
    /**
     * @brief Update GPS data (call regularly)
     * @return void
     */
    void update();
    
    /**
     * @brief Process incoming serial data
     */
    void processSerialData();
};

#endif // GPS_DRIVER_H