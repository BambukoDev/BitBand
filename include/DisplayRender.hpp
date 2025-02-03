#include <hardware/timer.h>
#include <pico/stdlib.h>
#include <string>
#include "LCD_I2C.hpp"

constexpr auto LCD_COLUMNS = 20;
constexpr auto LCD_ROWS = 4;

std::string DISPLAY_BUFFER[LCD_ROWS];
bool DISPLAY_LOCK = false;
bool DISPLAY_CHANGED = false;

void render_display(LCD_I2C &lcd) {
    printf("%s\n", "debug render begin");
    if (!DISPLAY_CHANGED) return;
    printf("%s\n", "debug render 1");

    lcd.Clear();
    printf("%s\n", "debug render 2");
    for (int i = 0; i < LCD_ROWS; i++) {
        printf("%s\n", "debug render 3");
        // wait for screen to unlock (very dirty workaround but i hope it works - Buko)
        while (DISPLAY_LOCK) {
            printf("%s\n", "debug render 4");
            busy_wait_us(1);
        }
        printf("%s\n", "debug render 5");
        lcd.SetCursor(i, 0);
        lcd.PrintString(DISPLAY_BUFFER[i]);
        printf("%s\n", "debug render 6");
    }
    DISPLAY_CHANGED = false;
    printf("%s\n", "debug render end");
}

void render_set_screen(std::string &text) {
    if (text.length() > LCD_COLUMNS * LCD_ROWS + 4) return;
    
    const char* buffer = text.c_str();
    uint8_t row = 0;
    for (uint i = 0; i < text.length(); i++) {
        if (buffer[i] == '\n') {
            row++;
        } else {
            DISPLAY_BUFFER[row] += buffer[i];
        }
    }
    DISPLAY_CHANGED = true;
}

void render_set_row(uint8_t row, std::string &text) {
    if (row >= LCD_ROWS) return;
    
    DISPLAY_BUFFER[row] = text;
    DISPLAY_CHANGED = true;
}