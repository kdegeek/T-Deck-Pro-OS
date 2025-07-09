#ifndef EINK_MANAGER_CORRECTED_H
#define EINK_MANAGER_CORRECTED_H

#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

// Display power states
enum DisplayPowerState {
    DISPLAY_POWER_OFF = 0,
    DISPLAY_POWER_ON = 1,
    DISPLAY_POWER_SLEEP = 2
};

class EinkManager {
public:
    EinkManager();
    ~EinkManager();

    bool init();
    bool isInitialized() const;

    void setRotation(int rotation);
    int getRotation() const;

    void clear();
    void refresh();
    void partialRefresh();

    void drawText(int16_t x, int16_t y, const char* text, uint8_t size);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool filled);
    void drawCircle(int16_t x, int16_t y, int16_t r, bool filled);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

    void showBootSplash(const char* title, const char* status);
    void showSystemReady();
    void showError(const char* title, const char* message);
    void showLowBatteryWarning();
    void updateBatteryStatus(float voltage);

    void powerOn();
    void powerOff();
    void sleep();
    void wake();

    DisplayPowerState getPowerState() const;
    uint32_t getLastRefreshTime() const;
    uint32_t getRefreshCount() const;

    bool isBusy() const;
    void waitForIdle();

private:
    void writeCommand(uint8_t command);
    void writeData(uint8_t data);
    uint8_t readData();
    void waitForBusy();
    void initSPI();
    void resetDisplay();
    void initDisplay();
    void setDisplayWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

    bool initialized_;
    int rotation_;
    bool partial_mode_;
    uint32_t last_refresh_;
    uint32_t refresh_count_;
    DisplayPowerState power_state_;
    int cs_pin_;
    int dc_pin_;
    int rst_pin_;
    int busy_pin_;
};

#endif // EINK_MANAGER_CORRECTED_H 