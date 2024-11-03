// USER INCLUDE
#include "LiquidCrystal_I2C.h"
#include "Log.h"
#include "ButtonInput.h"
#include "TimeKeep.h"

#include "menus/menu.h"

// SDK INCLUDE
#include "pico/cyw43_arch.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <hardware/adc.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/rtc.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <pico/types.h>
#include <pico/util/datetime.h>
#include <sys/time.h>

#include <sys/types.h>

// FreeRTOS
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "wifi_data.h"

#define LED_GPIO 27

const unsigned long I2C_CLOCK = 100000;
const uint8_t LCD_I2C_ADDRESS = 0x27;

struct process_struct_t {
    char* backbuffer = NULL;
    datetime_t *curr_time = NULL;
};

bool SHOULD_CLEAR = false;

int setup() {
	printf("Setting up Pico modules\r\n");
	stdio_init_all();
	rtc_init();
    ButtonInput::init();

    lcd_init(i2c_default, LCD_I2C_ADDRESS);

	// Initialize ADC
	// adc_init();
	// adc_set_temp_sensor_enabled(true);

	if (!sd_init_driver()) {
		lcd_set_cursor(0,0);
		lcd_print("SD init failed");
		sleep_ms(1000);
		return -2;
	}

	rtc_init();
    datetime_t t = {.year = 2024,
                    .month = 10,
                    .day = 31,
                    .dotw = 4,  // 0 is Sunday, so 5 is Friday
                    .hour = 11,
                    .min = 00,
                    .sec = 00};
	rtc_set_datetime(&t);
	sleep_us(64);
    if (!rtc_running()) {
		lcd_print("RTC not running");
		sleep_ms(1000);
	}

	gpio_init(LED_GPIO);
	gpio_set_dir(LED_GPIO, GPIO_OUT);

	return 0;
}

void blink_led(void* params) {
	while(true) {
		gpio_put(LED_GPIO, true);
		vTaskDelay(2000);
		gpio_put(LED_GPIO, false);
		vTaskDelay(2000);
	}
}

void lcd_format_print(char* str, size_t len) {
	if (SHOULD_CLEAR) {
        lcd_set_cursor(0, 0);
        for (int i = 0; i < len; i++) {
            lcd_print(" ");
        }
		SHOULD_CLEAR = false;
	}
    int curr_line = 0;
    lcd_set_cursor(0, 0);
    for (int i = 0; i < len; i++) {
        if (str[i] == '\0') break; // Terminate at the end of the string OR if the len is reached
        if (str[i] == '\n') { // Make newline work
            curr_line++;
            lcd_set_cursor(curr_line, 0);
            continue;
        }

		// Weird magic stuff cuz its C/C++
        char to_print[2];
        to_print[0] = str[i];
        to_print[1] = '\0';
        lcd_print(to_print);
    }
}

void render(void* params) {
	char framebuffer[85];
	char *backbuffer = (char*)params;
	while(true) {
        printf("%s\n", backbuffer);
        // Copy the backbuffer to framebuffer
		strncpy(framebuffer, backbuffer, sizeof(char[85]));
		// Clear the backbuffer
        memset(backbuffer, '\0', sizeof(char[85]));
        // Reset cursor position
        lcd_set_cursor(0, 0);
		// Print formatted text to lcd
		lcd_format_print(framebuffer, sizeof(char[85]));
		// Reset the framebuffer
        memset(framebuffer, '\0', sizeof(char[85]));
		vTaskDelay(10);
    }
}

void process_menus(void* params) {
	enum CURRENT_MODE {
		MODE_TIME,
		MODE_SELECT
	} CURRENT_MODE;
	CURRENT_MODE = MODE_TIME;
	enum CURRENT_MENU {
		MENU_NONE,
		MENU_WIFI_CONNECT,
		MENU_SETTINGS,
		MENU_CREDITS
	} CURRENT_MENU;
	const uint8_t MENU_COUNT = 4;

	uint8_t current_menu = 0;
	char menu_stack[MENU_COUNT][21]; // extra space for \0 character
    strcpy(menu_stack[0], "Connect to WIFI");
    strcpy(menu_stack[1], "Settings");
	strcpy(menu_stack[2], "Credits");
    strcpy(menu_stack[3], "Sleep");

	process_struct_t *process_struct = (process_struct_t*)params;
    char *backbuffer = process_struct->backbuffer;
	datetime_t *date = process_struct->curr_time;
    // print if rtc is running
	uint8_t date_field = 0; // 0: year, 1: month, 2: day, 3: hour, 4: min, 5: sec
	while(true) {
		char TimeString[85];
		if (ButtonInput::is_mod_pressed()) { // Simple switch between modes
            printf("Mode switched\n");
            if (CURRENT_MODE == MODE_TIME) {
				CURRENT_MODE = MODE_SELECT;
			} else {
				CURRENT_MODE = MODE_TIME;
			}
			// memset(backbuffer, ' ', sizeof(char[85]));
			SHOULD_CLEAR = true;
			vTaskDelay(20);
		}
        memset(backbuffer, '\0', sizeof(char[85]));
        switch (CURRENT_MODE) {
			case MODE_TIME: { // Render clock
                snprintf(backbuffer, sizeof(char[85]),
                        "        TIME\n%04d-%02d-%02d %02d:%02d:%02d\n",
                        (int)date->year, (int)date->month, (int)date->day, (int)date->hour, (int)date->min, (int)date->sec);
				
				// Change time depending on the current field selected
				if (ButtonInput::is_down_pressed()) {
					date_field = (date_field + 1) % 6;
				}
				if (ButtonInput::is_up_pressed()) {
					date_field = (date_field - 1 + 6) % 6;
				}
				if (ButtonInput::is_home_pressed()) {
					switch (date_field) {
						case 0: // change year
                            date->year++;
                            break;
						case 1: // change month
                            date->month = (date->month % 12) + 1;
                            break;
						case 2: // change day
							date->day = (date->day % 28) + 1; // Only valid for non-leap years
							break;
						case 3: // change hour
                            date->hour = (date->hour + 1) % 24;
                            break;
						case 4: // change min
                            date->min = (date->min + 1) % 60;
                            break;
						case 5: // change sec
                            date->sec = 0;
                            break;
					}
				}
				if (ButtonInput::is_enter_pressed()) {
					switch (date_field) {
						case 0: // change year
							date->year--;
							break;
						case 1: // change month
                            date->month = (date->month - 1 + 12) % 12;
                            break;
						case 2: // change day
							date->day = (date->day - 1 + 28) % 28; // Only valid for non-leap years
							break;
						case 3: // change hour
                            date->hour = (date->hour - 1 + 24) % 24;
                            break;
						case 4: // change min
                            date->min = (date->min - 1 + 60) % 60;
                            break;
						case 5: // change sec
                            date->sec = 0;
                            break;
					}
				}
				snprintf(backbuffer + 33, 31, "%c    %c  %c  %c  %c  %c\n", (date_field == 0) ? '^' : ' ', (date_field == 1) ? '^' : ' ', (date_field == 2) ? '^' : ' ', (date_field == 3) ? '^' : ' ', (date_field == 4) ? '^' : ' ', (date_field == 5) ? '^' : ' ');
				rtc_set_datetime(date);
				sleep_us(64);
                // printf("[TIME]: %04d-%02d-%02d %02d:%02d:%02d\n", t.year,
                // t.month, t.day, t.hour, t.min, t.sec);
				break;
            }
			case (MODE_SELECT): {
				switch (CURRENT_MENU) {
					case MENU_NONE: { // Render menu selection
                        for (int i = 0; i < MENU_COUNT; i++) {
							strcat(backbuffer, menu_stack[i]);
							strcat(backbuffer, "\n");
                        }
                        break;
					}
				}
				break;
			}
			break;
		}
		vTaskDelay(200);
	}
}

void write_time_to_sd(void *params) {
	auto *timekeep = (TimeKeep*)params;
	while (true) {
        timekeep->write_current_time();
		vTaskDelay(1000);
    }
}

void tick_time(void* params) {
	datetime_t *date = (datetime_t*)params;
	while(true) {
		rtc_get_datetime(date);
		sleep_us(64);
		vTaskDelay(1000);
	}
}

int main() {
	printf("STARTING PICO");
	setup();

	FATFS filesystem;
	f_mount(&filesystem, "0:", 1);

	Log log;
	log.init();
	log.log("# Raspberry PI Pico W : BukoPI Version 0.1");
	log.log("# This is a logfile of the last run of the device");

	TimeKeep timekeep;
	timekeep.load_current_time();

	lcd_clear();
	lcd_set_cursor(0,0);

	// Setup rendering values
	char backbuffer[85] = {'\0'};

	TaskHandle_t render_handle = NULL;
	TaskHandle_t process_handle = NULL;
	datetime_t curr_time;

	process_struct_t process_struct;
	process_struct.backbuffer = backbuffer;
	process_struct.curr_time = &curr_time;

	xTaskCreate(process_menus, "process", 1024, &process_struct, tskIDLE_PRIORITY, &process_handle);
    xTaskCreate(render, "render", 1024, backbuffer, tskIDLE_PRIORITY, &render_handle);
	xTaskCreate(write_time_to_sd, "write_time_to_sd", 1024, &timekeep, tskIDLE_PRIORITY, NULL);
	xTaskCreate(tick_time, "tick_time", 1024, &curr_time, tskIDLE_PRIORITY, NULL);

    vTaskStartScheduler();

	// should never get here

	log.log("Quitting...");
	lcd_clear();
	lcd_set_cursor(0,0);
	lcd_print("quit start");

	log.quit();

	lcd_set_cursor(1,0);
	lcd_print("quit end");
    return 0;
}
