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
#include <hardware/sync.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <pico/types.h>
#include <pico/util/datetime.h>
#include <sys/time.h>
#include <sys/types.h>

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

// User includes
#include "OutputList.hpp"
#include "LedPulse.hpp"

#include "soundfile.h"

#define CACHE_BUFFER 8000
unsigned char cache_buffer[CACHE_BUFFER];
static music_file mf;

#define AUDIO_PIN 9
int wav_position = 0;

void set_rtc_datetime(datetime_t* t = nullptr) {
    if (t == nullptr) return;
    rtc_set_datetime(t);
    sleep_us(128); // wait for the clock to take change (around 3 clock cycles)
}

void pwm_interrupt_handler() {
    // TODO: Figure out why this fixed pause works for 8kHz audio
    sleep_us(16);
    if (wav_position < (WAV_DATA_LENGTH<<3) - 1) {
        pwm_set_gpio_level(AUDIO_PIN, WAV_DATA[wav_position>>3]);
        wav_position++;
    } else {
        wav_position = 0;
    }
}

void blink_led(void* params) {
    uint16_t blink_speed = 10;
    LedManager led;
    led.setup_led(&blink_speed);
    while (true) {
        // led.blink_led();
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
        vTaskDelay(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
        vTaskDelay(500);
    }
}

void log_time(void* params) {
    while(true) {
        datetime_t t;
        if (!rtc_get_datetime(&t)) {
            printf("%s", "RTC not running!\n");
        }
        printf("%d/%d/%d %d:%d:%d\n", t.year, t.month, t.day, t.hour, t.min, t.sec);
        vTaskDelay(1000);
    }
}

void test_process(void* params) {
    // auto lcd = static_cast<LCD_I2C*>(params);
    while (true) {
        vCoRoutineSchedule();
    }
}

void play_audio(CoRoutineHandle_t xHandle, UBaseType_t uxIndex) {
    crSTART(xHandle);
    while (true) {
        pwm_interrupt_handler();
    }
    crEND();
}

void main_thread(void* params) {
    LCD_I2C lcd = LCD_I2C(0x27, 20, 4, PICO_DEFAULT_I2C_INSTANCE(), PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);
    OutputList olist(&lcd);

    OutputList olist(&lcd);

    lcd.BacklightOn();
    olist.push("Starting...");
    olist.render();

    {
        bool clock_set = set_sys_clock_khz(176000, true);
        setup_default_uart();
        if (!clock_set) {
            olist.push("Overclk failed");
            olist.render();
            return -1;
        }
    }
    olist.push("Overclk success");
    olist.render();

    printf("%s", "Setting up Pico modules\n");
    stdio_init_all();

    rtc_init();
    datetime_t t = {
        .year = 2025, .month = 1, .day = 8, .hour = 16, .min = 00, .sec = 00};
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
    cyw43_arch_init();
    gpio_init(CYW43_WL_GPIO_LED_PIN);

    if (!sd_init_driver()) {
        olist.push("SD failed");
        olist.render();
        sleep_ms(1000);
        return -2;
    }
    olist.push("SD success");
    olist.render();

    // TODO: Implement a scrolling list for loading text for submodules

    FATFS filesystem;
    FRESULT fr;
    fr = f_mount(&filesystem, "0:", 1);
    if (fr != FR_OK) {
        lcd.SetCursor(1, 0);
        lcd.PrintString("SD mount failed");
        sleep_ms(1000);
        return -3;
    }

    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    uint audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);

    // pwm_clear_irq(audio_pin_slice);
    // pwm_set_irq_enabled(audio_pin_slice, true);

    // irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_handler);
    // irq_set_enabled(PWM_IRQ_WRAP, true);

    pwm_config pwm_config = pwm_get_default_config();

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
    // pwm_config_set_wrap(&pwm_config, 250);
    pwm_init(audio_pin_slice, &pwm_config, false);
    pwm_set_enabled(audio_pin_slice, true);

    pwm_set_gpio_level(AUDIO_PIN, 0);

    olist.push("No errors reported");
    olist.render();
    // lcd.SetCursor(1, 0);
    printf("%s", "No errors reported");
    // timed_print(&lcd, "No errors reported", 18, 50);

    sleep_ms(2000);
    lcd.Clear();
    lcd.Home();
    timed_print(&lcd, "Test string!", 12, 50);
}

int main() {
    constexpr auto I2C = PICO_DEFAULT_I2C_INSTANCE();
    constexpr auto SDA = PICO_DEFAULT_I2C_SDA_PIN;
    constexpr auto SCL = PICO_DEFAULT_I2C_SCL_PIN;
    constexpr auto I2C_ADDRESS = 0x27;
    constexpr auto LCD_COLUMNS = 20;
    constexpr auto LCD_ROWS = 4;

    LCD_I2C lcd = LCD_I2C(I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS, I2C, SDA, SCL);

    

    TaskHandle_t blink_handle = nullptr;

    // MP3InitDecoder();
    // if (!musicFileCreate(&mf, "example.mp3", cache_buffer, CACHE_BUFFER)) {
    //     lcd.SetCursor(2, 0);
    //     lcd.PrintString("Cannot open music!");
    // }

    // bool playing = true;
    // while (playing) {
    //     MP3FrameInfo frame_info;
    //     unsigned char buf[1024];
    //     MP3GetNextFrameInfo(mf.hMP3Decoder, &frame_info, buf);
        
    // }

    // pwm_set_enabled(AUDIO_PIN, true);

    xTaskCreate(blink_led, "blink", 1024, nullptr, tskIDLE_PRIORITY, &blink_handle);
    xTaskCreate(log_time, "log_time", 1024, nullptr, tskIDLE_PRIORITY, nullptr);
    xTaskCreate(test_process, "test_process", 1024, &lcd, tskIDLE_PRIORITY, nullptr);
    xCoRoutineCreate(play_audio, 0, 0);

    vTaskStartScheduler();
    return 0;
}