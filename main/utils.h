#ifndef UTILS_H_CMD_BLINKER
#define UTILS_H_CMD_BLINKER

#include "defines.h"

int process_command(char* read_buffer) {
    char* params_string = strchr(read_buffer, ' ');
    if (strstr(read_buffer, 'led') != NULL) {

    } else if (strstr(read_buffer, 'echo') != NULL) {
        // Transmit the echo message
        xTaskNotifyGive(transmit_handle);
    } else {
        // Transmit that incorrect command was used.
        xTaskNotifyGive(transmit_handle);
    }
}

#endif