#include "esp_err.h"
#include <esp_camera.h>
#ifndef MQTT_PRODUCER_H
#define MQTT_PRODUCER_H

esp_err_t mqtt_start(void);
void mqtt_send_message(camera_fb_t* data);


#endif // MQTT_PRODUCER_H