#include "setup.h"
#include "tasks.h"

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