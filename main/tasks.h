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
        if (xQueuePeek(global_queue_handle, &message, portMAX_DELAY)) {  // If we have any message waiting for us in the queue...
            ESP_LOGI(TAG, "Blinker task peeked at the task from the queue");
            if ((xEventGroupGetBits(task_eventgroup_handle)&LED_TASK_QUEUE_READY) == 0) { // And it is a message that we can work with ...
                ESP_LOGI(TAG, "This task was meant for blinker task");
                if (blink_handle != NULL) {  // then first check if there's an already existing blink task that is running. If there is one...
                    xTaskAbortDelay(blink_handle); // then get it out of the vTaskDelay (blocked state), 
                    vTaskDelete(blink_handle); // delete it,
                    blink_handle = NULL;  // and ensure that the task handle is set to null
                    ESP_LOGI(TAG, "Blink Task has been deleted");
                } // blink task has been deleted at this point
                if (message.led_tp == 0)
                    gpio_set_level(LED_GPIO, 0);
                else if (message.led_tp == 1)
                    gpio_set_level(LED_GPIO, 1);
                else {  // If we have a blink task in our hands...
                    ESP_LOGI(TAG, "Entering blink with tp: %d", message.led_tp);
                    configASSERT(blink_handle == NULL);  // Then first ensure that there's no other pre-existing blink task that is already running...
                    BaseType_t blink_create = (blink_task, "blink", 2048, &(message.led_tp), 3, &blink_handle);  // and then create a blink task, with the highest priority
                    if (blink_create != pdPASS) {
                        ESP_LOGE(TAG, "Failed to create blink task");
                    }
                }

                xEventGroupSetBits(task_eventgroup_handle, LED_TASK_QUEUE_READY);  // Let it be known that we are done with this message. It is safe to delete from our side.

                // Deletion part
                if ((xEventGroupGetBits(task_eventgroup_handle)&ALL_TASKS_QUEUE_READY) == ALL_TASKS_QUEUE_READY) { // If every one is done with the message...
                    xQueueReceive(global_queue_handle, &message, portMAX_DELAY); // Then remove it from the message queue
                    vPortFree(message.data); // Free the heap memory allocated to the character array.
                    ESP_LOGI(TAG, "Blinker task freed the memory");
                } // Message safely removed from the queue and deleted at this point.
            }
        }
    }
}

void listener_task(void* args) {
    rx_buffer_ptr = (uint8_t*) malloc(UART_BUFFER_SIZE);
    int len;
    while (true) {
        len = uart_read_bytes(UART_PORT_NUM, rx_buffer_ptr, (UART_BUFFER_SIZE-1), 10);
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
        if (xQueuePeek(global_queue_handle, &message, portMAX_DELAY)) {  // If there is any message in the queue...
            ESP_LOGI(TAG, "Transmit task peeked at the task from the queue");
            if ((xEventGroupGetBits(task_eventgroup_handle)&TRANSMIT_TASK_QUEUE_READY) == 0) {  // And it has something that can be transmitted to the user...
                ESP_LOGI(TAG, "This task was meant for transmit task");
                uart_write_bytes(UART_PORT_NUM, (const char*)message.data, strlen((char*)message.data)); // Then transmit the data...
                xEventGroupSetBits(task_eventgroup_handle, TRANSMIT_TASK_QUEUE_READY);  // And let it be known that we are done with the message.

                // Deletion part 
                if ((xEventGroupGetBits(task_eventgroup_handle)&ALL_TASKS_QUEUE_READY) == ALL_TASKS_QUEUE_READY) { // If every one else is also done with the message, and we were the last ones to process the message...
                    xQueueReceive(global_queue_handle, &message, portMAX_DELAY);  // then remove the message from the queue,
                    vPortFree(message.data); // And free the character array that was allocated
                    ESP_LOGI(TAG, "Transmit task freed the memory");
                } // Message safely deleted and removedd from the queue at this point.
            }
        }
    }
}

#endif