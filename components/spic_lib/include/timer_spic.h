#pragma once
#include "esp_timer.h"
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef void(*ALARMCALLBACK) (void);

class Spic_Timer {
public:
    Spic_Timer(void);
    ~Spic_Timer(void);

    int8_t sb_timer_setAlarm(ALARMCALLBACK callback, uint32_t first_ms, uint32_t cycle_ms);
    int8_t sb_timer_cancelAlarm();

    int8_t sb_timer_delay(uint16_t waittime);
    void sb_timer_abortDelay();
private:
    esp_timer_handle_t _timer_handle;
    ALARMCALLBACK _callback;
    TaskHandle_t _delayed_task;
    static void esp_timer_wrapper(void* arg);
};