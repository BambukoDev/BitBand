#pragma once

#define BUTTON_UP_PIN 21
#define BUTTON_DOWN_PIN 20
#define BUTTON_ENTER_PIN 19
#define BUTTON_HOME_PIN 18
#define BUTTON_MOD_PIN 22

class ButtonInput {
public:
    static void init();

    // button check
    static bool is_up_pressed();
    static bool is_down_pressed();
    static bool is_enter_pressed();
    static bool is_home_pressed();
    static bool is_mod_pressed();
};