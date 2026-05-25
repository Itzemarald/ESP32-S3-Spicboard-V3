#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "led_spic.h"
#include "button_spic.h"
#include "adc_spic.h"
#include "esp_log.h"
#include "7seg_spic.h"
#include "timer_spic.h"

Spic_LED spic_led;
Spic_Button spic_button;
Spic_ADC spic_adc;
Spic_7seg spic_7seg;
Spic_Timer spic_timer;

volatile uint8_t system_mode = 0;

volatile uint8_t counter_value = 0;
volatile bool counter_running = false;

static SemaphoreHandle_t hardware_mutex;

#include "freertos/queue.h"

static QueueHandle_t buttonQueue;

typedef struct {
    Spic_Button::BUTTON button;
    uint64_t timestamp;
} ButtonRTOSEvent;

void counter_tick_callback(void) {
    if (counter_running) {
        if (xSemaphoreTake(hardware_mutex, portMAX_DELAY) == pdTRUE) {
            counter_value += 1;
            if (counter_value > 99) {
                counter_value = 0;
            }
            spic_7seg.sb_7seg_showNumber(counter_value);
            xSemaphoreGive(hardware_mutex);
        }
    }
}

void on_button_press(Spic_Button::BUTTON btn, Spic_Button::BUTTONEVENT event) {
    if (event != Spic_Button::ONPRESS) {
        return;
    }

    ButtonRTOSEvent buttonEvent;
    buttonEvent.button = btn;
    buttonEvent.timestamp = esp_timer_get_time();

    xQueueSend(buttonQueue, &buttonEvent, portMAX_DELAY);
}

void sensor_display_task(void *pvParameters) {
    while (1) {
        uint8_t current_mode = system_mode;
        if (current_mode == 1 || current_mode == 2) {
            int16_t adc_value = (current_mode == 1) ? spic_adc.sb_adc_read(Spic_ADC::POTI) : spic_adc.sb_adc_read(Spic_ADC::PHOTO);
            const char* sensor_name = (current_mode == 1) ? "POTI" : "PHOTO";

            uint8_t display_value = (adc_value * 99) / 4095;

            if (xSemaphoreTake(hardware_mutex, portMAX_DELAY) == pdTRUE) {
                if (system_mode == current_mode) {
                    spic_7seg.sb_7seg_showNumber(display_value);
                    spic_led.led_showLevel(adc_value, 4095);
                    ESP_LOGI("sensor_display_task", "%s: %d", sensor_name, display_value);
                }
            }
            xSemaphoreGive(hardware_mutex);
            
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

extern "C" void app_main(void) {
    ESP_LOGI("main", "Initializing...");
    if (spic_led.sb_led_init() != 0) {
        ESP_LOGE("main", "sb_led_init fail");
        return;
    }

    ESP_LOGI("main", "LEDs OK...");

    if (spic_adc.sb_adc_init() != 0) {
        ESP_LOGE("main", "sb_adc_init fail");
        return;
    }
    ESP_LOGI("main", "ADC OK...");

    if (spic_7seg.sb_7seg_init() != 0) {
        ESP_LOGE("main", "sb_7seg_init fail");
        return;
    }

    if (spic_timer.sb_timer_setAlarm(counter_tick_callback, 1000, 1000) != 0) {
        ESP_LOGE("main", "sb_timer_setAlarm fail");
        return;
    }
    ESP_LOGI("main", "Timer OK...");
    spic_7seg.sb_7seg_showNumber(counter_value);

    if (spic_button.sb_button_init() != 0) {
        ESP_LOGE("main", "sb_button_init fail");
        return;
    }

    hardware_mutex = xSemaphoreCreateMutex();
    if (hardware_mutex == NULL) {
        ESP_LOGE("main", "Mutex creation fail");
        return;
    }

    buttonQueue = xQueueCreate(10, sizeof(ButtonRTOSEvent));

    if (buttonQueue == nullptr) {
        ESP_LOGE("main", "Queue creation fail");
        return;
    }

    spic_button.sb_button_registerCallback(Spic_Button::BUTTON0, Spic_Button::ONPRESS, on_button_press);
    spic_button.sb_button_registerCallback(Spic_Button::BUTTON1, Spic_Button::ONPRESS, on_button_press);
    ESP_LOGI("main", "Buttons OK...");

    xTaskCreate(sensor_display_task, "sensor_display_task", 2048, NULL, 1, NULL);
    while (1) {
        ButtonRTOSEvent buttonEventFirst;
        if (xQueueReceive(buttonQueue, &buttonEventFirst, portMAX_DELAY)) {
            
            ButtonRTOSEvent buttonEventSecond;
            if (xQueueReceive(buttonQueue, &buttonEventSecond, pdMS_TO_TICKS(50))) {
                if (buttonEventFirst.button != buttonEventSecond.button) {
                    if (xSemaphoreTake(hardware_mutex, portMAX_DELAY) == pdTRUE) {
                        spic_led.led_setMask(0x00);
                        system_mode = (system_mode + 1) % 3;
                        if (system_mode == 0) {
                            spic_timer.sb_timer_setAlarm(counter_tick_callback, 1000, 1000);
                            spic_7seg.sb_7seg_showNumber(counter_value);
                        } else {
                            spic_timer.sb_timer_cancelAlarm();
                            spic_led.led_setMask(0x00);
                        }
                        ESP_LOGI("main", "Change mode.");
                        xSemaphoreGive(hardware_mutex);
                    }
                    continue;
                }
            }

            
            if (system_mode == 0) {
                if (xSemaphoreTake(hardware_mutex, portMAX_DELAY) == pdTRUE) {
                    uint8_t mask = spic_led.led_getMask();
                    if (buttonEventFirst.button == Spic_Button::BUTTON0) {
                        spic_led.led_setMask((mask << 1) | 0x01);
                        ESP_LOGI("main", "Button 0 pressed, turning one more LED on.");
                        counter_running = !counter_running;
                        ESP_LOGI("main", "Timer %s", counter_running ? "started" : "paused");
                    } else if (buttonEventFirst.button == Spic_Button::BUTTON1) {
                        counter_value = 0;
                        spic_7seg.sb_7seg_showNumber(counter_value);
                        spic_led.led_setMask(mask >> 1);
                        ESP_LOGI("main", "Button 1 pressed, turning off one LED.");
                        ESP_LOGI("main", "Timer resetted");
                    }      
                }
                xSemaphoreGive(hardware_mutex);
            }
        }
    }
    return;
}