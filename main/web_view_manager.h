#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

#define MAX_WEB_RESOURCES 50

esp_err_t web_view_manager_init(void);
esp_err_t web_view_manager_add_resource(const char *name, const char *content, const char *mime_type);
esp_err_t web_view_manager_serve_resource(httpd_req_t *req, const char *resource_name);
esp_err_t web_view_manager_generate_dashboard(char **output);
esp_err_t web_view_manager_get_resource_list(char **output);
void web_view_manager_cleanup(void);
