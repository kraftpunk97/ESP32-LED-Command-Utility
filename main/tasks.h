#ifndef TASKS_H_CMD_BLINKER
#define TASKS_H_CMD_BLINKER

#include "defines.h"
#include "utils.h"

void listener_task(void* args) {
    rx_buffer_ptr = (uint8_t*) malloc(UART_BUFFER_SIZE);
    int len;
    while (true) {
        len = uart_read_bytes(UART_PORT_NUM, rx_buffer_ptr, (UART_BUFFER_SIZE-1), 10 /*/portTICK_PERIOD_MS*/); // TODO: Play with this.
        if (len) {
            rx_buffer_ptr[len] = '\0';
            ESP_LOGI(TAG, "Recieved: %s", rx_buffer_ptr);
            process_command((char*)rx_buffer_ptr);
        }
    }
}

void transmit_task(void* args) {
    tx_buffer_ptr = (uint8_t*) malloc(UART_BUFFER_SIZE);
    command_message_t message;
    while(true) {
        // Copy the data from the queue/buffer to the tx_buffer and send that
        if (xQueueReceive(global_queue_handle, &message, portMAX_DELAY)) {
            if (message.msg_id == 0) {
                strcpy((char*)message.data, (char*)tx_buffer_ptr);
                ESP_LOGI(TAG, "Sending: %s", tx_buffer_ptr);
                uart_write_bytes(UART_PORT_NUM, (const char*)tx_buffer_ptr, strlen((char*)tx_buffer_ptr));
                
                bzero(tx_buffer_ptr, UART_BUFFER_SIZE);
                vPortFree(message.data);
            }
        }
    }
}

#endif