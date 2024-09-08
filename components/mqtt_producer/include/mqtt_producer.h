#include "esp_err.h"
#ifndef MQTT_PRODUCER_H
#define MQTT_PRODUCER_H

esp_err_t mqtt_start(void);
void mqtt_send_message(char* data);


#endif // MQTT_PRODUCER_H