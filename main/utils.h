#ifndef UTILS_H_CMD_BLINKER
#define UTILS_H_CMD_BLINKER

#include <ctype.h>
#include "defines.h"

void invalid_command_message(command_message_t* message) {
    ESP_LOGI(TAG, "Invalid command recieved");
    // Transmit that incorrect command was used.
    char* message_str = "Invalid command recieved";
    uint8_t* args = (uint8_t*)pvPortMalloc(sizeof(char)*(strlen(message_str)+1)); // Null character included
    strcpy((char*)args, message_str);
    message->msg_id = 0;
    message->led_tp = 0;
    message->data = args;
}

void process_command(char* read_buffer) {
    char* params_string = strchr(read_buffer, ' ');
    params_string++;  // We dont' want the space.
    command_message_t message;
    xEventGroupSetBits(task_eventgroup_handle, ALL_TASKS_QUEUE_READY);
    if (strstr(read_buffer, "led") != NULL) {
        xEventGroupClearBits(task_eventgroup_handle, LED_TASK_QUEUE_READY);
        char* start_ptr = params_string;
        int val = 0;
        while (!isdigit((int)*start_ptr)) {
            start_ptr++;
        }
        char* end_ptr = start_ptr;
        while (isdigit((int)*end_ptr)) {
            val *= 10;
            val += *end_ptr-'0';
            end_ptr++;
        }
        ESP_LOGI(TAG, "LED args: %d", val);
        message.msg_id = 1;
        message.led_tp = val;
        message.data = NULL;
    } else if (strstr(read_buffer, "echo") != NULL) {
        // Transmit the echo message
        xEventGroupClearBits(task_eventgroup_handle, TRANSMIT_TASK_QUEUE_READY);
        uint8_t* args = (uint8_t*)pvPortMalloc(sizeof(char)*(strlen(params_string)+1)); // Null character included
        strcpy((char*)args, params_string);
        message.msg_id = 0;
        message.led_tp = 0;
        message.data = args;
        ESP_LOGI(TAG, "echo message ready");
    } else {
        xEventGroupClearBits(task_eventgroup_handle, TRANSMIT_TASK_QUEUE_READY);
        invalid_command_message(&message);
    }

    if (xQueueSend(global_queue_handle, &message, 100) == pdTRUE) {
        ESP_LOGI(TAG, "Task added to the queue successfully");
    } else {
        ESP_LOGE(TAG, "Failed to send message");
    }
}

#endif