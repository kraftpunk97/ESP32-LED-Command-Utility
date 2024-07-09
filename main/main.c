#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

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

TaskHandle_t transmit_handle = NULL;
TaskHandle_t listener_handle = NULL;
TaskHandle_t led_handle = NULL;
TaskHandle_t blink_handle = NULL;

StackType_t listener_stack[2048];
StackType_t transmit_stack[2048];
StackType_t led_stack[2048];

StaticTask_t listener_task_buffer;
StaticTask_t transmit_task_buffer;
StaticTask_t led_task_buffer;

QueueHandle_t transmit_queue_handle = 0;
QueueHandle_t led_queue_handle = 0;
/*** END ***/

/*** Different message types  ***/
typedef struct transmit_message_t {
    uint8_t* data; // Data to be transmitted to the user.
} transmit_message_t;

typedef struct led_message_t {
    int led_tp; // Time period of the led-blinking
} led_message_t;
/*** END ***/


/*** Functions for setting up UART functionality and LED GPIO pins ***/
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
/** END ***/


/*** Utility functions ***/
void message_dispatcher(led_message_t* led_message, transmit_message_t* transmit_message) {
    if (led_message->led_tp > 0) {
        // Send to led task
        do {
            ESP_LOGE(TAG, "Failed to send message. Queue full. Trying again...");
        } while (xQueueSend(led_queue_handle, led_message, 50) == errQUEUE_FULL);
    }
    if (transmit_message->data != NULL) {
        // Send to transmit task
        do {
            ESP_LOGE(TAG, "Failed to send message. Queue full. Trying again...");
        } while (xQueueSend(transmit_queue_handle, transmit_message, 50) == errQUEUE_FULL);
    }
    ESP_LOGI(TAG, "Message sent to the queue successfully."); 
}

void construct_transmit_message(transmit_message_t* message, char* message_str) {
    uint8_t* args = (uint8_t*)pvPortMalloc(sizeof(char)*(strlen(message_str)+1)); // Null character included
    strcpy((char*)args, message_str);
    message->data = args;
}

void process_command(char* read_buffer, led_message_t* led_message, transmit_message_t* transmit_message) {
    char* params_string = strchr(read_buffer, ' ');
    params_string++;  // We dont' want the space.
    if (strstr(read_buffer, "led") != NULL) {
        // Handling the arguments in case of an `led` command
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
        led_message->led_tp = val;
        if (val == 0) {
            construct_transmit_message(transmit_message, "LED turned off");
        }
        else if (val == 1) {
            construct_transmit_message(transmit_message, "LED turned on");
        }
        else {
            char blink_message[50];
            sprintf(blink_message, "LED blinking with  half time period of %d ms", val);
            construct_transmit_message(transmit_message, blink_message);
        }
    } else if (strstr(read_buffer, "echo") != NULL) {
        // Transmit the echo message
        led_message->led_tp = -1;
        construct_transmit_message(transmit_message, params_string);
    } else {
        led_message->led_tp = -1;
        construct_transmit_message(transmit_message, "Invalid command received");
    }
}
/*** ***/

/*** Task Definitions ***/
void listener_task(void* args) {
    rx_buffer_ptr = (uint8_t*) malloc(UART_BUFFER_SIZE);
    transmit_message_t transmit_message = {NULL,};
    led_message_t led_message;
    int len;
    while (true) {
        len = uart_read_bytes(UART_PORT_NUM, rx_buffer_ptr, (UART_BUFFER_SIZE-1), 10);
        if (len) {
            rx_buffer_ptr[len] = '\0';
            ESP_LOGI(TAG, "Recieved: %s", rx_buffer_ptr);
            process_command((char*)rx_buffer_ptr, &led_message, &transmit_message);
            message_dispatcher(&led_message, &transmit_message);
        }
    }
}

void transmit_task(void* args) {
    transmit_message_t message;
    while (true) {
        ESP_LOGI(TAG, "transmit task now listening");
        if (xQueueReceive(transmit_queue_handle, &message, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "Transmit task recieved a message");
            uart_write_bytes(UART_PORT_NUM, (const char*)message.data, strlen((char*)message.data));
            vPortFree(message.data);
        }
    }
}

void led_task(void* args) {
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
/** END ***/


void app_main(void) {
    uart_setup();
    led_setup();

    transmit_queue_handle = xQueueCreate(7, sizeof(transmit_message_t));
    led_queue_handle = xQueueCreate(7, sizeof(led_message_t));

    listener_handle = xTaskCreateStatic(listener_task, "listener", 2048, NULL, 2, listener_stack, &listener_task_buffer);
    configASSERT(listener_handle != NULL);

    transmit_handle = xTaskCreateStatic(transmit_task, "transmit", 2048, NULL, 1, transmit_stack, &transmit_task_buffer);
    configASSERT(transmit_handle != NULL);

    led_handle = xTaskCreateStatic(led_task, "led", 2048, NULL, 1, led_stack, &led_task_buffer);
    configASSERT(led_handle != NULL);
}