#include <camera.h>
#include <esp_log.h>
#include <esp_psram.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <wifi.h>
#include <websocket.h>

static const char *TAG = "iot-cam-surveillance:main";

QueueHandle_t buffer;

void log_startup_info() {
    ESP_LOGI(TAG, "Startup..");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Total PSRAM: %d bytes", esp_psram_get_size());
}
void app_main(void)
{
    log_startup_info();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(wifi_init_sta());
    ESP_ERROR_CHECK(camera_init());
    httpd_handle_t websocket_server = start_webserver();
    if (websocket_server == NULL) {
        ESP_LOGE(TAG, "Failed to start websocket server");
    }
}
