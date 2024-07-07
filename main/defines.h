#ifndef DEFINES_H_CMD_BLINKER
#define DEFINES_H_CMD_BLINKER

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

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

/*** FreeRTOS defines ***/
#define TRANSMIT_TASK_QUEUE_READY (1<<0)
#define LED_TASK_QUEUE_READY (1<<1)
#define ALL_TASKS_QUEUE_READY (TRANSMIT_TASK_QUEUE_READY | LED_TASK_QUEUE_READY)

/*** Other declarations and constants***/
static const char* TAG = "APP";
uint8_t* rx_buffer_ptr;

TaskHandle_t transmit_handle;
TaskHandle_t listener_handle;
TaskHandle_t led_handle;
TaskHandle_t blink_handle = NULL; // Especially need to declare this handle NULL, because it may create issues with Task Deletion.

QueueHandle_t global_queue_handle = 0; // The queue used for sending messages between tasks.

EventGroupHandle_t task_eventgroup_handle; // An event group created to signal when it is safe to remove message from the queue and de-allocate its memory.

typedef struct Message {
    int led_tp;    // Time period of the led-blinking
    uint8_t* data; // Data to be transmitted to the user.
} command_message_t;



#endif