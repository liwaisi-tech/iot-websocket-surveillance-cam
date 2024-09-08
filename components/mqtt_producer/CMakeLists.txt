idf_component_register(SRCS "mqtt_producer.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "esp_wifi" "nvs_flash" "mqtt" "esp32-camera" "json" "mbedtls")
