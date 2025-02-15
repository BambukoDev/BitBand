// SDK INCLUDE
#include <boards/pico_w.h>
#include <hardware/adc.h>
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/irq.h>
#include <hardware/pwm.h>
#include <hardware/regs/intctrl.h>
#include <hardware/rtc.h>
#include <hardware/structs/io_bank0.h>
#include <hardware/sync.h>
#include <pico.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <pico/types.h>
#include <pico/util/datetime.h>
#include <sys/time.h>
#include <sys/types.h>

#include <string>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>
#include "class/cdc/cdc_device.h"

extern "C" {
    #include "ff.h"
    #include "sd_card.h"
    #include "music_file.h"

    #define DR_WAV_IMPLEMENTATION
    #include "dr_wav.h"
    #include "ds1302.h"
}

#include "ADXL345.h"

#include "tusb.h"

// Wireless stuff
#include <pico/cyw43_arch.h>

#include "LCD_I2C.hpp"
#include "DisplayPrint.hpp"

// FreeRTOS
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "croutine.h"

#ifndef mainRUN_FREE_RTOS_ON_CORE
#define mainRUN_FREE_RTOS_ON_CORE 0
#endif

#define CORE_AFFINITY_0 0x1
#define CORE_AFFINITY_1 0x2

// User includes
#include "DisplayRender.hpp"
#include "OutputList.hpp"
#include "Menu.hpp"
#include "LedPulse.hpp"
#include "Icons.h"
#include "Tetris.hpp"

constexpr auto I2C = PICO_DEFAULT_I2C_INSTANCE();
constexpr auto SDA = PICO_DEFAULT_I2C_SDA_PIN;
constexpr auto SCL = PICO_DEFAULT_I2C_SCL_PIN;
constexpr auto I2C_ADDRESS = 0x27;

LCD_I2C lcd = LCD_I2C(I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS, I2C, SDA, SCL);

#define BTN_UP 21
#define BTN_DOWN 20
#define BTN_HOME 19
#define BTN_SELECT 18
#define BTN_MODE 22
#define ROTARY_A_PIN 17  // Pin connected to the A signal of the rotary encoder
#define ROTARY_B_PIN 16  // Pin connected to the B signal of the rotary encoder

volatile uint32_t last_interrupt_time = 0;  // Debounce timing
const uint32_t debounce_delay = 300;          // Adjust this delay (milliseconds)

// #include "soundfile.h"

#define MAX_FILE_COUNT 100
FATFS fs;
bool playing = false;
bool paused = false;
char files[MAX_FILE_COUNT][32];
int file_count = 0;

#define BRIGHTNESS_PIN 6
#define AUDIO_PIN_LEFT 9
#define AUDIO_PIN_RIGHT 8

#define BUZZER_PIN 3

enum CURRENT_MODE_T { MODE_TIME, MODE_SELECT };
CURRENT_MODE_T CURRENT_MODE = MODE_TIME;

Menu mainmenu;
Menu musicmenu;
Menu settingsmenu;
Menu alarmmenu;

std::vector<TaskHandle_t> alarm_handles;

void set_local_datetime(datetime_t* t = nullptr) {
    if (t == nullptr) return;
    rtc_set_datetime(t);
    sleep_us(128); // wait for the clock to take change (around 3 clock cycles)
}

void set_datetime(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    DateTime date = {
        .year = year,
        .month = month,
        .day = day,
        .hour = hour,
        .minute = minute,
        .second = second
    };
    writeProtect(0);
    ds1302setDateTime(&date);
    writeProtect(1);
}

float get_light_percentage() {
    adc_select_input(1);
    float light = (adc_read() * 65535) / 4096;
    return light;
}

void check_core(const char* name) {
    if (vTaskCoreAffinityGet(NULL) == CORE_AFFINITY_1) {
        printf("%s: %s\n", name, "Running on core 1");
    } else {
        printf("%s: %s\n", name, "Running on core 0");
    }
}

void launch_execute_task(void* params) {
    check_core("launch");
    if (Menu::get_current()) Menu::get_current()->execute_current();
    printf("%s\n", "launch executed successfully");
    vTaskDelete(NULL);
}

void render(void* params) {
    render_init();
    check_core("render");
    DateTime now;

    while (true) {
        // if (Menu::get_current() != nullptr) Menu::get_current()->render();
        if (CURRENT_MODE == MODE_TIME) {
            ds1302getDateTime(&now);
            adc_select_input(4);
            float temperature = 27.0 - ((float)adc_read() * (3.3f / (1 << 12)) - 0.706) / 0.001721;
            adc_select_input(3);
            float voltage = (float)adc_read() / 10.0f;
            char row1[21];
            char row2[21];
            snprintf(row1, sizeof(char[21]), "%.2fC          %.1fV", temperature, voltage);
            snprintf(row2, sizeof(char[21]), "20%02d-%02d-%02d %02d:%02d:%02d", (int)now.year, (int)now.month, (int)now.day, (int)now.hour, (int)now.minute, (int)now.second);
            render_clear_buffer();
            render_set_row(0, row1);
            render_set_row(1, row2);

            vTaskDelay(1000);
        } else {
            if (Menu::get_current()) Menu::get_current()->render();
        }

        pwm_set_gpio_level(BRIGHTNESS_PIN, get_light_percentage());
        
        render_display(lcd);
        vTaskDelay(100);
    }
}

// void blink_led(void* params) {
//     // LedManager led;
//     // uint16_t speed = 100;
//     // led.setup_led(&speed);
//     while (true) {
//         // led.blink_led();
//         cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
//         vTaskDelay(500);
//         cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
//         vTaskDelay(500);
//     }
// }

std::string get_time_string() {
    DateTime now;
    ds1302getDateTime(&now);
    std::string dow = "Noneday";
    switch (now.dow) {
        case DOW_MON:
            dow = "Monday";
            break;
        case DOW_TUE:
            dow = "Tuesday";
            break;
        case DOW_WED:
            dow = "Wednesday";
            break;
        case DOW_THU:
            dow = "Thursday";
            break;
        case DOW_FRI:
            dow = "Friday";
            break;
        case DOW_SAT:
            dow = "Saturday";
            break;
        case DOW_SUN:
            dow = "Sunday";
            break;
        default:
            break;
    }
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%02d/%02d/20%d %02d:%02d:%02d %s", now.month, now.day, now.year, now.hour, now.minute, now.second, dow.c_str());
    // printf("%02d/%02d/20%d %02d:%02d:%02d %s\n", now.month, now.day, now.year, now.hour, now.minute, now.second, dow.c_str());
    return buffer;
}

void log_accel(void* params) {
    ADXL345 adxl;
    adxl.begin(ADXL345_DEFAULT_ADDRESS, &i2c1_inst, 14, 15);
    while (true) {
        printf("x: %i, y: %i, z: %i\n", adxl.getX(), adxl.getY(), adxl.getZ());
        vTaskDelay(10000);
    }
}


void log_time(void* params) {
    DateTime now;
    while (true) {
        std::string time = get_time_string();
        printf("%s\n", time.c_str());
        vTaskDelay(10000);  // one second
    }
}

void scan_files() {
    DIR dir;
    FILINFO fno;
    file_count = 0;
    if (f_opendir(&dir, "music") == FR_OK) { // Open "music" folder
        while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] && file_count < MAX_FILE_COUNT) {
            if (!(fno.fattrib & AM_DIR) && strstr(fno.fname, ".wav")) {
                snprintf(files[file_count], sizeof(files[file_count]), "%s", fno.fname);
                file_count++;
            }
        }
        f_closedir(&dir);
    }
}

size_t wav_read_callback(void* pUserData, void* pBufferOut, size_t bytesToRead) {
    FIL* file = (FIL*)pUserData;
    UINT bytesRead = 0;
    f_read(file, pBufferOut, bytesToRead, &bytesRead);
    return (size_t)bytesRead;
}

drwav_bool32 wav_seek_callback(void* pUserData, int offset, drwav_seek_origin origin) {
    FIL* file = (FIL*)pUserData;
    if (origin == drwav_seek_origin_start) {
        return (f_lseek(file, offset) == FR_OK);
    } else {
        return (f_lseek(file, f_tell(file) + offset) == FR_OK);
    }
}

void reset_speakers() {
    pwm_set_gpio_level(AUDIO_PIN_LEFT, 0);
    pwm_set_gpio_level(AUDIO_PIN_RIGHT, 0);
}

void play_wav_task(void *pvParameters) {
    if (playing) vTaskDelete(NULL);
    check_core("music");

    char filename[40] = "music/";
    strncat(filename, (char*)pvParameters, sizeof(filename) - 7);

    FIL file;
    if (f_open(&file, filename, FA_READ) == FR_OK) {
        drwav wav;
        if (drwav_init(&wav, wav_read_callback, wav_seek_callback, &file, NULL)) {
            playing = true;
            int16_t sampleBuffer[2];
            uint16_t outputSample;
            uint32_t elapsed_frames = 0, last_update_time = 0;
            lcd.Clear();

            char bar[21] = "[                  ]";
            char time_display[16];

            // Get the sample wait time in microseconds
            uint32_t wait_time_us = 920000 / wav.sampleRate;

            while (playing && drwav_read_pcm_frames(&wav, 1, sampleBuffer) > 0) {
                while (paused) vTaskDelay(10);

                // Update the screen every second
                uint32_t elapsed_seconds = elapsed_frames / wav.sampleRate;
                if (elapsed_seconds != last_update_time) {
                    last_update_time = elapsed_seconds;

                    int progress = (elapsed_frames * 18) / wav.totalPCMFrameCount;
                    memset(bar + 1, '=', progress);
                    memset(bar + 1 + progress, ' ', 18 - progress);

                    uint32_t elapsed_min = elapsed_seconds / 60, elapsed_sec = elapsed_seconds % 60;
                    uint32_t total_min = (wav.totalPCMFrameCount / wav.sampleRate) / 60;
                    uint32_t total_sec = (wav.totalPCMFrameCount / wav.sampleRate) % 60;
                    snprintf(time_display, sizeof(time_display), "%02u:%02u / %02u:%02u", elapsed_min, elapsed_sec, total_min, total_sec);

                    render_set_row(0, bar);
                    render_set_row(1, time_display);
                    render_set_row(2, playing ? "Playing" : "Paused");
                    render_set_row(3, (char*)pvParameters);
                }

                uint16_t pwmValueLeft = 0;
                uint16_t pwmValueRight = 0;

                if (wav.channels == 2) {
                    pwmValueLeft = (sampleBuffer[0] >> 8) + 128;
                    pwmValueRight = (sampleBuffer[1] >> 8) + 128;
                } else {
                    pwmValueLeft = (sampleBuffer[0] >> 8) + 128;
                    pwmValueRight = (sampleBuffer[0] >> 8) + 128;
                }
                pwm_set_gpio_level(AUDIO_PIN_LEFT, pwmValueLeft);
                pwm_set_gpio_level(AUDIO_PIN_RIGHT, pwmValueRight);
                busy_wait_us(wait_time_us);
                elapsed_frames++;
            }
            drwav_uninit(&wav);
        }
        f_close(&file);
    }
    printf("%s", "Stopping audio playback");
    playing = false;
    reset_speakers();
    musicmenu.set_as_current();
    vTaskDelete(NULL);
}

void alarm_task(void *params) {
    DateTime *alarm = (DateTime*)params;
    if (alarm == nullptr) {
        printf("%s\n", "Alarm task failed");
        vTaskDelete(NULL);
    }
    DateTime now;
    ds1302getDateTime(&now);
    printf("%s: %02u:%02u:%02u", "Alarm set on", alarm->hour, alarm->minute, alarm->second);
    // TODO: Figure out how to make an alarm
    while (true) {
        if (now.hour == alarm->hour && now.minute == alarm->minute && now.second == alarm->second) {
            break;
        }
        ds1302getDateTime(&now);
        vTaskDelay(1000);
    }

    printf("%s\n", "Alarm triggered!");
    for (int i = 100; i > 0; i--) {
        gpio_put(BUZZER_PIN, 0);
        vTaskDelay(200);
        gpio_put(BUZZER_PIN, 1);
        vTaskDelay(200);
    }
    delete alarm;
    vTaskDelete(NULL);
}

void gpio_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (current_time - last_interrupt_time < debounce_delay) {
        return;  // Ignore bouncing
    }

    last_interrupt_time = current_time;  // Update last valid interrupt time

    if (gpio == BTN_UP) {
        if (Menu::get_current() != nullptr && CURRENT_MODE == MODE_SELECT) Menu::get_current()->move_selection(-1);
    }
    else if (gpio == BTN_DOWN) {
        if (Menu::get_current() != nullptr && CURRENT_MODE == MODE_SELECT) Menu::get_current()->move_selection(1);
    }
    else if (gpio == BTN_HOME) {
        paused = !paused;
    }
    else if (gpio == BTN_SELECT) {
        //slide_clear_animation(&lcd);
        TaskHandle_t xHandle;
        xTaskCreate(launch_execute_task, "launch", 4096, NULL, tskIDLE_PRIORITY, &xHandle);
        vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);
        printf("%s\n", "Launched task!");
    }
    else if (gpio == BTN_MODE) {
        if (CURRENT_MODE == MODE_TIME) {
            CURRENT_MODE = MODE_SELECT;
            mainmenu.set_as_current();
        } else {
            CURRENT_MODE = MODE_TIME;
            render_clear_buffer();
        }
        playing = false;
        paused = false;
    }
    else if (gpio == ROTARY_A_PIN) {
        int a_state = gpio_get(ROTARY_A_PIN);
        int b_state = gpio_get(ROTARY_B_PIN);

        if (a_state == 0) {  // Only trigger when A falls
            if (b_state == 1) {
                // Clockwise movement (scroll down)
                if (Menu::get_current() != nullptr) {
                    Menu::get_current()->move_selection(-1);
                }
            } else {
                // Counterclockwise movement (scroll up)
                if (Menu::get_current() != nullptr) {
                    Menu::get_current()->move_selection(1);
                }
            }
        }
    }
    printf("%s\n", "Button callback");
}

void button_task(void *pvParameters) {
    check_core("button");

    gpio_init(BTN_UP);
    gpio_init(BTN_DOWN);
    gpio_init(BTN_HOME);
    gpio_init(BTN_SELECT);
    gpio_init(BTN_MODE);

    gpio_set_dir(BTN_UP, GPIO_IN);
    gpio_set_dir(BTN_DOWN, GPIO_IN);
    gpio_set_dir(BTN_HOME, GPIO_IN);
    gpio_set_dir(BTN_SELECT, GPIO_IN);
    gpio_set_dir(BTN_MODE, GPIO_IN);

    gpio_pull_up(BTN_UP);
    gpio_pull_up(BTN_DOWN);
    gpio_pull_up(BTN_HOME);
    gpio_pull_up(BTN_SELECT);
    gpio_pull_up(BTN_MODE);

    // Setup rotary encoder
    gpio_init(ROTARY_A_PIN);
    gpio_init(ROTARY_B_PIN);
    gpio_set_dir(ROTARY_A_PIN, GPIO_IN);
    gpio_set_dir(ROTARY_B_PIN, GPIO_IN);
    gpio_pull_up(ROTARY_A_PIN);
    gpio_pull_up(ROTARY_B_PIN);

    gpio_set_irq_enabled_with_callback(BTN_UP, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BTN_DOWN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_HOME, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_SELECT, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_MODE, GPIO_IRQ_EDGE_FALL, true);

    gpio_set_irq_enabled(ROTARY_A_PIN, GPIO_IRQ_EDGE_FALL, true);

    while (true) vTaskDelay(pdMS_TO_TICKS(500));
}

void game_tetris_task(void *params) {
    check_core("tetris");
    Tetris tetris;
    tetris.init();

    while (true) {
        tetris.tick();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void repl_task(void* pvParameters) {
    char input_buffer[64];
    int index = 0;

    while (true) {
        // Wait for USB serial connection
        while (!tud_cdc_connected()) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        tud_cdc_write_str("\n> ");  // Print prompt
        tud_cdc_write_flush();

        // Read input
        while (true) {
            if (tud_cdc_available()) {
                char c = tud_cdc_read_char();
                if (c == '\r' || c == '\n') {  // Enter key pressed
                    input_buffer[index] = '\0';
                    tud_cdc_write_str("\n");
                    tud_cdc_write_flush();
                    break;
                } else if (index < (sizeof(input_buffer) - 1)) {
                    input_buffer[index++] = c;
                    tud_cdc_write_char(c);  // Echo back
                    tud_cdc_write_flush();
                }
            }
            vTaskDelay(pdMS_TO_TICKS(10));  // Reduce CPU usage
        }

        // Parse command
        if (strncmp(input_buffer, "set_time", 8) == 0) {
            tud_cdc_write_str("Setting time...\n");
            // Example: set_time 2025/02/08 14:30:00
            int year, month, day, hour, minute, second;
            char date_str[11], time_str[9];
            int n =sscanf(input_buffer + 9, "%10s %8s", date_str, time_str);

            if (n == 2) {
                // Parse date: yyyy/mm/dd
                if (sscanf(date_str, "%d/%d/%d", &year, &month, &day) == 3 &&
                    sscanf(time_str, "%d:%d:%d", &hour, &minute, &second) == 3) {
                    // Set time on DS1302
                    tud_cdc_write_str("Setting time...\n");
                    set_datetime(year - 2000, month, day, hour, minute, second);
                    tud_cdc_write_str("Time set!\n");
                    tud_cdc_write_str("Current time: ");
                    tud_cdc_write_str(get_time_string().c_str());
                    tud_cdc_write_char('\n');
                } else {
                    tud_cdc_write_str("Invalid date or time format. Use yyyy/mm/dd hh:mm:ss.\n");
                }
            } else {
                tud_cdc_write_str("Invalid command format. Use: set_time yyyy/mm/dd hh:mm:ss\n");
            }
        } else if (strcmp(input_buffer, "get_time") == 0) {
            tud_cdc_write_str("Fetching time...\n");
            tud_cdc_write_str(get_time_string().c_str());
            tud_cdc_write_char('\n');
        } else if (strcmp(input_buffer, "scan_files") == 0) {
            tud_cdc_write_str("Scanning files...\n");
            scan_files();
            for (int i = 0; i < file_count; i++) {
                tud_cdc_write_str(files[i]);
                tud_cdc_write_char('\n');
            }
        } else if (std::strncmp(input_buffer, "set_alarm", 9) == 0) {
            tud_cdc_write_str("Setting alarm...\n");
            // Example: set_alarm 14:30:00
            uint8_t hour, minute, second;
            char time_str[9];
            int n = sscanf(input_buffer + 10, "%8s", time_str);
            if (n == 1) {
                // Parse time: hh:mm:ss
                if (sscanf(time_str, "%d:%d:%d", &hour, &minute, &second) == 3) {
                    // Set alarm on DS1302
                    DateTime *alarm = new DateTime();
                    alarm->year = 0;
                    alarm->month = 0;
                    alarm->day = 0;
                    alarm->hour = hour;
                    alarm->minute = minute;
                    alarm->second = second;
                    xTaskCreate(alarm_task, "alarm", 512, alarm, tskIDLE_PRIORITY, NULL);
                    tud_cdc_write_str("Alarm set!\n");
                } else {
                    tud_cdc_write_str("Invalid time format. Use hh:mm:ss.\n");
                }
            }
        } else if (std::strcmp(input_buffer, "start_game") == 0) {
            tud_cdc_write_str("Starting game...\n");
            xTaskCreate(game_tetris_task, "game", 4096, NULL, tskIDLE_PRIORITY, NULL);
            Menu::remove_current();
        } else if (std::strcmp(input_buffer, "reset_display") == 0) {
            tud_cdc_write_str("Resetting display...\n");
            lcd.DisplayOff();
            lcd.BacklightOff();
            sleep_ms(5000);
            lcd.BacklightOn();
            lcd.DisplayOn();
            tud_cdc_write_str("Display reset successfull\n");
        } else {
            tud_cdc_write_str("Unknown command\n");
        }
        tud_cdc_write_flush();
        index = 0;  // Reset input
    }
}

int main() {
    lcd.BacklightOn();

    OutputList olist(&lcd);

    pwm_config backlight_pwm_config = pwm_get_default_config();
    stdio_init_all();

    // pwm_config_set_clkdiv(&backlight_pwm_config, 2.0f);
    // pwm_config_set_wrap(&backlight_pwm_config, 250);
    gpio_set_function(BRIGHTNESS_PIN, GPIO_FUNC_PWM);
    uint brightness_pin_slice = pwm_gpio_to_slice_num(BRIGHTNESS_PIN);
    pwm_init(brightness_pin_slice, &backlight_pwm_config, true);
    pwm_set_gpio_level(BRIGHTNESS_PIN, 65535/4);

    // Setup buzzer
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, true);
    gpio_put(BUZZER_PIN, 1);

    // gpio_init(BRIGHTNESS_PIN);
    // gpio_set_dir(BRIGHTNESS_PIN, true);
    // // gpio_set_pulls(BRIGHTNESS_PIN, true, false);
    // gpio_put(BRIGHTNESS_PIN, true);

    olist.push("Starting...");
    olist.render();

    {
        bool clock_set = set_sys_clock_khz(180000, true);
        setup_default_uart();
        if (!clock_set) {
            olist.push("Overclock failed");
            olist.render();
            return -1;
        }
    }
    olist.push("Overclock success");
    olist.render();

    printf("%s", "Setting up Pico modules\n");

    ds1302init(0, 2, 1);
    
    // writeProtect(0);
    // DateTime now = {
    //     .year = 25,
    //     .month = 2,
    //     .day = 7,
    //     .hour = 17,
    //     .minute = 30,
    //     .second = 0,
    //     .dow = DOW_FRI
    // };
    // ds1302setDateTime(&now);

    rtc_init();
    DateTime date;
    ds1302getDateTime(&date);
    datetime_t t = {
        .year = (int8_t)date.year,
        .month = (int8_t)date.month,
        .day = (int8_t)date.day,
        .hour = (int8_t)date.hour,
        .min = (int8_t)date.minute,
        .sec = (int8_t)date.second
    };
    set_local_datetime(&t);
    if (!rtc_running()) {
        olist.push("RTC failed");
        olist.render();
        sleep_ms(1000);
    }
    olist.push("RTC success");
    olist.render();

    adc_init();
    adc_set_temp_sensor_enabled(true);
    // cyw43_arch_init();
    // gpio_init(CYW43_WL_GPIO_LED_PIN);

    if (!sd_init_driver()) {
        olist.push("SD failed");
        olist.render();
        sleep_ms(1000);
        return -2;
    }
    olist.push("SD success");
    olist.render();

    FRESULT fr;
    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        lcd.SetCursor(1, 0);
        lcd.PrintString("SD mount failed");
        sleep_ms(1000);
        return -3;
    }

    olist.push("SD mount success");
    olist.render();

    // pwm_clear_irq(audio_pin_slice);
    // pwm_set_irq_enabled(audio_pin_slice, true);

    // irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_handler);
    // irq_set_enabled(PWM_IRQ_WRAP, true);

    // pwm_config pwm_config = pwm_get_default_config();

    /* Base clock 176,000,000 Hz divide by wrap 250 then the clock divider
     * further divides to set the interrupt rate.
     *
     * 11 KHz is fine for speech. Phone lines generally sample at 8 KHz
     *
     *
     * So clkdiv should be as follows for given sample rate
     *  8.0f for 11 KHz
     *  4.0f for 22 KHz
     *  2.0f for 44 KHz etc
    */
    // pwm_config_set_clkdiv(&pwm_config, 20.0f);


    gpio_set_function(AUDIO_PIN_LEFT, GPIO_FUNC_PWM);
    uint audio_pin_left_slice = pwm_gpio_to_slice_num(AUDIO_PIN_LEFT);
    pwm_set_wrap(audio_pin_left_slice, 255);
    pwm_set_clkdiv(audio_pin_left_slice, 2.44f);
    pwm_set_enabled(audio_pin_left_slice, true);

    gpio_set_function(AUDIO_PIN_RIGHT, GPIO_FUNC_PWM);
    uint audio_pin_right_slice = pwm_gpio_to_slice_num(AUDIO_PIN_RIGHT);
    pwm_set_wrap(audio_pin_right_slice, 255);
    pwm_set_clkdiv(audio_pin_right_slice, 2.44f);
    pwm_set_enabled(audio_pin_right_slice, true);

    pwm_set_gpio_level(AUDIO_PIN_LEFT, 0);
    pwm_set_gpio_level(AUDIO_PIN_RIGHT, 0);

    scan_files();

    printf("%s", "No errors reported\n");
    olist.push("No errors reported");
    olist.render();

    // FIXME: Temp code

    int i = 1234;
    mainmenu.add_option("Test", [](void* p) { printf("%s\n", "Test"); }, &i);
    mainmenu.add_option("Alarm", [](void* p) {
        printf("%s\n", "Alarm Menu");
        alarmmenu.set_as_current();
    }, nullptr);
    mainmenu.add_option("Settings", [](void* p) {
        printf("%s\n", "Settings");
        settingsmenu.set_as_current();
    }, nullptr);
    mainmenu.add_option("Testus2", [](void* p) { printf("%s\n", "Testus2"); }, nullptr);
    mainmenu.add_option("Testus3", [](void* p) { printf("%s\n", "Testus3"); }, nullptr);
    mainmenu.add_option("Music", [](void* p) {
        printf("%s\n", "Pressed music");
        musicmenu.set_as_current();
        printf("%s\n", "Set music menu as current");
    }, nullptr);
    mainmenu.set_as_current();

    printf("%s: %i\n", "File count", file_count);
    for (int i = 0; i < file_count; i++) {
        printf("%s %s\n", "Adding menu", files[i]);
        int *f_num = new int(i);
        musicmenu.add_option(files[i], [](void* p) {
            int *f = (int*)p;
            printf("%s: %i\n", "Playing music track", f);
            TaskHandle_t xHandle;
            // xTaskCreate(play_wav_task, "PlayWav", 4096, (void*)files[2], 2, &xHandle);
            xTaskCreate(play_wav_task, "PlayWav", 4096, (void*)files[*f], 2, &xHandle);
            vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_1);
            Menu::remove_current();
            // render_clear_buffer();
            lcd.Clear();
        }, (void*)f_num);
    }
    musicmenu.add_option("Back", [](void *p) {
        mainmenu.set_as_current();
    }, nullptr);

    // TODO: Debug why sometimes on a key press in the time change functions crashes the board
    uint8_t alarm_hour = 0;
    alarmmenu.add_option("Hour: 0", [&alarm_hour](void *p) {
        uint8_t before = alarm_hour;
        alarm_hour++;
        if (alarm_hour > 23) alarm_hour = 0;
        alarmmenu.modify_option("Hour: " + std::to_string(before), "Hour: " + std::to_string(alarm_hour));
        printf("%s: %s\n", "Modified option", std::string("Hour: " + std::to_string(alarm_hour)).c_str());
    }, nullptr);

    uint8_t alarm_minute = 0;
    alarmmenu.add_option("Minute: 0", [&alarm_minute](void *p) {
        uint8_t before = alarm_minute;
        alarm_minute++;
        if (alarm_minute > 59) alarm_minute = 0;
        alarmmenu.modify_option("Minute: " + std::to_string(before), "Minute: " + std::to_string(alarm_minute));
        printf("%s: %s\n", "Modified option", std::string("Minute: " + std::to_string(alarm_minute)).c_str());
    }, nullptr);

    uint8_t alarm_second = 0;
    alarmmenu.add_option("Second: 0", [&alarm_second](void *p) {
        uint8_t before = alarm_second;
        alarm_second++;
        if (alarm_second > 59) alarm_second = 0;
        alarmmenu.modify_option("Second: " + std::to_string(before), "Second: " + std::to_string(alarm_second));
        printf("%s: %s\n", "Modified option", std::string("Second: " + std::to_string(alarm_second)).c_str());
    }, nullptr);

    alarmmenu.add_option("Set", [&alarm_hour, &alarm_minute, &alarm_second](void *p) {
        DateTime *alarm = new DateTime();
        alarm->hour = alarm_hour;
        alarm->minute = alarm_minute;
        alarm->second = alarm_second;
        TaskHandle_t xHandle;
        xTaskCreate(alarm_task, "alarm", 1024, (void*)alarm, tskIDLE_PRIORITY, &xHandle);
        vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);
        alarm_handles.push_back(xHandle);
    }, nullptr);
    alarmmenu.add_option("Remove", [](void *p) {
        printf("%s\n", "Remove all alarms");
        for (int i = 0; i < alarm_handles.size(); i++) {
            vTaskDelete(alarm_handles[i]);
        }
        alarm_handles.clear();
    }, nullptr);
    alarmmenu.add_option("Back", [](void *p) {
        mainmenu.set_as_current();
    }, nullptr);

    // sleep_ms(2000);
    lcd.Clear();
    lcd.Home();
    render_set_row(0, "-=-=-=-=-=-=-=-=-=-=", lcd);
    render_set_row(1, "|     BitBand      |", lcd);
    render_set_row(2, "|    by Bambuko    |", lcd);
    render_set_row(3, "=-=-=-=-=-=-=-=-=-=-", lcd);
    sleep_ms(1000);

    TaskHandle_t xHandle;

    // xTaskCreate(main_thread, "main", 1024, &lcd, tskIDLE_PRIORITY + 4UL, nullptr);
    // xTaskCreate(blink_led, "blink", 128, NULL, tskIDLE_PRIORITY, &xHandle);
    // vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);

    // xTaskCreate(log_time, "log_time", 256, NULL, tskIDLE_PRIORITY, &xHandle);
    // vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);

    // xTaskCreate(log_accel, "log_accel", 1024, nullptr, tskIDLE_PRIORITY, &xHandle);
    // vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);

    xTaskCreate(button_task, "button_input", 1024, NULL, tskIDLE_PRIORITY, &xHandle);
    vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);

    xTaskCreate(render, "render", 1024, nullptr, tskIDLE_PRIORITY, &xHandle);
    vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);

    xTaskCreate(repl_task, "repl", 1024, nullptr, tskIDLE_PRIORITY, &xHandle);
    vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);

    printf("%s\n", "All tasks set up successfully");

    vTaskStartScheduler();

    while (true) {
        tight_loop_contents();
    }
    return 0;
}
