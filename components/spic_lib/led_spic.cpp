#include "led_spic.h"

int8_t Spic_LED::sb_led_init(void) {
    for (size_t i = 0; i < 8; ++i) {
        gpio_config_t led_config = {};
        led_config.pin_bit_mask = (1ULL << gpio[i]);
        led_config.mode         = GPIO_MODE_INPUT_OUTPUT;
        led_config.pull_up_en   = GPIO_PULLUP_DISABLE;
        led_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        led_config.intr_type    = GPIO_INTR_DISABLE;
        if (gpio_config(&led_config) != ESP_OK) {
            return -1;
        }

        if (sb_led_off(static_cast<spic_led_color>(i)) != ESP_OK) {
            return -1;
        }
    }
    return 0;
}

int8_t Spic_LED::sb_led_on(spic_led_color color) {
    if (gpio_set_level(gpio[color], 0) != ESP_OK) {
        return -1;
    };

    return 0;
}

int8_t Spic_LED::sb_led_off(spic_led_color color) {
    if (gpio_set_level(gpio[color], 1) != ESP_OK) {
        return -1;
    };
    return 0;
}

int8_t Spic_LED::sb_led_toggle(spic_led_color color) {
    int8_t level = gpio_get_level(gpio[color]);
    if (gpio_set_level(gpio[color], !level) != ESP_OK) {
        return -1;
    }
    return 0;
}

int8_t Spic_LED::led_showLevel(uint16_t level, uint16_t max) {
    if (max == 0) {
        return -2;
    }
    if (max < level) {
        return -1;
    }
    uint32_t temp = (static_cast<uint32_t>(level) * 8) / max;
    uint8_t leds_on = static_cast<uint8_t>(temp);

    for (size_t i = 0; i < 8; ++i) {
        if (i < leds_on) {
            if (sb_led_on(static_cast<spic_led_color>(i)) != ESP_OK) {
                return -1;
            }
        } else {
            if (sb_led_off(static_cast<spic_led_color>(i)) != ESP_OK) {
                return -1;
            }
        }
    }

    return leds_on;
}

void Spic_LED::led_setMask(uint8_t mask) {
    for (size_t i = 0; i < 8; ++i) {
        if (mask & (1 << i)) {
            if (sb_led_on(static_cast<spic_led_color>(i)) != ESP_OK) {
                return;
            }
        } else {
            if (sb_led_off(static_cast<spic_led_color>(i)) != ESP_OK) {
                return;
            }
        }
    }
}

uint8_t Spic_LED::led_getMask(void) {
    uint8_t mask = 0;
    for (size_t i = 0; i < 8; ++i) {
        if (gpio_get_level(gpio[i]) == 0) {
            mask |= (1 << i);
        }
    }
    return mask;
}