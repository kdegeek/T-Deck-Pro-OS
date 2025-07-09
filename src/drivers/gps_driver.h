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
    GPS_QUALITY_INVALID = 0,    ///< Invalid fix
    GPS_QUALITY_GPS = 1,        ///< GPS fix
    GPS_QUALITY_DGPS = 2,       ///< Differential GPS fix
    GPS_QUALITY_PPS = 3,        ///< PPS fix
    GPS_QUALITY_RTK = 4,        ///< Real Time Kinematic
    GPS_QUALITY_FLOAT_RTK = 5,  ///< Float RTK
    GPS_QUALITY_ESTIMATED = 6,  ///< Estimated/dead reckoning
    GPS_QUALITY_MANUAL = 7,     ///< Manual input mode
    GPS_QUALITY_SIMULATION = 8  ///< Simulation mode
} GPSQuality;

/**
 * @brief GPS position data structure
 */
struct GPSPosition {
    double latitude;        ///< Latitude in decimal degrees
    double longitude;       ///< Longitude in decimal degrees
    double altitude;        ///< Altitude in meters above sea level
    float speed;            ///< Speed in km/h
    float course;           ///< Course over ground in degrees
    GPSFixType fix_type;    ///< Type of GPS fix
    GPSQuality quality;     ///< Fix quality indicator
    uint8_t satellites;     ///< Number of satellites in use
    float hdop;             ///< Horizontal dilution of precision
    float vdop;             ///< Vertical dilution of precision
    float pdop;             ///< Position dilution of precision
    uint32_t timestamp;     ///< GPS timestamp (seconds since epoch)
    bool valid;             ///< True if position data is valid
};

/**
 * @brief GPS time structure
 */
struct GPSTime {
    uint16_t year;          ///< Year (e.g., 2025)
    uint8_t month;          ///< Month (1-12)
    uint8_t day;            ///< Day (1-31)
    uint8_t hour;           ///< Hour (0-23)
    uint8_t minute;         ///< Minute (0-59)
    uint8_t second;         ///< Second (0-59)
    uint16_t millisecond;   ///< Millisecond (0-999)
    bool valid;             ///< True if time data is valid
};

/**
 * @brief GPS satellite information
 */
struct GPSSatellite {
    uint8_t id;             ///< Satellite ID
    uint8_t elevation;      ///< Elevation angle in degrees
    uint16_t azimuth;       ///< Azimuth angle in degrees
    uint8_t snr;            ///< Signal-to-noise ratio in dB
    bool used_in_fix;       ///< True if satellite used in position fix
};

/**
 * @brief GPS Driver Class
 * 
 * Provides comprehensive GPS functionality including:
 * - NMEA sentence parsing
 * - Position and time extraction
 * - Satellite tracking
 * - Fix quality monitoring
 * - Power management
 */
class GPSDriver {
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
     * @param baud_rate Serial baud rate (default 9600)
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
    
    // ===== POWER MANAGEMENT =====
    
    /**
     * @brief Enable GPS module power
     * @return true if successful
     */
    bool powerOn();
    
    /**
     * @brief Disable GPS module power
     * @return true if successful
     */
    bool powerOff();
    
    /**
     * @brief Check if GPS module is powered
     * @return true if powered on
     */
    bool isPowered() const { return powered_; }
    
    // ===== DATA ACQUISITION =====
    
    /**
     * @brief Update GPS data by processing incoming NMEA sentences
     * @return true if new data processed
     */
    bool update();
    
    /**
     * @brief Get current position data
     * @param position Reference to position structure to fill
     * @return true if valid position available
     */
    bool getPosition(GPSPosition& position);
    
    /**
     * @brief Get current GPS time
     * @param time Reference to time structure to fill
     * @return true if valid time available
     */
    bool getTime(GPSTime& time);
    
    /**
     * @brief Get satellite information
     * @param satellites Array to store satellite data
     * @param max_satellites Maximum number of satellites to store
     * @return Number of satellites stored
     */
    uint8_t getSatellites(GPSSatellite* satellites, uint8_t max_satellites);
    
    // ===== STATUS QUERIES =====
    
    /**
     * @brief Check if GPS has a valid fix
     * @return true if fix available
     */
    bool hasFix() const { return position_.valid && position_.fix_type >= GPS_FIX_2D; }
    
    /**
     * @brief Get fix type
     * @return Current fix type
     */
    GPSFixType getFixType() const { return position_.fix_type; }
    
    /**
     * @brief Get number of satellites in use
     * @return Satellite count
     */
    uint8_t getSatelliteCount() const { return position_.satellites; }
    
    /**
     * @brief Get horizontal dilution of precision
     * @return HDOP value
     */
    float getHDOP() const { return position_.hdop; }
    
    /**
     * @brief Get age of last fix in milliseconds
     * @return Age in milliseconds
     */
    uint32_t getFixAge() const;
    
    // ===== CONFIGURATION =====
    
    /**
     * @brief Set GPS update rate
     * @param rate_hz Update rate in Hz (1-10)
     * @return true if successful
     */
    bool setUpdateRate(uint8_t rate_hz);
    
    /**
     * @brief Enable/disable specific NMEA sentences
     * @param sentence NMEA sentence type (e.g., "GGA", "RMC")
     * @param enable true to enable, false to disable
     * @return true if successful
     */
    bool enableNMEASentence(const char* sentence, bool enable);
    
    /**
     * @brief Set navigation mode
     * @param mode Navigation mode (0=pedestrian, 1=automotive, 2=sea, 3=airborne)
     * @return true if successful
     */
    bool setNavigationMode(uint8_t mode);
    
    // ===== UTILITIES =====
    
    /**
     * @brief Calculate distance between two points
     * @param lat1 Latitude of first point
     * @param lon1 Longitude of first point
     * @param lat2 Latitude of second point
     * @param lon2 Longitude of second point
     * @return Distance in meters
     */
    static double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    
    /**
     * @brief Calculate bearing between two points
     * @param lat1 Latitude of first point
     * @param lon1 Longitude of first point
     * @param lat2 Latitude of second point
     * @param lon2 Longitude of second point
     * @return Bearing in degrees
     */
    static double calculateBearing(double lat1, double lon1, double lat2, double lon2);
    
    /**
     * @brief Convert coordinates to different formats
     * @param decimal_degrees Input in decimal degrees
     * @param degrees Output degrees
     * @param minutes Output minutes
     * @param seconds Output seconds
     */
    static void convertToDMS(double decimal_degrees, int& degrees, int& minutes, float& seconds);
    
    // ===== CALLBACKS =====
    
    /**
     * @brief Set callback for position updates
     * @param callback Function to call when position updated
     */
    void setPositionCallback(void (*callback)(const GPSPosition&));
    
    /**
     * @brief Set callback for time updates
     * @param callback Function to call when time updated
     */
    void setTimeCallback(void (*callback)(const GPSTime&));

private:
    // ===== INTERNAL METHODS =====
    
    /**
     * @brief Initialize hardware pins
     * @return true if successful
     */
    bool initializeHardware();
    
    /**
     * @brief Initialize serial communication
     * @param baud_rate Serial baud rate
     * @return true if successful
     */
    bool initializeSerial(uint32_t baud_rate);
    
    /**
     * @brief Process incoming NMEA sentence
     * @param sentence NMEA sentence string
     * @return true if sentence processed successfully
     */
    bool processNMEASentence(const char* sentence);
    
    /**
     * @brief Parse GGA sentence (Global Positioning System Fix Data)
     * @param tokens Array of sentence tokens
     * @param token_count Number of tokens
     * @return true if parsed successfully
     */
    bool parseGGA(char** tokens, uint8_t token_count);
    
    /**
     * @brief Parse RMC sentence (Recommended Minimum)
     * @param tokens Array of sentence tokens
     * @param token_count Number of tokens
     * @return true if parsed successfully
     */
    bool parseRMC(char** tokens, uint8_t token_count);
    
    /**
     * @brief Parse GSV sentence (Satellites in View)
     * @param tokens Array of sentence tokens
     * @param token_count Number of tokens
     * @return true if parsed successfully
     */
    bool parseGSV(char** tokens, uint8_t token_count);
    
    /**
     * @brief Parse GSA sentence (GPS DOP and active satellites)
     * @param tokens Array of sentence tokens
     * @param token_count Number of tokens
     * @return true if parsed successfully
     */
    bool parseGSA(char** tokens, uint8_t token_count);
    
    /**
     * @brief Validate NMEA sentence checksum
     * @param sentence NMEA sentence
     * @return true if checksum valid
     */
    bool validateChecksum(const char* sentence);
    
    /**
     * @brief Convert latitude/longitude from NMEA format
     * @param coord_str Coordinate string from NMEA
     * @param direction Direction character (N/S/E/W)
     * @return Coordinate in decimal degrees
     */
    double convertCoordinate(const char* coord_str, char direction);
    
    /**
     * @brief Convert time from NMEA format
     * @param time_str Time string from NMEA (HHMMSS.SSS)
     * @param time_struct Time structure to fill
     */
    void convertTime(const char* time_str, GPSTime& time_struct);
    
    /**
     * @brief Convert date from NMEA format
     * @param date_str Date string from NMEA (DDMMYY)
     * @param time_struct Time structure to fill
     */
    void convertDate(const char* date_str, GPSTime& time_struct);
    
    // ===== MEMBER VARIABLES =====
    
    bool initialized_;                    ///< Driver initialization state
    bool powered_;                        ///< GPS module power state
    HardwareSerial* serial_;              ///< Serial interface for GPS communication
    uint32_t baud_rate_;                  ///< Serial baud rate
    
    // Current GPS data
    GPSPosition position_;                ///< Current position data
    GPSTime time_;                        ///< Current time data
    GPSSatellite satellites_[12];         ///< Satellite information array
    uint8_t satellite_count_;             ///< Number of satellites tracked
    
    // NMEA parsing
    char nmea_buffer_[128];               ///< Buffer for incoming NMEA data
    uint8_t buffer_index_;                ///< Current buffer position
    uint32_t last_fix_time_;              ///< Timestamp of last position fix
    
    // Statistics
    uint32_t sentence_count_;             ///< Total NMEA sentences processed
    uint32_t valid_sentences_;            ///< Valid NMEA sentences processed
    uint32_t checksum_errors_;            ///< Checksum error count
    
    // Callbacks
    void (*position_callback_)(const GPSPosition&); ///< Position update callback
    void (*time_callback_)(const GPSTime&);         ///< Time update callback
};

#endif // GPS_DRIVER_H