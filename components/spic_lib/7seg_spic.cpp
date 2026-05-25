#include "7seg_spic.h"

int8_t Spic_7seg::sb_7seg_init(void) {
    gpio_config_t io_config = {};
    io_config.pin_bit_mask = (1ULL << _data_gpio) | (1ULL << _clk_gpio) | (1ULL << _latch_gpio);
    io_config.mode         = GPIO_MODE_OUTPUT;
    io_config.pull_up_en   = GPIO_PULLUP_DISABLE;
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.intr_type    = GPIO_INTR_DISABLE;
    if (gpio_config(&io_config) != ESP_OK) {
        return -1;
    }

    _display_data[0] = 0x00;
    _display_data[1] = 0x00;
    _enabled = false;
    _current_digit = 0;

    esp_timer_create_args_t timer_args = {};
    timer_args.callback = _timer_callback;
    timer_args.arg = this;
    timer_args.name = "7seg_spic_timer";

    if (esp_timer_create(&timer_args, &_timer_handle) != ESP_OK) {
        return -1;
    }

    if (esp_timer_start_periodic(_timer_handle, 1000) != ESP_OK) {
        return -1;
    }

    return 0;
}

int8_t Spic_7seg::sb_7seg_showNumber(int8_t num) {
    if (num < -9 || num > 99) {
        return -1;
    }

    if (num < 0) {
        _display_data[0] = char_to_7seg('-');
        _display_data[1] = char_to_7seg('0' + (-num));
    } else {
        uint8_t z = num / 10;
        uint8_t e = num % 10;

        _display_data[0] = (z == 0) ? 0x00 : char_to_7seg('0' + z);
        _display_data[1] = char_to_7seg('0' + e);
    }

    _enabled = true;
    
    return 0;
}

int8_t Spic_7seg::sb_7seg_showHexNumber(uint8_t nmbr) {
    const char hex_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    _display_data[0] = char_to_7seg(hex_chars[nmbr >> 4]);
    _display_data[1] = char_to_7seg(hex_chars[nmbr & 0x0F]);
    _enabled = true;
    return 0;
}

int8_t Spic_7seg::sb_7seg_showString(const char *str) {
    if (!str || !str[0]) {
        return -1;
    }
    _display_data[0] = char_to_7seg(str[0]);
    _display_data[1] = (str[1] != '\0') ? char_to_7seg(str[1]) : 0x00;
    _enabled = true;
    return 0;
}

void Spic_7seg::sb_7seg_disable(void) {
    _enabled = false;
}

void Spic_7seg::shift_out(uint8_t val) {
    for (int i = 7; i >= 0; --i) {
        gpio_set_level(_clk_gpio, 0);
        gpio_set_level(_data_gpio, (val >> i) & 1);
        gpio_set_level(_clk_gpio, 1);
    }
}

void Spic_7seg::update_display(void) {
    if (!_enabled) {
        gpio_set_level(_latch_gpio, 0);
        shift_out(0xFF);
        gpio_set_level(_latch_gpio, 1);
        return;
    }

    _current_digit = !_current_digit;

    uint8_t mask = _display_data[_current_digit];
    uint8_t actual_out = ~mask;

    if (_current_digit == 1) {
        actual_out |= (1 << 7);
    } else {
        actual_out &= ~(1 << 7);
    }

    gpio_set_level(_latch_gpio, 0);
    shift_out(actual_out);
    gpio_set_level(_latch_gpio, 1);
}

void Spic_7seg::_timer_callback(void *arg) {
    Spic_7seg* instance = static_cast<Spic_7seg*>(arg);
    instance->update_display();
}


uint8_t Spic_7seg::char_to_7seg(char c) {
    switch (c) {
        case '0': case 'O': return 0b00111111; 
        case '1': return 0b00000110; 
        case '2': return 0b01011011; 
        case '3': return 0b01001111; 
        case '4': return 0b01100110; 
        case '5': case 'S': case 's': return 0b01101101; 
        case '6': return 0b01111101; 
        case '7': return 0b00000111; 
        case '8': return 0b01111111; 
        case '9': return 0b01101111; 
        case 'A': case 'a': return 0b01110111; 
        case 'b': return 0b01111100; 
        case 'C': return 0b00111001; 
        case 'c': return 0b01011000; 
        case 'd': return 0b01011110; 
        case 'E': case 'e': return 0b01111001; 
        case 'F': case 'f': return 0b01110001; 
        case 'H': return 0b01110110; 
        case 'h': return 0b01110100; 
        case 'L': case 'l': return 0b00111000; 
        case 'n': return 0b01010100; 
        case 'o': return 0b01011100; 
        case 'P': case 'p': return 0b01110011; 
        case 'U': return 0b00111110; 
        case 'u': return 0b00011100; 
        case '-': return 0b01000000; 
        case '_': return 0b00001000; 
        case ' ': default: return 0b00000000; 
    }
}