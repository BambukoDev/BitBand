#ifndef LOG_H
#define LOG_H

#include <pico/stdlib.h>
#include "sd_card.h"
#include "ff.h"

class Log {
public:
    int init();
    void log(const char* data);
    void quit();
private:
    FRESULT fr;
	FIL fil;
    char filename[8] = "log.txt";
};

#endif