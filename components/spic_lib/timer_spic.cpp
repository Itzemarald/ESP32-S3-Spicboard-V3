#include "timer_spic.h"

Spic_Timer::Spic_Timer(void) : _timer_handle(nullptr), _callback(nullptr), _delayed_task(nullptr) {}

Spic_Timer::~Spic_Timer(void) {
    sb_timer_cancelAlarm();
}

void Spic_Timer::esp_timer_wrapper(void* arg) {
    Spic_Timer* instance = static_cast<Spic_Timer*>(arg);
    if (instance && instance->_callback) {
        instance->_callback();
    }
}

int8_t Spic_Timer::sb_timer_setAlarm(ALARMCALLBACK callback, uint32_t first_ms, uint32_t cycle_ms) {
    if (callback == nullptr) return -1;

    sb_timer_cancelAlarm();

    _callback = callback;

    esp_timer_create_args_t timer_args = {};
    timer_args.callback = esp_timer_wrapper;
    timer_args.arg = this;
    timer_args.name = "spic_alarm";

    if (esp_timer_create(&timer_args, &_timer_handle) != ESP_OK) return -1;

    if (cycle_ms > 0) {
        esp_timer_start_periodic(_timer_handle, cycle_ms * 1000UL);
    } else {
        esp_timer_start_once(_timer_handle, first_ms * 1000UL);
    }

    return 0;
}

int8_t Spic_Timer::sb_timer_cancelAlarm(void) {
    if (_timer_handle == nullptr) return -1;

    esp_timer_stop(_timer_handle);
    esp_timer_delete(_timer_handle);
    _timer_handle = nullptr;
    _callback = nullptr;
    return 0;
}

int8_t Spic_Timer::sb_timer_delay(uint16_t waittime) {
    if (waittime == 0) return 0;

    if (xPortInIsrContext()) return -2;

    _delayed_task = xTaskGetCurrentTaskHandle();
    
    ulTaskNotifyTake(pdTRUE, 0);

    uint32_t result = ulTaskNotifyTake(pdTRUE,  pdMS_TO_TICKS(waittime));
    _delayed_task = nullptr;
    
    return (result > 0) ? 0 : -1;
}

void Spic_Timer::sb_timer_abortDelay() {
    if (_delayed_task) {
        if (xPortInIsrContext()) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR(_delayed_task, &xHigherPriorityTaskWoken);
            
            if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
        } else {
            xTaskNotifyGive(_delayed_task);
        }
    }
}