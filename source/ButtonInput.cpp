#include "ButtonInput.h"
#include <hardware/gpio.h>

void ButtonInput::init() {
    gpio_init(BUTTON_UP_PIN);
    gpio_set_dir(BUTTON_UP_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_UP_PIN);

    gpio_init(BUTTON_DOWN_PIN);
    gpio_set_dir(BUTTON_DOWN_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_DOWN_PIN);

    gpio_init(BUTTON_ENTER_PIN);
    gpio_set_dir(BUTTON_ENTER_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_ENTER_PIN);

    gpio_init(BUTTON_HOME_PIN);
    gpio_set_dir(BUTTON_HOME_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_HOME_PIN);

    gpio_init(BUTTON_MOD_PIN);
    gpio_set_dir(BUTTON_MOD_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_MOD_PIN);
}

bool ButtonInput::is_up_pressed() {
    return !gpio_get(BUTTON_UP_PIN);
}
bool ButtonInput::is_down_pressed() {
    return !gpio_get(BUTTON_DOWN_PIN);
}
bool ButtonInput::is_enter_pressed() {
    return !gpio_get(BUTTON_ENTER_PIN);
}
bool ButtonInput::is_home_pressed() {
    return !gpio_get(BUTTON_HOME_PIN);
}

bool ButtonInput::is_mod_pressed() {
    return !gpio_get(BUTTON_MOD_PIN);
}