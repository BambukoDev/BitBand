#include "Log.h"
#include <cstdio>

void Log::init() {
    f_mount(&fs, "0:", 1);
    f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    f_close(&fil);
}

void Log::log(const char* data) {
    f_open(&fil, filename, FA_WRITE | FA_OPEN_APPEND);
    f_printf(&fil, "Trying to log\r\n");
    f_printf(&fil, "[LOG]: %d\r\n", data);
    printf("[LOG]: %d\r\n", data);
    f_close(&fil);
}

void Log::quit() {
    f_unmount("0:");
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