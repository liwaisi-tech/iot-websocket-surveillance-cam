#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mqtt_producer.h>
#include <esp_camera.h>

extern QueueHandle_t buffer;

void task_receive_queue_message(void *pvParameter)
{
    while (true)
    {
        camera_fb_t data;
        xQueueReceive(buffer, &data, portMAX_DELAY);
        // Send the picture data to the MQTT broker
        mqtt_send_message(&data);
    }
}
