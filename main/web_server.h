#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

void web_server_start(void);
esp_err_t web_server_send_html(httpd_req_t *req, const char *html);
esp_err_t web_server_send_json(httpd_req_t *req, const char *json);
const char* web_server_get_mime_type(const char* filename);
