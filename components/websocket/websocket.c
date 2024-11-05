#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include <esp_http_server.h>
#include "websocket.h"
#include "esp_camera.h"
#include "esp_timer.h"

static const char *TAG = "websocket";

// Add structure to hold stream context
typedef struct {
    httpd_handle_t hd;
    int fd;
    esp_timer_handle_t timer;
    bool is_streaming;
} stream_context_t;

// Forward declarations
static void stream_camera_frame(void* arg);
static esp_err_t start_video_stream(httpd_handle_t handle, int socket_fd);
static void cleanup_stream_context(stream_context_t *ctx);

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

static void ws_async_send(void *arg)
{
    static const char * data = "Async data";
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
{
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    if (resp_arg == NULL) {
        return ESP_ERR_NO_MEM;
    }
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    esp_err_t ret = httpd_queue_work(handle, ws_async_send, resp_arg);
    if (ret != ESP_OK) {
        free(resp_arg);
    }
    return ret;
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return start_video_stream(req->handle, httpd_req_to_sockfd(req));
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    
    // First receive the frame len
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed: %d", ret);
        return ret;
    }

    // Handle close messages
    if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        // Cleanup will be handled by the connection close callback
        return ESP_OK;
    }

    return ESP_OK;
}

static const httpd_uri_t ws = {
    .uri        = "/ws",
    .method     = HTTP_GET,
    .handler    = ws_handler,
    .user_ctx   = NULL,
    .is_websocket = true
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &ws);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

esp_err_t stop_webserver(httpd_handle_t server)
{
    return httpd_stop(server);
}

static esp_err_t start_video_stream(httpd_handle_t handle, int socket_fd)
{
    stream_context_t *ctx = calloc(1, sizeof(stream_context_t));
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate stream context");
        return ESP_ERR_NO_MEM;
    }

    ctx->hd = handle;
    ctx->fd = socket_fd;
    ctx->is_streaming = true;
    
    esp_timer_create_args_t timer_args = {
        .callback = &stream_camera_frame,
        .arg = ctx,
        .name = "camera_stream"
    };
    
    esp_err_t ret = esp_timer_create(&timer_args, &ctx->timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer: %d", ret);
        cleanup_stream_context(ctx);
        return ret;
    }

    ret = esp_timer_start_periodic(ctx->timer, 50000); // 50ms interval
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start timer: %d", ret);
        cleanup_stream_context(ctx);
        return ret;
    }
    
    return ESP_OK;
}

static void stream_camera_frame(void* arg)
{
    stream_context_t *ctx = (stream_context_t *)arg;
    if (!ctx || !ctx->is_streaming) {
        return;
    }
    
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return;
    }

    httpd_ws_frame_t ws_pkt = {
        .payload = fb->buf,
        .len = fb->len,
        .type = HTTPD_WS_TYPE_BINARY
    };

    esp_err_t ret = httpd_ws_send_frame_async(ctx->hd, ctx->fd, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send frame: %d", ret);
        ctx->is_streaming = false;  // Stop streaming on error
    }
    
    esp_camera_fb_return(fb);
}

static void cleanup_stream_context(stream_context_t *ctx) {
    if (ctx) {
        if (ctx->timer) {
            esp_timer_stop(ctx->timer);
            esp_timer_delete(ctx->timer);
        }
        free(ctx);
    }
}
