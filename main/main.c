#include "tasks.h"

void app_main(void) {
    uart_setup();
    led_setup();
    
    BaseType_t listener_create = xTaskCreate(listener_task, "listener", 1024, NULL, 2, &listener_handle);
    BaseType_t transmit_create = xTaskCreate(transmit_task, "transmit", 1024, NULL, 1, &transmit_handle);
    if (listener_create != pdPASS) {
        ESP_LOGI(TAG, "listener task creation failed.");
    }

}