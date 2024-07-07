#ifndef TASKS_H_CMD_BLINKER
#define TASKS_H_CMD_BLINKER

#include "utils.h"

void blink_task (void* args) {
    int tp = *((int*)args);
    while (true) {
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(tp / portTICK_PERIOD_MS);
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(tp / portTICK_PERIOD_MS);
    }

}

void led_task(void* args) {
    command_message_t message;
    while (true) {
        ESP_LOGI(TAG, "blinker task now listening");
        if (xQueuePeek(global_queue_handle, &message, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Blinker task peeked at the task from the queue");
            if ((xEventGroupGetBits(task_eventgroup_handle)&LED_TASK_QUEUE_READY) == 0) {
                ESP_LOGI(TAG, "This task was meant for blinker task");
                if (blink_handle != NULL) {
                    xTaskAbortDelay(blink_handle);
                    vTaskDelete(blink_handle);
                    blink_handle = NULL;
                    ESP_LOGI(TAG, "Blink Task has been deleted");
                }
                if (message.led_tp == 0)
                    gpio_set_level(LED_GPIO, 0);
                else if (message.led_tp == 1)
                    gpio_set_level(LED_GPIO, 1);
                else {
                    ESP_LOGI(TAG, "Entering Blink with tp: %d", message.led_tp);
                    configASSERT(blink_handle == NULL);
                    xTaskCreate(blink_task, "blink", 2048, &(message.led_tp), 3, &blink_handle);   
                }

                xEventGroupSetBits(task_eventgroup_handle, LED_TASK_QUEUE_READY);

                // Deletion part
                if ((xEventGroupGetBits(task_eventgroup_handle)&ALL_TASKS_QUEUE_READY) == ALL_TASKS_QUEUE_READY) {
                    xQueueReceive(global_queue_handle, &message, portMAX_DELAY);
                    vPortFree(message.data);
                    ESP_LOGI(TAG, "Blinker task freed the memory");
                }
            }
        }
    }
}

void listener_task(void* args) {
    rx_buffer_ptr = (uint8_t*) malloc(UART_BUFFER_SIZE);
    int len;
    while (true) {
        len = uart_read_bytes(UART_PORT_NUM, rx_buffer_ptr, (UART_BUFFER_SIZE-1), 10); // TODO: Play with this.
        if (len) {
            rx_buffer_ptr[len] = '\0';
            ESP_LOGI(TAG, "Recieved: %s", rx_buffer_ptr);
            process_command((char*)rx_buffer_ptr);
        }
    }
}

void transmit_task(void* args) {
    command_message_t message;
    while(true) {
        ESP_LOGI(TAG, "transmit task now listening");
        // Copy the data from the queue/buffer to the tx_buffer and send that

        if (xQueuePeek(global_queue_handle, &message, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Transmit task peeked at the task from the queue");
            if ((xEventGroupGetBits(task_eventgroup_handle)&TRANSMIT_TASK_QUEUE_READY) == 0) {
                ESP_LOGI(TAG, "This task was meant for transmit task");
                uart_write_bytes(UART_PORT_NUM, (const char*)message.data, strlen((char*)message.data));
                xEventGroupSetBits(task_eventgroup_handle, TRANSMIT_TASK_QUEUE_READY);

                // Deletion part 
                if ((xEventGroupGetBits(task_eventgroup_handle)&ALL_TASKS_QUEUE_READY) == ALL_TASKS_QUEUE_READY) {
                    xQueueReceive(global_queue_handle, &message, portMAX_DELAY);
                     vPortFree(message.data);
                    ESP_LOGI(TAG, "Transmit task freed the memory");
                }
            }
        }
    }
}

#endif