idf_component_register(SRCS "websocket.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "esp_wifi" "nvs_flash" "esp_netif" "esp_eth" "esp_http_server" "esp32-camera""camera" "esp_timer")
