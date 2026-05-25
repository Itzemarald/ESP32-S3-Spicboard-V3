#pragma once

#include "driver/gpio.h"
#include "esp_timer.h"
#include "stdint.h"

class Spic_7seg {
public:
    Spic_7seg(void) = default;
    ~Spic_7seg(void) = default;

    int8_t sb_7seg_init(void);
    
    int8_t sb_7seg_showNumber(int8_t nmbr);
    int8_t sb_7seg_showHexNumber(uint8_t nmbr);
    int8_t sb_7seg_showString(const char *str);
    void sb_7seg_disable(void);
private:
    const gpio_num_t _data_gpio = GPIO_NUM_16;
    const gpio_num_t _clk_gpio = GPIO_NUM_15;
    const gpio_num_t _latch_gpio = GPIO_NUM_17;

    uint8_t _display_data[2];
    bool _enabled;
    uint64_t _current_digit;

    esp_timer_handle_t _timer_handle;
    static void _timer_callback(void *arg);

    void shift_out(uint8_t val);
    void update_display(void);
    uint8_t char_to_7seg(char c);
};