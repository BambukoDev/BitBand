#ifndef LOG_H
#define LOG_H

#include <pico/stdlib.h>
#include "sd_card.h"
#include "ff.h"

class Log {
public:
    void init();
    void log(const char* data);
    void quit();
private:
    FRESULT fr;
	FATFS fs;
	FIL fil;
    char filename[8] = "log.txt";
};

#endif