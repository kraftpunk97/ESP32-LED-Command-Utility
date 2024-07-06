#ifndef SETUP_H_CMD_BLNKER
#define SETUP_H_CMD_BLINKER

#include "defines.h"

void uart_setup(void) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    int intr_alloc_flags = 0;

    #if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
    #endif

    QueueHandle_t uart_queue;

    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM,
                                 UART_TXD_IO,
                                 UART_RXD_IO,
                                 UART_RTS_IO,
                                 UART_CTS_IO));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM,
                                        UART_BUFFER_SIZE*2,
                                        UART_BUFFER_SIZE,
                                        10,
                                        &uart_queue,
                                        intr_alloc_flags));
    ESP_LOGI(TAG, "UART setup complete");
}

void led_setup(void) {
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}

#endif