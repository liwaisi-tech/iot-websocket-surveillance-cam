#ifndef WEBSOCKET_H
#define WEBSOCKET_H
#include <esp_http_server.h>

httpd_handle_t start_webserver(void);
esp_err_t stop_webserver(httpd_handle_t server);

#endif // WEBSOCKET_H
