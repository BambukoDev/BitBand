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
#include <memory>
#include "mp3dec.h"

extern "C" {
    #include "ff.h"
    #include "sd_card.h"
    #include "music_file.h"

    #define DR_WAV_IMPLEMENTATION
    #include "dr_wav.h"

    // #include "ds1302.h"
}

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
#include "button.h"

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

// #include "soundfile.h"

#define MAX_FILE_COUNT 100
FATFS fs;
bool playing = false;
bool paused = false;
char files[MAX_FILE_COUNT][32];
int file_count = 0;
int selected_file = 0;

#define BRIGHTNESS_PIN 6
#define AUDIO_PIN 9

Menu mainmenu;
Menu musicmenu;

void set_rtc_datetime(datetime_t* t = nullptr) {
    if (t == nullptr) return;
    rtc_set_datetime(t);
    sleep_us(128); // wait for the clock to take change (around 3 clock cycles)
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

    while (true) {
        // if (Menu::get_current() != nullptr) Menu::get_current()->render();
        if (Menu::get_current()) Menu::get_current()->render();
        render_display(lcd);
        vTaskDelay(10);
    }
}

void blink_led(void* params) {
    LedManager led;
    uint16_t speed = 100;
    led.setup_led(&speed);
    while (true) {
        led.blink_led();
        // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
        vTaskDelay(500);
        // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
        vTaskDelay(500);
    }
}

void log_time(void* params) {
    while(true) {
        datetime_t t;
        if (!rtc_get_datetime(&t)) {
            printf("%s", "RTC not running!\n");
        }
        // printf("%d/%d/%d %d:%d:%d\n", t.year, t.month, t.day, t.hour, t.min, t.sec);
        vTaskDelay(1000);
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

            char bar[22] = "[                   ]";
            char time_display[16];

            while (playing && drwav_read_pcm_frames(&wav, 1, sampleBuffer) > 0) {
                while (paused) vTaskDelay(10);

                if (wav.channels == 2) {
                    outputSample = ((int16_t)sampleBuffer[0] + (int16_t)sampleBuffer[1]) / 2;
                } else {
                    outputSample = sampleBuffer[0];
                }
                uint8_t pwmValue = (outputSample >> 8) + 128;
                pwm_set_gpio_level(AUDIO_PIN, pwmValue);
                busy_wait_us(880000 / wav.sampleRate);
                elapsed_frames++;

                uint32_t elapsed_seconds = elapsed_frames / wav.sampleRate;
                if (elapsed_seconds != last_update_time) {
                    last_update_time = elapsed_seconds;

                    int progress = (elapsed_frames * 20) / wav.totalPCMFrameCount;
                    memset(bar + 1, '=', progress);
                    memset(bar + 1 + progress, ' ', 20 - progress);

                    uint32_t elapsed_min = elapsed_seconds / 60, elapsed_sec = elapsed_seconds % 60;
                    uint32_t total_min = (wav.totalPCMFrameCount / wav.sampleRate) / 60;
                    uint32_t total_sec = (wav.totalPCMFrameCount / wav.sampleRate) % 60;
                    snprintf(time_display, sizeof(time_display), "%02u:%02u / %02u:%02u", elapsed_min, elapsed_sec, total_min, total_sec);

                    render_set_row(0, bar);
                    render_set_row(1, time_display);
                    render_set_row(2, playing ? "Playing" : "Paused");
                    render_set_row(3, (char*)pvParameters);
                }
            }
            drwav_uninit(&wav);
        }
        f_close(&file);
    }
    playing = false;
    musicmenu.set_as_current();
    vTaskDelete(NULL);
}

void logic_task() {
    check_core("logic");
}

void lcd_update() {
    lcd.PrintString("Select track:");
    render_set_row(0, "Select track:");
    for (int i = 0; i < file_count && i < 3; i++) {
        std::string line = (i == selected_file ? "> " : "  ") + std::string(files[i]);
        render_set_row(i + 1, line);
    }
}

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BTN_UP) Menu::get_current()->move_selection(-1);
    else if (gpio == BTN_DOWN) Menu::get_current()->move_selection(1);
    else if (gpio == BTN_HOME) {
        playing = false;
        mainmenu.set_as_current();
    }
    else if (gpio == BTN_MODE) paused = !paused;
    else if (gpio == BTN_SELECT) {
        //slide_clear_animation(&lcd);
        TaskHandle_t xHandle;
        xTaskCreate(launch_execute_task, "launch", 1024, NULL, tskIDLE_PRIORITY, &xHandle);
        vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);
        printf("%s\n", "Launched task!");
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

    gpio_set_irq_enabled_with_callback(BTN_UP, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BTN_DOWN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_HOME, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_SELECT, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_MODE, GPIO_IRQ_EDGE_FALL, true);

    while (true) vTaskDelay(pdMS_TO_TICKS(500));
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

    rtc_init();
    datetime_t t = { .year = 2025, .month = 2, .day = 5, .hour = 12, .min = 00, .sec = 00};
    set_rtc_datetime(&t);
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


    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    uint audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    pwm_set_wrap(audio_pin_slice, 255);
    pwm_set_enabled(audio_pin_slice, true);
    // pwm_init(audio_pin_slice, nullptr, true);

    pwm_set_gpio_level(AUDIO_PIN, 0);

    scan_files();

    printf("%s", "No errors reported\n");
    olist.push("No errors reported");
    olist.render();

    // FIXME: Temp code

    int i = 1234;
    mainmenu.add_option("Test", [](void* p) { printf("%s\n", "Test"); }, &i);
    mainmenu.add_option("Testus", [](void* p) { printf("%s\n", "Testus"); }, nullptr);
    mainmenu.add_option("Testus1", [](void* p) { printf("%s\n", "Testus1"); }, nullptr);
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
        musicmenu.add_option(files[i], [](void* p) {
            int f = *(int*)p;
            printf("%s: %i\n", "Playing music track", f);
            TaskHandle_t xHandle;
            xTaskCreate(play_wav_task, "PlayWav", 4096, (void*)files[2], 2, &xHandle);
            vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_1);
            Menu::remove_current();
            render_clear_buffer();
        }, (void*)&i);
    }

    // sleep_ms(2000);
    lcd.Clear();
    lcd.Home();
    timed_print(&lcd, "Test string!", 12, 50);

    TaskHandle_t xHandle;

    // xTaskCreate(main_thread, "main", 1024, &lcd, tskIDLE_PRIORITY + 4UL, nullptr);
    xTaskCreate(blink_led, "blink", 128, NULL, tskIDLE_PRIORITY, &xHandle);
    vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);

    xTaskCreate(log_time, "log_time", 256, NULL, tskIDLE_PRIORITY, &xHandle);
    vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);

    xTaskCreate(button_task, "button_input", 256, NULL, tskIDLE_PRIORITY, &xHandle);
    vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);

    xTaskCreate(render, "render", 1024, nullptr, tskIDLE_PRIORITY, &xHandle);
    vTaskCoreAffinitySet(xHandle, CORE_AFFINITY_0);

    printf("%s\n", "All tasks set up successfully");

    vTaskStartScheduler();
    return 0;
}