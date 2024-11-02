#ifndef TIMEKEEP_H
#define TIMEKEEP_H

#include <pico/stdlib.h>
#include <pico/types.h>
#include "sd_card.h"
#include "ff.h"

class TimeKeep {
public:
    void init();
    void load_current_time();
    void write_current_time();
    void quit();
private:
	FIL fil;
    char* time_to_json(char *dest, datetime_t *time, size_t *remLenght);
};

#endif