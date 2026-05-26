#include "Spic_UART.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

Spic_UART::Spic_UART(void) : _uart_port(UART_NUM_2), _callback(nullptr) {} 

Spic_UART::~Spic_UART(void) {
    uart_driver_delete(_uart_port);
}

int8_t Spic_UART::sb_uart_init(uint32_t baud_rate) {
    uart_config_t uart_config = {};
    uart_config.baud_rate = baud_rate;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    
    if (uart_param_config(_uart_port, &uart_config) != ESP_OK) return -1;
    if (uart_set_pin(_uart_port, _tx_pin, _rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) return -1;

    if (uart_driver_install(_uart_port, 1024, 0, 0, NULL, 0) != ESP_OK) return -1;

    xTaskCreate(rx_task, "uart_rx_task", 2048, this, 2, NULL);
    return 0;
}

void Spic_UART::sb_uart_send(const char* message) {
    ESP_LOGI("UART", "Sending: %s", message);
    uart_write_bytes(_uart_port, message, strlen(message));
}

void Spic_UART::sb_uart_registerCallback(UARTCALLBACK callback) {
    _callback = callback;
}

void Spic_UART::rx_task(void* arg) {
    Spic_UART* instance = static_cast<Spic_UART*>(arg);
    uint8_t* data = (uint8_t*) malloc(1024);

    while (1) {
        int length = uart_read_bytes(instance->_uart_port, data, 1023, pdMS_TO_TICKS(100));

        if (length > 0) {
            data[length] = '\0';

            if (instance->_callback) instance->_callback((const char*) data);
        }
    }

    free(data);
    vTaskDelete(NULL);
}