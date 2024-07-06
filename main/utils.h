#ifndef UTILS_H_CMD_BLINKER
#define UTILS_H_CMD_BLINKER

#include "defines.h"

void process_command(char* read_buffer) {
    char* params_string = strchr(read_buffer, ' ');
    command_message_t message;
    if (strstr(read_buffer, "led") != NULL) {
    
    } else if (strstr(read_buffer, "echo") != NULL) {
        // Transmit the echo message
        uint8_t* args = (uint8_t*)pvPortMalloc(sizeof(char)*(strlen(params_string)+1));
        strcpy((char*)args, params_string);
        message.data = 0;
        message.led_tp = 0;
        message.data = args;

    } else {
        // Transmit that incorrect command was used.
        uint8_t* args = (uint8_t*)pvPortMalloc(sizeof(char)*(strlen(params_string)+1));
        strcpy((char*)args, "Invalid command recieved");
        message.data = 0;
        message.led_tp = 0;
        message.data = args;
    }
    if (!xQueueSend(global_queue_handle, &message, 100)) {
        ESP_LOGE(TAG, "Failed to send message");
    }
}

#endif