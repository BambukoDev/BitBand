#include "TimeKeep.h"
#include "ff.h"
#include "json-maker/json-maker.h"
#include "tiny-json.h"
#include <cstdio>
#include <hardware/rtc.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <pico/types.h>
#include <sys/time.h>

void TimeKeep::init() {
    f_open(&fil, "time.txt", FA_CREATE_ALWAYS | FA_WRITE);
    f_close(&fil);
}

void TimeKeep::load_current_time() {
    f_open(&fil, "time.txt", FA_READ);
    json_t pool[12];
    char f_buf[256];
    f_read(&fil, f_buf, 256, NULL);
    auto json = json_create(f_buf, pool, 12);
    if (json) {
        printf("JSON open successful\n");
        json_t const* year = json_getProperty(json, "year");
        json_t const* month = json_getProperty(json, "month");
        json_t const* day = json_getProperty(json, "day");
        json_t const* hour = json_getProperty(json, "hour");
        json_t const* minute = json_getProperty(json, "minute");
        json_t const* second = json_getProperty(json, "second");
        datetime_t dt = {
            .year = json_getInteger(year),
            .month = json_getInteger(month),
            .day = json_getInteger(day),
            .hour = json_getInteger(hour),
            .min = json_getInteger(minute),
            .sec = json_getInteger(second),
        };
        // printf("[TIME]: %hd-%hd-%hd %hd:%hd:%hd\n", dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
        rtc_set_datetime(&dt);
        sleep_us(64);
    }
    f_close(&fil);
}

void TimeKeep::write_current_time() {
    datetime_t dt;
    rtc_get_datetime(&dt);
    const uint TIME_FILE_SIZE = 256;
    char f_buf[TIME_FILE_SIZE] = {0};
    size_t f_size = TIME_FILE_SIZE;
    time_to_json(f_buf, &dt, &f_size);
    f_open(&fil, "time.txt", FA_WRITE | FA_CREATE_ALWAYS);
    f_printf(&fil, f_buf);
    //printf("[TIME]: %s\n", f_buf);
    f_close(&fil);
}

void TimeKeep::quit() {
    f_close(&fil);
}

char* TimeKeep::time_to_json(char *dest, datetime_t *time, size_t *remLenght) {
    dest = json_objOpen(dest, NULL, remLenght);
    dest = json_int(dest, "year", time->year, remLenght);
    dest = json_int(dest, "month", time->month, remLenght);
    dest = json_int(dest, "day", time->day, remLenght);
    dest = json_int(dest, "hour", time->hour, remLenght);
    dest = json_int(dest, "minute", time->min, remLenght);
    dest = json_int(dest, "second", time->sec, remLenght);
    dest = json_objClose(dest, remLenght);
    return dest;
}