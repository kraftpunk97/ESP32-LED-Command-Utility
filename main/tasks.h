#ifndef TASKS_H_CMD_BLINKER
#define TASKS_H_CMD_BLINKER

#include "setup.h"
#include "utils.h"

void listener_task(void* args) {
    rx_buffer_ptr = (uint8_t*) malloc(UART_BUFFER_SIZE);
    int len;
    while (true) {
        len = uart_read_bytes(UART_PORT_NUM, rx_buffer_ptr, (UART_BUFFER_SIZE-1), 10 /*/portTICK_PERIOD_MS*/); // TODO: Play with this.
        if (len) {
            rx_buffer_ptr[len] = '\0';
            process_command((char*)rx_buffer_ptr);
        }
    }
}

void transmit_task(void* args) {
    tx_buffer_ptr = (uint8_t) malloc(UART_BUFFER_SIZE);
    while(true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uart_write_bytes(UART_PORT_NUM, (const char*)tx_buffer_ptr, strlen((char*)tx_buffer_ptr));
    }
}

#endif