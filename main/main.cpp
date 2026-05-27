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
#include "Spic_UART.h"
#include "Spic_Web.h"
#include "nvs_flash.h"

Spic_LED spic_led;
Spic_Button spic_button;
Spic_ADC spic_adc;
Spic_7seg spic_7seg;
Spic_Timer spic_timer;
Spic_UART spic_uart;
Spic_Web spic_web;

volatile uint8_t system_mode = 0;

volatile uint8_t counter_value = 0;
volatile bool counter_running = false;

volatile bool web_override_adc = false;
volatile uint8_t web_sensor_value = 0;

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

void on_web_command(Spic_Web::WebCommand cmd, uint8_t value) {
    if (cmd == Spic_Web::WEB_MODE) {
        web_override_adc = false; 
        if (xSemaphoreTake(hardware_mutex, portMAX_DELAY) == pdTRUE) {
            system_mode = value;
            spic_led.led_setMask(0x00);
            if (system_mode == 0) {
                spic_timer.sb_timer_setAlarm(counter_tick_callback, 1000, 1000);
                spic_7seg.sb_7seg_showNumber(counter_value);
            } else {
                spic_timer.sb_timer_cancelAlarm();
            }
            ESP_LOGI("main", "Changed Mode via Web: %d", system_mode);
            xSemaphoreGive(hardware_mutex);
        }
    } 
    else if (cmd == Spic_Web::WEB_BTN0) {
        ButtonRTOSEvent evt;
        evt.button = Spic_Button::BUTTON0;
        xQueueSend(buttonQueue, &evt, 0); 
        ESP_LOGI("main", "Web-Click: Button 0");
    } 
    else if (cmd == Spic_Web::WEB_BTN1) {
        ButtonRTOSEvent evt;
        evt.button = Spic_Button::BUTTON1;
        xQueueSend(buttonQueue, &evt, 0); 
        ESP_LOGI("main", "Web-Click: Button 1");
    } else if (cmd == Spic_Web::WEB_POTI && system_mode == 1) {
        if (value == 0) {
            web_override_adc = false;
            return;
        }
        web_override_adc = true; 
        web_sensor_value = value;
    }
    else if (cmd == Spic_Web::WEB_PHOTO && system_mode == 2) {
        if (value == 0) {
            web_override_adc = false;
            return;
        }
        web_override_adc = true;
        web_sensor_value = value;
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
            uint8_t display_value;
            int16_t adc_value;
            const char* sensor_name = (current_mode == 1) ? "POTI" : "PHOTO";

            if (web_override_adc) {
                display_value = web_sensor_value;
                adc_value = (web_sensor_value * 4095) / 99;
            } else {
                adc_value = (current_mode == 1) ? spic_adc.sb_adc_read(Spic_ADC::POTI) : spic_adc.sb_adc_read(Spic_ADC::PHOTO);
                display_value = (adc_value * 99) / 4095;
            }
            
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

void switch_mode(void) {
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
}

void on_uart_receive(const char* message) {
    ESP_LOGI("main", "UART: %s", message);

    if (strcmp(message, "UART_NEXT_MODE") == 0) {
        switch_mode();
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

    if (spic_uart.sb_uart_init(115200) != 0) {
        ESP_LOGE("main", "sb_uart_init fail");
        return;
    }
    spic_uart.sb_uart_registerCallback(on_uart_receive);
    ESP_LOGI("main", "UART OK...");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (spic_web.sb_web_init("YOUR_SSID", "YOUR_PASSWORD") != 0) {
        ESP_LOGE("main", "sb_web_init fail");
        return;
    }
    spic_web.sb_web_registerCallback(on_web_command);
    ESP_LOGI("main", "WEB OK...");

    while (1) {
        ButtonRTOSEvent buttonEventFirst;
        if (xQueueReceive(buttonQueue, &buttonEventFirst, portMAX_DELAY)) {
            
            ButtonRTOSEvent buttonEventSecond;
            if (xQueueReceive(buttonQueue, &buttonEventSecond, pdMS_TO_TICKS(50))) {
                if (buttonEventFirst.button != buttonEventSecond.button) {
                    switch_mode();
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