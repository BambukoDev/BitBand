#pragma once
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <pico/time.h>

#include <cstddef>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#define LED_GPIO 27

class LedManager {
public:
    void setup_led(uint16_t* blink_speed) {
        gpio_init(LED_GPIO);
        gpio_set_function(LED_GPIO, GPIO_FUNC_PWM);
        pwm_set_enabled(pwm_gpio_to_slice_num(LED_GPIO), true);
        this->blink_speed = blink_speed;
    }

    void set_led_rightness(uint8_t brightness) {
        uint16_t duty = brightness * brightness;
        pwm_set_gpio_level(LED_GPIO, duty);
    }

    /**
     * @brief Blinks the LED in a specific invertal (smooth, blocking)
     * @return Never returns
     */
    void blink_led() {
        if (blink_speed == nullptr) return;
        for (uint8_t i = 0; i < 255; i++) {
            set_led_rightness(i);
            vTaskDelay(*blink_speed);
        }
        for (uint8_t i = 255; i > 0; i--) {
            set_led_rightness(i);
            vTaskDelay(*blink_speed);
        }
    }
private:
    uint16_t* blink_speed = nullptr;
};