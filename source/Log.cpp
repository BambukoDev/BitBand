#include "Log.h"
#include "LiquidCrystal_I2C.h"
#include <cstdio>
#include <ff.h>
#include <pico/time.h>

int Log::init() {
    printf("Initializing log...\n");
    auto result = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FR_OK) {
        printf("Failed to open %s: %d\n", filename, result);
        lcd_set_cursor(0, 0);
        lcd_print("Failed to open file");
        sleep_ms(1000);
        printf("Failed to open %s: %d\n", filename, result);
        return 1;
    } else {
        printf("File %s opened successfully.\n", filename);
        lcd_set_cursor(0, 0);
        lcd_print("File open successful");
        sleep_ms(1000);
    }
    f_close(&fil);
    printf("Log initialization complete.\n");
    return 0;
}

void Log::log(const char* data) {
    f_open(&fil, filename, FA_WRITE | FA_OPEN_APPEND);
    f_printf(&fil, "Trying to log\r\n");
    f_printf(&fil, "[LOG]: %s\r\n", data);
    printf("[LOG]: %s\n", data);
    f_close(&fil);
}

void Log::quit() {
    f_close(&fil);
}

    // lcd_set_cursor(0,0);
	// fr = f_mount(&fs, "0:", 1);
	// if (fr == FR_OK) { lcd_print("Filesystem succ"); }
	// else { lcd_print("Filesystem fail"); }
	// printf("%d", fr);

	// lcd_set_cursor(1,0);
	// fr = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
	// if (fr == FR_OK) { lcd_print("File succ"); }
	// else { lcd_print("File fail"); }
	// printf("%d", fr);

	// lcd_set_cursor(2,0);
	// ret = f_printf(&fil, "Test!\r\n");
	// if (ret < 0) { lcd_print("Write fail"); }
	// else { lcd_print("Write succ"); }

	// lcd_set_cursor(3,0);
	// fr = f_close(&fil);
	// if (fr == FR_OK) { lcd_print("Close succ"); }
	// else { lcd_print("Close fail"); }
	// printf("%d", fr);

	// sleep_ms(2000);

	// lcd_set_cursor(0,0);
	// fr = f_unmount("0:");
	// if (fr == FR_OK) { lcd_print("Unmount succ"); }
	// else { lcd_print("Unmount fail"); }
	// printf("%d", fr);

	// sleep_ms(5000);