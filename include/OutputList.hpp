#pragma once
#include <sys/types.h>
#include <cstddef>
#include <cstring>

#include "LCD_I2C.hpp"
#include "DisplayPrint.hpp"

class OutputList {
public:
    OutputList(LCD_I2C *lcd){
        this->lcd = lcd;
        clear();
    }

    void push(const char* data) {
        if (size != 4) {
            lines[size] = data;
            size++;
        } else {
            lines[0] = lines[1];
            lines[1] = lines[2];
            lines[2] = lines[3];
            lines[3] = data;
        }
    }

    void clear() {
        for (short i = 0; i < 4; i++) {
            lines[i] = nullptr;
        }
    }
    
    void render(bool force = false) {
        if (lcd == nullptr) return;
        if (last_rendered_size != size || force) lcd->Clear();
        for (short i = 0; i < size; i++) {
            if (lines[i] == nullptr) continue;
            lcd->SetCursor(i, 0);
            if (i != size - 1) {
                lcd->PrintString(lines[i]);
            } else { // print last line slowed
                timed_print(lcd, lines[i], strlen(lines[i]), 50);
            }
        }

        last_rendered_size = size;
    }
private:
    const char* lines[4];
    short size = 0; // between 0 and 4
    short last_rendered_size = -1;
    LCD_I2C *lcd;
};