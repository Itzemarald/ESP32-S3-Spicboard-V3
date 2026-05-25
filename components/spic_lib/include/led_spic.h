#pragma once

#include "driver/gpio.h"
#include <stdint.h>

class Spic_LED{
public:
enum spic_led_color{
    RED0,           // 10
    YELLOW0,        // 11
    GREEN0,         // 12
    BLUE0,          // 9
    RED1,           // 8
    YELLOW1,        // 18
    GREEN1,         // 13
    BLUE1           // 14
};
    Spic_LED(void) = default;
    ~Spic_LED(void) = default;

    int8_t sb_led_init(void);
    
    int8_t sb_led_on(spic_led_color color);
    int8_t sb_led_off(spic_led_color color);
    int8_t sb_led_toggle(spic_led_color color);
    int8_t led_showLevel(uint16_t level, uint16_t max);
    void led_setMask(uint8_t mask);
    uint8_t led_getMask(void);
private:
    const gpio_num_t gpio[8] = {
        GPIO_NUM_10, GPIO_NUM_11, GPIO_NUM_12, GPIO_NUM_9, 
        GPIO_NUM_8,  GPIO_NUM_18, GPIO_NUM_13, GPIO_NUM_14
    };
};