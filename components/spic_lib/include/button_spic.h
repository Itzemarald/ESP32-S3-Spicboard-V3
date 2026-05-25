#pragma once

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_timer.h"
#include <stdint.h>
#include "freertos/timers.h"

class Spic_Button {
public:
    enum BUTTON {
        BUTTON0,
        BUTTON1
    };

    enum BUTTONSTATE {
        PRESSED,
        RELEASED,
        UNKNOWN
    };

    enum BUTTONEVENT {
        ONPRESS,
        ONRELEASE
    };

    Spic_Button(void) = default;
    ~Spic_Button(void) = default;

    typedef void(*BUTTONCALLBACK) (BUTTON, BUTTONEVENT);

    int8_t sb_button_init(void);
    
    int8_t sb_button_registerCallback(BUTTON btn, BUTTONEVENT event, BUTTONCALLBACK callback);
    int8_t sb_button_unregisterCallback(BUTTON btn, BUTTONEVENT event, BUTTONCALLBACK callback);
    BUTTONSTATE sb_button_getState(BUTTON btn);
private:
    const gpio_num_t _gpio[2] = {GPIO_NUM_7, GPIO_NUM_6};

    BUTTONCALLBACK _callbacks[2][2] = { {nullptr, nullptr}, {nullptr, nullptr} };
    struct IsrContext {
        Spic_Button* instance;
        BUTTON btn;
    };
    IsrContext _isr_contexts[2];

    TimerHandle_t _debounce_timers[2];

    static void gpio_isr_wrapper(void* arg);
    void handle_interrupt(BUTTON btn);
    static void timer_callback(TimerHandle_t xTimer);
};