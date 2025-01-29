#pragma once

#include <cstdio>
#include <pico/stdlib.h>
#include "LCD_I2C.hpp"

#include "FreeRTOS.h"
#include "task.h"

void timed_print(LCD_I2C* lcd, const char* str, size_t size, uint timeout) {
    // lcd->CursorOn();
    // lcd->CursorBlinkOn();
    for (int i = 0; i < size; i++) {
        lcd->PrintChar(str[i]);
        sleep_ms(timeout);
    }
    // lcd->CursorBlinkOff();
    // lcd->CursorOff();
}

void timed_print_task(LCD_I2C *lcd, const char* str, size_t size, uint timeout) {
    lcd->CursorOn();
    for (int i = 0; i < size; i++) {
        lcd->PrintChar(str[i]);
        vTaskDelay(timeout);
    }
    lcd->CursorOff();
}