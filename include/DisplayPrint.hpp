#pragma once

#include <cstdio>
#include <cmath>
#include <pico/stdlib.h>
#include <ctime>

#include "LCD_I2C.hpp"

#include "FreeRTOS.h"
#include "task.h"

void timed_print(LCD_I2C* lcd, const char* str, size_t size, uint timeout) {
    lcd->CursorOn();
    // lcd->CursorBlinkOn();
    for (int i = 0; i < size; i++) {
        lcd->PrintChar(str[i]);
        sleep_ms(timeout);
    }
    // lcd->CursorBlinkOff();
    lcd->CursorOff();
    sleep_ms(timeout);
}

void timed_print_task(LCD_I2C *lcd, const char* str, size_t size, uint timeout) {
    lcd->CursorOn();
    for (int i = 0; i < size; i++) {
        lcd->PrintChar(str[i]);
        vTaskDelay(timeout);
    }
    lcd->CursorOff();
}

char random_char() {
    char c = (char)(33 + (rand() % 94)); // ASCII 33-126
    return c;
}

// Slide clear animation with random characters
void slide_clear_animation(LCD_I2C *lcd) {
    if (lcd == nullptr) return;
    srand(time(NULL));  // Seed randomness

    for (int col = 20 - 1; col >= 0; col--) {  // Slide from right to left
        for (int row = 0; row < 4; row++) {
            lcd->SetCursor(row, col);
            lcd->PrintChar(random_char());
        }
        sleep_ms(40);  // Adjust speed
    }
    sleep_ms(100);
    lcd->Clear();
}