#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "button_spic.h"

static bool isr_service_installed = false;

int8_t Spic_Button::sb_button_init(void) {
    if (!isr_service_installed) {
        gpio_install_isr_service(0);
        isr_service_installed = true;
    }

    for (size_t i = 0; i < 2; ++i) {
        _isr_contexts[i].instance = this;
        _isr_contexts[i].btn = static_cast<BUTTON>(i);
        
        _debounce_timers[i] = xTimerCreate("btn_tmr", pdMS_TO_TICKS(50), pdFALSE, &_isr_contexts[i], timer_callback);

        gpio_config_t button_config = {};
        button_config.pin_bit_mask = (1ULL << _gpio[i]);
        button_config.mode         = GPIO_MODE_INPUT;
        button_config.pull_up_en   = GPIO_PULLUP_ENABLE;
        button_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        button_config.intr_type    = GPIO_INTR_ANYEDGE;
        if (gpio_config(&button_config) != ESP_OK || gpio_isr_handler_add(_gpio[i], gpio_isr_wrapper, &_isr_contexts[i]) != ESP_OK) {
            return -1;
        }
    }
    return 0;
}

void IRAM_ATTR Spic_Button::gpio_isr_wrapper(void* arg) {
    IsrContext* ctx = static_cast<IsrContext*>(arg);
    ctx->instance->handle_interrupt(ctx->btn);
}

void IRAM_ATTR Spic_Button::handle_interrupt(BUTTON btn) {
    gpio_intr_disable(_gpio[btn]);

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xTimerStartFromISR(_debounce_timers[btn], &higherPriorityTaskWoken);
    if (higherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void Spic_Button::timer_callback(TimerHandle_t xTimer) {
    IsrContext* ctx = static_cast<IsrContext*>(pvTimerGetTimerID(xTimer));
    BUTTON btn = ctx->btn;
    Spic_Button* instance = ctx->instance;

    int level = gpio_get_level(instance->_gpio[btn]);
    BUTTONEVENT current_event = (level == 0) ? ONPRESS : ONRELEASE;

    BUTTONCALLBACK callback = instance->_callbacks[btn][current_event];
    if (callback != nullptr) callback(btn, current_event);

    gpio_intr_enable(instance->_gpio[btn]);
}

int8_t Spic_Button::sb_button_registerCallback(BUTTON btn, BUTTONEVENT event, BUTTONCALLBACK callback) {
    _callbacks[btn][event] = callback;
    return 0;
}

int8_t Spic_Button::sb_button_unregisterCallback(BUTTON btn, BUTTONEVENT event, BUTTONCALLBACK callback) {
    _callbacks[btn][event] = nullptr;
    return 0;
}

Spic_Button::BUTTONSTATE Spic_Button::sb_button_getState(BUTTON btn) {
    return static_cast<BUTTONSTATE>(gpio_get_level(_gpio[btn]));
}