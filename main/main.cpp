#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "Spic_UART.h"

#define BOOT_BUTTON GPIO_NUM_0

Spic_UART spic_uart;
static QueueHandle_t button_queue;

static void IRAM_ATTR boot_button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    
    xQueueSendFromISR(button_queue, &gpio_num, NULL);
}

extern "C" void app_main(void) {
    if (spic_uart.sb_uart_init(115200) != 0) {
        ESP_LOGE("UART", "sb_uart_init failed");
        return;
    }
    ESP_LOGI("main", "UART OK...");

    button_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_config_t io_config = {};
    io_config.intr_type = GPIO_INTR_NEGEDGE;
    io_config.mode = GPIO_MODE_INPUT;
    io_config.pin_bit_mask = (1ULL << BOOT_BUTTON);
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_config);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BOOT_BUTTON, boot_button_isr_handler, (void*) BOOT_BUTTON);

    ESP_LOGI("main", "Ready to use...");

    TickType_t last_press_time = 0;

    while (1) {
        uint32_t io_num;

        if (xQueueReceive(button_queue, &io_num, portMAX_DELAY) == pdPASS) {
            TickType_t current_time = xTaskGetTickCount();
            if ((current_time - last_press_time) > pdMS_TO_TICKS(300)) {
                ESP_LOGI("main", "Button %d pressed", io_num);
                spic_uart.sb_uart_send("UART_NEXT_MODE");
                last_press_time = current_time;
            }
        }
    }
}
