/**
 * @file      test_touchpad.h
 * @author    ShallowGreen123
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2024-05-27
 *
 */

#include <Arduino.h>
#include "utilities.h"

#include <TouchDrvCSTXXX.hpp>
TouchDrvCSTXXX touch;

void setup()
{
    Serial.begin(115200);
    
    touch.setPins(BOARD_TOUCH_RST, BOARD_TOUCH_INT);
    bool hasTouch = touch.begin(Wire, BOARD_I2C_ADDR_TOUCH, BOARD_TOUCH_SDA, BOARD_TOUCH_SCL);
    if (!hasTouch) {
        Serial.println("Failed to find Capacitive Touch !");
    } else {
        Serial.println("Find Capacitive Touch");
    }

}

void loop()
{
    int16_t read_x, read_y;
    uint8_t touched = touch.getPoint(&read_x, &read_y, 1);
    if(touched) {
        Serial.print("touch_x=");Serial.print(read_x);
        Serial.print(", touch_y=");Serial.println(read_y);
    }
    delay(1);
}

