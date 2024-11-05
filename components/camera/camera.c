// Standard C libraries
#include <string.h>
#include <sys/param.h>

// ESP System headers
#include <esp_log.h>
#include <esp_system.h>
#include <esp_camera.h>

// FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Power and control pins
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1  // software reset will be performed
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

// Data pins (D0-D7)
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0      5

// Sync pins
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

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

// Add this forward declaration at the top of the file, after the includes
static camera_config_t get_camera_config(void);

#define CHECK_ERROR(expr, msg) do { \
    esp_err_t err = (expr); \
    if (err != ESP_OK) { \
        ESP_LOGE(TAG, "%s: %d", msg, err); \
        return err; \
    } \
} while(0)

esp_err_t camera_init(void)
{
    ESP_LOGI(TAG, "Initializing camera");
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_1_BIT,
        .freq_hz = CONFIG_XCLK_FREQ,
        .clk_cfg = LEDC_AUTO_CLK};

    CHECK_ERROR(ledc_timer_config(&ledc_timer), "Timer config error");

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = CAM_PIN_XCLK,
        .duty = 1,
        .hpoint = 0};

    CHECK_ERROR(ledc_channel_config(&ledc_channel), "Channel config error");
    camera_config_t camera_config = get_camera_config();
    CHECK_ERROR(esp_camera_init(&camera_config), "Camera init error");
    ESP_LOGI(TAG, "Camera initialized");
    return ESP_OK;
}

camera_fb_t* camera_capture(void) {
    camera_fb_t* original_fb = esp_camera_fb_get();
    if (!original_fb) {
        ESP_LOGE(TAG, "Camera Capture Failed");
        return NULL;
    }

    camera_fb_t* fb_copy = malloc(sizeof(camera_fb_t));
    uint8_t* buf_copy = malloc(original_fb->len);
    
    if (!fb_copy || !buf_copy) {
        ESP_LOGE(TAG, "Memory allocation failed");
        free(fb_copy);
        free(buf_copy);
        esp_camera_fb_return(original_fb);
        return NULL;
    }

    // Copy frame buffer data
    *fb_copy = *original_fb;  // Copy the structure
    fb_copy->buf = buf_copy;
    memcpy(fb_copy->buf, original_fb->buf, original_fb->len);

    esp_camera_fb_return(original_fb);
    return fb_copy;
}

static camera_config_t get_camera_config(void) {
    return (camera_config_t) {
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
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY
    };
}