#ifndef DEFINES_H_CMD_BLINKER
#define DEFINES_H_CMD_BLINKER

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

/*** UART defines ***/
#define UART_TXD_IO GPIO_NUM_4
#define UART_RXD_IO GPIO_NUM_5
#define UART_RTS_IO UART_PIN_NO_CHANGE
#define UART_CTS_IO UART_PIN_NO_CHANGE

#define UART_PORT_NUM 2
#define UART_BAUD_RATE 115200
#define UART_STACK_SIZE 2048

#define UART_BUFFER_SIZE 1024

/*** LED defines ***/
#define LED_GPIO GPIO_NUM_2

/*** Other declarations and constants***/
static const char* TAG = "APP";
uint8_t* rx_buffer_ptr;
uint8_t* tx_buffer_ptr;

TaskHandle_t transmit_handle;
TaskHandle_t listener_handle;


struct Message {
    uint8_t msg_id;
    uint8_t data[128];
};

#endif