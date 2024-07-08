#include <stdio.h>
#include <string.h>
#include <ctype.h>

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

void info_message(command_message_t* message, char* message_str) {
    uint8_t* args = (uint8_t*)pvPortMalloc(sizeof(char)*(strlen(message_str)+1)); // Null character included
    strcpy((char*)args, message_str);
    message->data = args;
}

void process_command(char* read_buffer) {
    char* params_string = strchr(read_buffer, ' ');
    params_string++;  // We dont' want the space.
    command_message_t message;
    xEventGroupSetBits(task_eventgroup_handle, ALL_TASKS_QUEUE_READY);
    if (strstr(read_buffer, "led") != NULL) {
        // Handling the arguments in case of an `led` command
        xEventGroupClearBits(task_eventgroup_handle, LED_TASK_QUEUE_READY);
        xEventGroupClearBits(task_eventgroup_handle, TRANSMIT_TASK_QUEUE_READY);
        
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
        message.led_tp = val;
        if (val == 0) {
            info_message(&message, "LED turned off");
        }
        else if (val == 1) {
            info_message(&message, "LED turned on");
        }
        else {
            char blink_message[50];
            sprintf(blink_message, "LED blinking with  half time period of %d ms", val);
            info_message(&message, blink_message);
        }
    } else if (strstr(read_buffer, "echo") != NULL) {
        // Transmit the echo message
        xEventGroupClearBits(task_eventgroup_handle, TRANSMIT_TASK_QUEUE_READY);
        uint8_t* args = (uint8_t*)pvPortMalloc(sizeof(char)*(strlen(params_string)+1)); // Null character included
        strcpy((char*)args, params_string);
        message.led_tp = 0;
        message.data = args;
        ESP_LOGI(TAG, "echo message ready");
    } else {
        xEventGroupClearBits(task_eventgroup_handle, TRANSMIT_TASK_QUEUE_READY);
        message.led_tp = 0;
        info_message(&message, "Invalid command received");
    }

    if (xQueueSend(global_queue_handle, &message, 100) == pdTRUE) {
        ESP_LOGI(TAG, "Task added to the queue successfully");
    } else {
        ESP_LOGE(TAG, "Failed to send message");
    }
}

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

void app_main(void) {
    uart_setup();
    led_setup();

    global_queue_handle = xQueueCreate(7, sizeof(struct Message));
    task_eventgroup_handle = xEventGroupCreate();

    BaseType_t listener_create = xTaskCreate(listener_task, "listener", 2048, NULL, 2, &listener_handle);
    if (listener_create != pdPASS) {
        ESP_LOGI(TAG, "listener task creation failed.");
    }
    
    BaseType_t transmit_create = xTaskCreate(transmit_task, "transmit", 2048, NULL, 1, &transmit_handle);
    if (transmit_create != pdPASS) {
        ESP_LOGI(TAG, "transmit task creation failed");
    }
    BaseType_t led_create = xTaskCreate(led_task, "blink", 2048, NULL, 1, &led_handle);
    if (led_create != pdPASS) {
        ESP_LOGI(TAG, "blink task creation failed");
    }
}