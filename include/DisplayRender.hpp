#pragma once

#include <hardware/timer.h>
#include <pico/stdlib.h>
#include <string>
#include "LCD_I2C.hpp"

#include "FreeRTOS.h"
#include "task.h"

constexpr auto LCD_COLUMNS = 20;
constexpr auto LCD_ROWS = 4;

std::string DISPLAY_BUFFER[LCD_ROWS];
bool DISPLAY_LOCK = false;
bool DISPLAY_CHANGED = false;

void render_lock_display() { DISPLAY_LOCK = true; }

void render_unlock_display() { DISPLAY_LOCK = false; }

void render_display(LCD_I2C &lcd) {
    if (!DISPLAY_CHANGED) return;

    // printf("%s\n%s\n%s\n%s\n", DISPLAY_BUFFER[0].c_str(), DISPLAY_BUFFER[1].c_str(), DISPLAY_BUFFER[2].c_str(), DISPLAY_BUFFER[3].c_str());
    lcd.Clear();
    for (int i = 0; i < LCD_ROWS; i++) {
        // wait for screen to unlock (very dirty workaround but i hope it works - Buko)
        // while (DISPLAY_LOCK) {
        //     printf("%s\n", "render wait");
        //     vTaskDelay(100);
        // }
        lcd.SetCursor(i, 0);
        // for (int j = 0; j < LCD_COLUMNS; j++) {
        //     if (DISPLAY_BUFFER[i].length() > j) {
        //         lcd.PrintChar(DISPLAY_BUFFER[i][j]);
        //     }
        // }
        lcd.PrintString(DISPLAY_BUFFER[i]);
    }
    DISPLAY_CHANGED = false;
}

void render_clear_buffer() {
    render_lock_display();

    for (int i = 0; i < LCD_ROWS; i++) {
        DISPLAY_BUFFER[i] = "";
    }
    DISPLAY_CHANGED = true;

    render_unlock_display();
}

void render_init() {
    render_clear_buffer();
}

void render_set_screen(std::string text, bool force_refresh = false) {
    if (text.length() > LCD_COLUMNS * LCD_ROWS + 4) return;

    if (force_refresh) render_clear_buffer();

    // TODO: Compare the actual content of the text to the buffer
    // before switching the DISPLAY_CHANGED flag

    render_lock_display();
    
    std::string screen_buf[LCD_ROWS];

    const char* buffer = text.c_str();
    uint8_t row = 0;
    for (uint i = 0; i < text.length(); i++) {
        // TODO: Figure out how to get to the current char of a row for comparison
        //                                    here
        // if (buffer[i] != DISPLAY_BUFFER[row][]) DISPLAY_CHANGED = true;
        if (buffer[i] == '\n') {
            row++;
        } else {
            screen_buf[row] += buffer[i];
        }
    }

    // Copy the contents of screen_buf to DISPLAY_BUFFER
    for (uint i = 0; i < LCD_ROWS; i++) {
        DISPLAY_BUFFER[i] = screen_buf[i];
    }

    render_unlock_display();
}

void render_set_row(uint8_t row, std::string text) {
    if (row >= LCD_ROWS) return;
    if (DISPLAY_BUFFER[row] == text) return;

    render_lock_display();

    // printf("%s\n", text.c_str());

    DISPLAY_BUFFER[row] = text;
    DISPLAY_CHANGED = true;

    render_unlock_display();
}