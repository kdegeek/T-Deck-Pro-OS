
#include "utilities.h"
#include "peripheral.h"
#include <TinyGPS++.h>

/* clang-format off */

TinyGPSPlus gps;
static bool GPS_Recovery();
bool setupGPS();
void displayInfo();

static TaskHandle_t gps_handle;
static double gps_lat=0, gps_lng=0, gps_altitude=0, gps_speed=0;
static uint16_t gps_year=0;
static uint8_t gps_month=0, gps_day=0;
static uint8_t gps_hour=0, gps_minute=0, gps_second=0;
static uint32_t gps_vsat=0;

uint8_t buffer[256];

bool gps_init(void)
{   
    bool result = false;
    // L76K GPS USE 9600 BAUDRATE
    // result = setupGPS();
    if(!result) {
        // Set u-blox m10q gps baudrate 38400
        SerialGPS.begin(38400, SERIAL_8N1, BOARD_GPS_RXD, BOARD_GPS_TXD);
        result = GPS_Recovery();
        if (!result) {
            SerialGPS.updateBaudRate(9600);
            result = GPS_Recovery();
            if (!result) {
                Serial.println("GPS Connect failed~!");
                result = false;
            }
            SerialGPS.updateBaudRate(38400);
        }
    }
    if(result) {
        Serial.println("GPS Task Create...!");
        gps_task_create();
    }
    return result;
}

void gps_task(void *param)
{
    static uint32_t last_display_time = 0;
    static uint32_t last_check_time = 0;

    while(1)
    {
        uint32_t now = millis();

        // Performance optimization: Reduce serial forwarding overhead
        while (Serial.available()) {
            SerialGPS.write(Serial.read());
        }

        // Performance optimization: Process GPS data but limit display updates
        while (SerialGPS.available()) {
            int c = SerialGPS.read();
            if (gps.encode(c)) {
                // Performance optimization: Only display GPS info every 5 seconds to reduce console spam
                if (now - last_display_time > 5000) {
                    displayInfo();
                    last_display_time = now;
                }
            }
        }

        // Performance optimization: Check for GPS detection less frequently
        if (now - last_check_time > 10000) { // Check every 10 seconds instead of constantly
            if (now > 30000 && gps.charsProcessed() < 10) {
                Serial.println(F("No GPS detected: check wiring."));
            }
            last_check_time = now;
        }

        // Performance optimization: Increase delay to reduce CPU usage
        delay(100); // 100ms instead of 1ms - GPS doesn't need millisecond precision
    }
}

void gps_task_create(void)
{
    xTaskCreate(gps_task, "gps_task", 1024 * 3, NULL, GPS_PRIORITY, &gps_handle);
    vTaskSuspend(gps_handle);
}

void gps_task_suspend(void)
{
    vTaskSuspend(gps_handle);
}

void gps_task_resume(void)
{
    vTaskResume(gps_handle);
}

void gps_get_coord(double *lat, double *lng)
{
    *lat = gps_lat;
    *lng = gps_lng;
}

void gps_get_data(uint16_t *year, uint8_t *month, uint8_t *day)
{
    *year = gps_year;
    *month = gps_month;
    *day = gps_day;
}

void gps_get_time(uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    *hour = gps_hour;
    *minute = gps_minute;
    *second = gps_second;
}

void gps_get_satellites(uint32_t *vsat)
{
    *vsat = gps_vsat;   // Visible Satellites
}

void gps_get_speed(double *speed)
{
    *speed = gps_speed;
}

/* clang-format on */
void displayInfo()
{
    // Performance optimization: Disable GPS console output for launcher performance
    // Only update GPS data variables, no serial printing to reduce system load
    static bool gps_console_disabled = true; // Set to false to re-enable GPS console output

    if (gps_console_disabled) {
        // Silent mode: Only update variables, no console output
        if (gps.location.isValid()) {
            gps_lat = gps.location.lat();
            gps_lng = gps.location.lng();
        }
        if (gps.date.isValid()) {
            gps_year = gps.date.year();
            gps_month = gps.date.month();
            gps_day = gps.date.day();
        }
        if (gps.time.isValid()) {
            gps_hour = gps.time.hour();
            gps_minute = gps.time.minute();
            gps_second = gps.time.second();
        }
        if (gps.satellites.isValid()) {
            gps_vsat = gps.satellites.value();
        }
        if (gps.speed.isValid()) {
            gps_speed = gps.speed.kmph();
        }
        return; // Exit early to avoid console output
    }

    // Original console output code (only runs if gps_console_disabled = false)

    if (gps.location.isValid())
    {
        gps_lat = gps.location.lat();
        gps_lng = gps.location.lng();
        // Performance optimization: Only print location when it changes significantly
        static double last_lat = 0, last_lng = 0;
        if (abs(gps_lat - last_lat) > 0.0001 || abs(gps_lng - last_lng) > 0.0001) {
            Serial.print(F("Location: "));
            Serial.print(gps_lat, 6);
            Serial.print(F(","));
            Serial.print(gps_lng, 6);
            last_lat = gps_lat;
            last_lng = gps_lng;
        }
    }
    else
    {
        // Performance optimization: Only print INVALID once per session
        static bool invalid_printed = false;
        if (!invalid_printed) {
            Serial.print(F("Location: INVALID"));
            invalid_printed = true;
        }
    }

    if (gps.date.isValid())
    {
        gps_year = gps.date.year();
        gps_month = gps.date.month();
        gps_day = gps.date.day();
        // Performance optimization: Only print date when it changes
        static uint16_t last_year = 0;
        static uint8_t last_month = 0, last_day = 0;
        if (gps_year != last_year || gps_month != last_month || gps_day != last_day) {
            Serial.print(F("  Date/Time: "));
            Serial.print(gps_month);
            Serial.print(F("/"));
            Serial.print(gps_day);
            Serial.print(F("/"));
            Serial.print(gps_year);
            last_year = gps_year;
            last_month = gps_month;
            last_day = gps_day;
        }
    }
    else
    {
        // Performance optimization: Reduce INVALID spam
        static bool date_invalid_printed = false;
        if (!date_invalid_printed) {
            Serial.print(F("  Date/Time: INVALID"));
            date_invalid_printed = true;
        }
    }

    Serial.print(F(" "));
    if (gps.time.isValid())
    {
        gps_hour = gps.time.hour();
        gps_minute = gps.time.minute();
        gps_second = gps.time.second();

        if (gps_hour < 10)
            Serial.print(F("0"));
        Serial.print(gps_hour);
        Serial.print(F(":"));
        if (gps_minute < 10)
            Serial.print(F("0"));
        Serial.print(gps_minute);
        Serial.print(F(":"));
        if (gps_second < 10)
            Serial.print(F("0"));
        Serial.print(gps_second);
        Serial.print(F("."));
    }
    else
    {
        Serial.print(F("INVALID"));
    }

    Serial.print(F("  Satellites: "));
    if(gps.satellites.isValid())
    {
        gps_vsat = gps.satellites.value();
        Serial.print(gps_vsat);
        Serial.print(F(" "));
    }

    Serial.print(F("  Speed: "));
    if(gps.speed.isValid())
    {
        gps_speed = gps.speed.kmph();
        Serial.print(gps_speed);
        Serial.print(F(" "));
    }

    Serial.println();
}
/* clang-format off */

bool setupGPS()
{
    // L76K GPS USE 9600 BAUDRATE
    SerialGPS.begin(9600, SERIAL_8N1, BOARD_GPS_RXD, BOARD_GPS_TXD);
    bool result = false;
    uint32_t startTimeout ;
    for (int i = 0; i < 3; ++i) {
        SerialGPS.write("$PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0*02\r\n");
        delay(5);
        // Get version information
        startTimeout = millis() + 3000;
        Serial.print("Try to init L76K . Wait stop .");
        while (SerialGPS.available()) {
            Serial.print(".");
            SerialGPS.readString();
            if (millis() > startTimeout) {
                Serial.println("Wait L76K stop NMEA timeout!");
                return false;
            }
        };
        Serial.println();
        SerialGPS.flush();
        delay(200);

        SerialGPS.write("$PCAS06,0*1B\r\n");
        startTimeout = millis() + 500;
        String ver = "";
        while (!SerialGPS.available()) {
            if (millis() > startTimeout) {
                Serial.println("Get L76K timeout!");
                return false;
            }
        }
        SerialGPS.setTimeout(10);
        ver = SerialGPS.readStringUntil('\n');
        if (ver.startsWith("$GPTXT,01,01,02")) {
            Serial.println("L76K GNSS init succeeded, using L76K GNSS Module\n");
            result = true;
            break;
        }
        delay(500);
    }
    // Initialize the L76K Chip, use GPS + GLONASS
    SerialGPS.write("$PCAS04,5*1C\r\n");
    delay(250);
    SerialGPS.write("$PCAS03,1,1,1,1,1,1,1,1,1,1,,,0,0*26\r\n");
    delay(250);
    // Switch to Vehicle Mode, since SoftRF enables Aviation < 2g
    SerialGPS.write("$PCAS11,3*1E\r\n");
    return result;
}


static int getAck(uint8_t *buffer, uint16_t size, uint8_t requestedClass, uint8_t requestedID)
{
    uint16_t    ubxFrameCounter = 0;
    bool        ubxFrame = 0;
    uint32_t    startTime = millis();
    uint16_t    needRead;

    while (millis() - startTime < 800) {
        while (SerialGPS.available()) {
            int c = SerialGPS.read();
            switch (ubxFrameCounter) {
            case 0:
                if (c == 0xB5) {
                    ubxFrameCounter++;
                }
                break;
            case 1:
                if (c == 0x62) {
                    ubxFrameCounter++;
                } else {
                    ubxFrameCounter = 0;
                }
                break;
            case 2:
                if (c == requestedClass) {
                    ubxFrameCounter++;
                } else {
                    ubxFrameCounter = 0;
                }
                break;
            case 3:
                if (c == requestedID) {
                    ubxFrameCounter++;
                } else {
                    ubxFrameCounter = 0;
                }
                break;
            case 4:
                needRead = c;
                ubxFrameCounter++;
                break;
            case 5:
                needRead |=  (c << 8);
                ubxFrameCounter++;
                break;
            case 6:
                if (needRead >= size) {
                    ubxFrameCounter = 0;
                    break;
                }
                if (SerialGPS.readBytes(buffer, needRead) != needRead) {
                    ubxFrameCounter = 0;
                } else {
                    return needRead;
                }
                break;

            default:
                break;
            }
        }
    }
    return 0;
}

static bool GPS_Recovery()
{
    uint8_t cfg_clear1[] = {0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x1C, 0xA2};
    uint8_t cfg_clear2[] = {0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x1B, 0xA1};
    uint8_t cfg_clear3[] = {0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x03, 0x1D, 0xB3};
    SerialGPS.write(cfg_clear1, sizeof(cfg_clear1));

    if (getAck(buffer, 256, 0x05, 0x01)) {
        Serial.println("Get ack successes!");
    }
    SerialGPS.write(cfg_clear2, sizeof(cfg_clear2));
    if (getAck(buffer, 256, 0x05, 0x01)) {
        Serial.println("Get ack successes!");
    }
    SerialGPS.write(cfg_clear3, sizeof(cfg_clear3));
    if (getAck(buffer, 256, 0x05, 0x01)) {
        Serial.println("Get ack successes!");
    }

    // UBX-CFG-RATE, Size 8, 'Navigation/measurement rate settings'
    uint8_t cfg_rate[] = {0xB5, 0x62, 0x06, 0x08, 0x00, 0x00, 0x0E, 0x30};
    SerialGPS.write(cfg_rate, sizeof(cfg_rate));
    if (getAck(buffer, 256, 0x06, 0x08)) {
        Serial.println("Get ack successes!");
    } else {
        return false;
    }
    return true;
}
