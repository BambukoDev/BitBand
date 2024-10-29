#pragma once

#define BUTTON_UP_PIN 21
#define BUTTON_DOWN_PIN 20
#define BUTTON_ENTER_PIN 19
#define BUTTON_HOME_PIN 18

class ButtonInput {
public:
    void init();

    // button check
    bool is_up_pressed();
    bool is_down_pressed();
    bool is_enter_pressed();
    bool is_home_pressed();
};