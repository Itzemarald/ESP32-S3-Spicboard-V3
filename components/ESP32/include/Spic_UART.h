#pragma once

#include "driver/uart.h"
#include "driver/gpio.h"
#include <stdint.h>

class Spic_UART {
public:
    typedef void(*UARTCALLBACK)(const char* message);

    Spic_UART(void);
    ~Spic_UART(void);

    int8_t sb_uart_init(uint32_t baud_rate = 115200);
    void sb_uart_send(const char* message);
    void sb_uart_registerCallback(UARTCALLBACK callback);

private:
    uart_port_t _uart_port;
    UARTCALLBACK _callback;
    gpio_num_t _rx_pin = GPIO_NUM_35;
    gpio_num_t _tx_pin = GPIO_NUM_36;

    static void rx_task(void* arg);
};