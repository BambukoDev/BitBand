// USER INCLUDE
#include "LiquidCrystal_I2C.h"
#include "Log.h"
#include "ButtonInput.h"

#include "menus/menu.h"

// SDK INCLUDE
#include <hardware/gpio.h>
#include <pico/time.h>
#include <pico/stdlib.h>
// #include <pico/cyw43_arch.h> 
#include <pico/multicore.h>
#include <hardware/i2c.h>

#include <map>

// FreeRTOS

extern "C" {
	#include "FreeRTOS.h"
	#include "FreeRTOSConfig.h"
	#include "task.h"
}

#include <cstdint>

#define LED_GPIO 27

const unsigned long I2C_CLOCK = 100000;
const uint8_t LCD_I2C_ADDRESS = 0x27;

int setup() {
	printf("Setting up Pico modules\r\n");
	stdio_init_all();

	lcd_init(i2c_default, LCD_I2C_ADDRESS);

	if (!sd_init_driver()) {
		lcd_set_cursor(0,0);
		lcd_print("SD init failed");
		sleep_ms(1000);
		return -2;
	}

	gpio_init(LED_GPIO);
	gpio_set_dir(LED_GPIO, GPIO_OUT);

	return 0;
}

void blink_led(void* params) {
	gpio_put(LED_GPIO, true);
	vTaskDelay(100);
    gpio_put(LED_GPIO, false);
	vTaskDelay(100);
}

int main() {
	printf("STARTING PICO");
	ButtonInput binput;
	binput.init();

	Log log;
	log.init();
	log.log("# Raspberry PI Pico W : BukoPI Version 0.1");
	log.log("# This is a logfile of the last run of the device");

	setup();
	lcd_clear();
	lcd_set_cursor(0,0);

	TaskHandle_t blink_led_handle = NULL;

	uint32_t task_status = xTaskCreate(blink_led, "blink_led", 1024, NULL, tskIDLE_PRIORITY, &blink_led_handle);

	vTaskStartScheduler();

	// should never get here

    // main loop
	bool running = true;
    while(running) {
		lcd_print("Hello world!");
		printf("Hello world!");
	}

	log.log("Quitting...");
	lcd_clear();
	lcd_set_cursor(0,0);
	lcd_print("quit start");

	log.quit();

	lcd_set_cursor(1,0);
	lcd_print("quit end");
    return 0;
}
