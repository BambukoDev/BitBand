#ifndef MENU_H
#define MENU_H

#include <stdio.h>
#include <map>
#include <string>
#include <LiquidCrystal_I2C.h>

class Menu {
public:
    const char* name = "";
    Menu* menus[3];
    uint selected_menu = 0;

    void render(bool force_redraw = false) {
        if (force_redraw) lcd_clear();
        // TODO: Somehow calculate centered position
        lcd_set_cursor(0,0);
        lcd_print("-[");
        lcd_print(name);
        lcd_print("]-");

        for (uint i = selected_menu; (i < selected_menu + 3) && (i < 3 /*menu array len*/); i++) {
            lcd_set_cursor(i + 1, 0);
            lcd_print(menus[i]->name);
        }
    }
};

#endif