/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include <esp_camera.h>
#include <cJSON.h>
#include "mbedtls/base64.h"


static const char *TAG = "mqtt:esp32-cam:producer";
esp_mqtt_client_handle_t client;


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_MQTT_BROKER_URL,
        .credentials.authentication.password = CONFIG_MQTT_PASSWORD,
        .credentials.username = CONFIG_MQTT_USERNAME,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

esp_err_t mqtt_start(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    mqtt_app_start();
    return ESP_OK;
}

char* convert_to_json_string(camera_fb_t* data) {
    // Create a cJSON object
    cJSON *json = cJSON_CreateObject();

    // Calculate the size of the base64 encoded string
    // size_t encoded_len = 0;
    // mbedtls_base64_encode(NULL, 0, &encoded_len, data->buf, data->len);

    // // Allocate memory for the base64 encoded string
    // unsigned char *encoded_buf = (unsigned char *)malloc(encoded_len + 1);
    // if (encoded_buf == NULL) {
    //     cJSON_Delete(json);
    //     return NULL;
    // }

    // // Encode the buffer data to a base64 string
    // if (mbedtls_base64_encode(encoded_buf, encoded_len, &encoded_len, data->buf, data->len) != 0) {
    //     free(encoded_buf);
    //     cJSON_Delete(json);
    //     return NULL;
    // }
    // encoded_buf[encoded_len] = '\0'; // Null-terminate the string

    
    //cJSON_AddStringToObject(json, "image", (const char*)encoded_buf);
    cJSON_AddNumberToObject(json, "width", data->width);
    cJSON_AddNumberToObject(json, "height", data->height);
    cJSON_AddNumberToObject(json, "format", data->format);
    cJSON_AddNumberToObject(json, "len", data->len);
    cJSON_AddNumberToObject(json, "seconds", data->timestamp.tv_sec);
    cJSON_AddNumberToObject(json, "microseconds", data->timestamp.tv_usec);

    // Convert cJSON object to string
    char* json_string = cJSON_Print(json);
    // Clean up cJSON object
    cJSON_Delete(json);

    return json_string; // Remember to free this string after use
}

void mqtt_send_message(camera_fb_t* data)
{   
    esp_mqtt_client_publish(client, CONFIG_MQTT_TOPIC, (const unsigned char*)data->buf, data->len, 1, 0);
    free(data);
}

