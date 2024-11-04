#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 // software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#define CONFIG_XCLK_FREQ 20000000
#define CONFIG_OV2640_SUPPORT 1
#define CONFIG_OV7725_SUPPORT 1
#define CONFIG_OV3660_SUPPORT 1
#define CONFIG_OV5640_SUPPORT 1
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

static const char *TAG = "iot-cam-surveillance:camera";
extern QueueHandle_t buffer;

esp_err_t camera_init(void)
{
    ESP_LOGI(TAG, "Initializing camera");
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_1_BIT,
        .freq_hz = CONFIG_XCLK_FREQ,
        .clk_cfg = LEDC_AUTO_CLK};

    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Timer config error: %d", ret);
    }

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = CAM_PIN_XCLK,
        .duty = 1,
        .hpoint = 0};

    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Channel config error: %d", ret);
    }
    camera_config_t camera_config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,

        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        .xclk_freq_hz = CONFIG_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_UXGA,
        .jpeg_quality = 25,
        .fb_count = 1,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY}; // CAMERA_GRAB_LATEST. Sets when buffers should be filled
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera init error: %d", err);
        return err;
    }
    ESP_LOGI(TAG, "Camera initialized");
    return ESP_OK;
}

camera_fb_t* camera_capture() {
    // Acquire a frame
    camera_fb_t* original_fb = esp_camera_fb_get();
    if (!original_fb) {
        ESP_LOGE(TAG, "Camera Capture Failed");
        return NULL;
    }

    // Create a copy of the frame buffer structure
    camera_fb_t* fb_copy = (camera_fb_t*)malloc(sizeof(camera_fb_t));
    if (!fb_copy) {
        ESP_LOGE(TAG, "Failed to allocate frame buffer copy");
        esp_camera_fb_return(original_fb);
        return NULL;
    }

    // Allocate memory for the pixel data
    fb_copy->buf = (uint8_t*)malloc(original_fb->len);
    if (!fb_copy->buf) {
        ESP_LOGE(TAG, "Failed to allocate pixel buffer");
        free(fb_copy);
        esp_camera_fb_return(original_fb);
        return NULL;
    }

    // Copy all the data
    memcpy(fb_copy->buf, original_fb->buf, original_fb->len);
    fb_copy->len = original_fb->len;
    fb_copy->width = original_fb->width;
    fb_copy->height = original_fb->height;
    fb_copy->format = original_fb->format;
    fb_copy->timestamp = original_fb->timestamp;

    // Return the original buffer to the driver
    esp_camera_fb_return(original_fb);
    
    return fb_copy;
}