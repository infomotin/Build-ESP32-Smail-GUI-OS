#include "web_server.h"
#include "web_view_manager.h"
#include "realtime_monitor.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "auth.h"
#include "gpio_manager.h"
#include "activity_manager.h"
#include "scheduler.h"
#include "storage.h"
#include <string.h>
#include <cJSON.h>

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;

static const char* HTML_TEMPLATE = "<!DOCTYPE html>\n<html>\n<head>\n    <meta charset='UTF-8'>\n    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n    <title>ESP32 OS Manager</title>\n    <style>\n        * { margin: 0; padding: 0; box-sizing: border-box; }\n        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #1a1a1a; color: #fff; }\n        .container { max-width: 1200px; margin: 0 auto; padding: 20px; }\n        .header { background: #2d2d2d; padding: 20px; border-radius: 8px; margin-bottom: 20px; }\n        .card { background: #2d2d2d; padding: 20px; border-radius: 8px; margin-bottom: 20px; }\n        .btn { background: #007acc; color: white; border: none; padding: 10px 20px; border-radius: 4px; cursor: pointer; margin: 5px; }\n        .btn:hover { background: #005a9e; }\n        .btn.danger { background: #dc3545; }\n        .btn.danger:hover { background: #c82333; }\n        .status { display: inline-block; padding: 4px 8px; border-radius: 4px; font-size: 12px; }\n        .status.online { background: #28a745; }\n        .status.offline { background: #dc3545; }\n        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }\n        .gpio-pin { border: 1px solid #444; padding: 10px; margin: 5px 0; border-radius: 4px; }\n        .log { background: #000; padding: 10px; border-radius: 4px; font-family: monospace; height: 200px; overflow-y: auto; }\n    </style>\n</head>\n<body>\n    <div class='container'>\n        <div class='header'>\n            <h1>ESP32 OS Manager</h1>\n            <p>System Status: <span id='system-status' class='status online'>Online</span></p>\n            <p>Uptime: <span id='uptime'>0s</span></p>\n            <p>Free Heap: <span id='heap'>0</span> bytes</p>\n        </div>\n        <div class='grid'>\n            <div class='card'>\n                <h2>System Control</h2>\n                <button class='btn' onclick='restartSystem()'>Restart System</button>\n                <button class='btn danger' onclick='shutdownSystem()'>Shutdown</button>\n                <button class='btn' onclick='refreshStatus()'>Refresh Status</button>\n            </div>\n            <div class='card'>\n                <h2>GPIO Control</h2>\n                <div id='gpio-controls'></div>\n            </div>\n            <div class='card'>\n                <h2>Activity Manager</h2>\n                <button class='btn' onclick='triggerActivity(1)'>Trigger Activity 1</button>\n                <button class='btn' onclick='triggerActivity(2)'>Trigger Activity 2</button>\n                <button class='btn' onclick='triggerActivity(3)'>Trigger Activity 3</button>\n            </div>\n            <div class='card'>\n                <h2>System Logs</h2>\n                <div id='system-logs' class='log'></div>\n                <button class='btn' onclick='clearLogs()'>Clear Logs</button>\n            </div>\n        </div>\n    </div>\n    <script>\n        function updateStatus() {\n            fetch('/api/status').then(r => r.json()).then(data => {\n                document.getElementById('uptime').textContent = data.uptime + 's';\n                document.getElementById('heap').textContent = data.free_heap;\n                addLog('Status updated: ' + JSON.stringify(data));\n            });\n        }\n        function restartSystem() {\n            if(confirm('Restart system?')) {\n                fetch('/api/restart', {method: 'POST'}).then(() => {\n                    addLog('System restart initiated');\n                });\n            }\n        }\n        function shutdownSystem() {\n            if(confirm('Shutdown system?')) {\n                fetch('/api/shutdown', {method: 'POST'}).then(() => {\n                    addLog('System shutdown initiated');\n                });\n            }\n        }\n        function triggerActivity(id) {\n            fetch('/api/activity/' + id, {method: 'POST'}).then(r => r.json()).then(data => {\n                addLog('Activity ' + id + ': ' + data.message);\n            });\n        }\n        function toggleGPIO(pin) {\n            fetch('/api/gpio/' + pin + '/toggle', {method: 'POST'}).then(r => r.json()).then(data => {\n                addLog('GPIO ' + pin + ': ' + data.message);\n                updateGPIO();\n            });\n        }\n        function updateGPIO() {\n            fetch('/api/gpio').then(r => r.json()).then(data => {\n                const container = document.getElementById('gpio-controls');\n                container.innerHTML = '';\n                data.pins.forEach(pin => {\n                    const div = document.createElement('div');\n                    div.className = 'gpio-pin';\n                    div.innerHTML = `\n                        Pin ${pin.number}: ${pin.mode} - Level: ${pin.level}\n                        <button class='btn' onclick='toggleGPIO(${pin.number})'>Toggle</button>\n                    `;\n                    container.appendChild(div);\n                });\n            });\n        }\n        function addLog(message) {\n            const logs = document.getElementById('system-logs');\n            const time = new Date().toLocaleTimeString();\n            logs.innerHTML += '[' + time + '] ' + message + '\n';\n            logs.scrollTop = logs.scrollHeight;\n        }\n        function clearLogs() {\n            document.getElementById('system-logs').innerHTML = '';\n        }\n        function refreshStatus() {\n            updateStatus();\n            updateGPIO();\n        }\n        setInterval(updateStatus, 5000);\n        window.onload = function() {\n            updateStatus();\n            updateGPIO();\n            addLog('ESP32 OS Manager loaded');\n        };\n    </script>\n</body>\n</html>";

static esp_err_t status_get_handler(httpd_req_t *req)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "ok");
    cJSON_AddNumberToObject(response, "uptime", esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(response, "free_heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(response, "min_free_heap", esp_get_minimum_free_heap_size());
    
    char *json_string = cJSON_Print(response);
    web_server_send_json(req, json_string);
    
    free(json_string);
    cJSON_Delete(response);
    return ESP_OK;
}

static esp_err_t restart_handler(httpd_req_t *req)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "message", "System restart initiated");
    
    char *json_string = cJSON_Print(response);
    web_server_send_json(req, json_string);
    
    free(json_string);
    cJSON_Delete(response);
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

static esp_err_t shutdown_handler(httpd_req_t *req)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "message", "System shutdown initiated");
    
    char *json_string = cJSON_Print(response);
    web_server_send_json(req, json_string);
    
    free(json_string);
    cJSON_Delete(response);
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_deep_sleep_start();
    return ESP_OK;
}

static esp_err_t gpio_get_handler(httpd_req_t *req)
{
    cJSON *response = cJSON_CreateObject();
    cJSON *pins = cJSON_CreateArray();
    
    for (int i = 0; i < 40; i++) {
        cJSON *pin = cJSON_CreateObject();
        cJSON_AddNumberToObject(pin, "number", i);
        cJSON_AddNumberToObject(pin, "level", gpio_get_level_safe(i));
        cJSON_AddStringToObject(pin, "mode", "input");
        cJSON_AddItemToArray(pins, pin);
    }
    
    cJSON_AddItemToObject(response, "pins", pins);
    
    char *json_string = cJSON_Print(response);
    web_server_send_json(req, json_string);
    
    free(json_string);
    cJSON_Delete(response);
    return ESP_OK;
}

static esp_err_t gpio_toggle_handler(httpd_req_t *req)
{
    char *buf = NULL;
    size_t buf_len = 0;
    
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char pin_str[8];
            if (httpd_query_key_value(buf, "pin", pin_str, sizeof(pin_str)) == ESP_OK) {
                int pin = atoi(pin_str);
                int current_level = gpio_get_level_safe(pin);
                gpio_set_level_safe(pin, !current_level);
                
                cJSON *response = cJSON_CreateObject();
                cJSON_AddStringToObject(response, "message", "GPIO pin toggled");
                cJSON_AddNumberToObject(response, "pin", pin);
                cJSON_AddNumberToObject(response, "new_level", !current_level);
                
                char *json_string = cJSON_Print(response);
                web_server_send_json(req, json_string);
                
                free(json_string);
                cJSON_Delete(response);
                free(buf);
                return ESP_OK;
            }
        }
        free(buf);
    }
    
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid pin number");
    return ESP_FAIL;
}

static esp_err_t activity_trigger_handler(httpd_req_t *req)
{
    char *buf = NULL;
    size_t buf_len = 0;
    
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char activity_str[8];
            if (httpd_query_key_value(buf, "id", activity_str, sizeof(activity_str)) == ESP_OK) {
                int activity_id = atoi(activity_str);
                bool success = activity_manager_trigger(activity_id);
                
                cJSON *response = cJSON_CreateObject();
                cJSON_AddStringToObject(response, "message", success ? "Activity triggered" : "Failed to trigger activity");
                cJSON_AddNumberToObject(response, "activity_id", activity_id);
                cJSON_AddBoolToObject(response, "success", success);
                
                char *json_string = cJSON_Print(response);
                web_server_send_json(req, json_string);
                
                free(json_string);
                cJSON_Delete(response);
                free(buf);
                return ESP_OK;
            }
        }
        free(buf);
    }
    
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid activity ID");
    return ESP_FAIL;
}

static esp_err_t index_handler(httpd_req_t *req)
{
    web_server_send_html(req, HTML_TEMPLATE);
    return ESP_OK;
}

static const char* web_server_get_mime_type(const char* filename)
{
    if (strstr(filename, ".html")) return "text/html";
    if (strstr(filename, ".css")) return "text/css";
    if (strstr(filename, ".js")) return "application/javascript";
    if (strstr(filename, ".json")) return "application/json";
    if (strstr(filename, ".png")) return "image/png";
    if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) return "image/jpeg";
    if (strstr(filename, ".ico")) return "image/x-icon";
    return "text/plain";
}

esp_err_t web_server_send_html(httpd_req_t *req, const char *html)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

esp_err_t web_server_send_json(httpd_req_t *req, const char *json)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}

static httpd_uri_t status_uri = {
    .uri = "/api/status",
    .method = HTTP_GET,
    .handler = status_get_handler,
    .user_ctx = NULL
};

static httpd_uri_t restart_uri = {
    .uri = "/api/restart",
    .method = HTTP_POST,
    .handler = restart_handler,
    .user_ctx = NULL
};

static httpd_uri_t shutdown_uri = {
    .uri = "/api/shutdown",
    .method = HTTP_POST,
    .handler = shutdown_handler,
    .user_ctx = NULL
};

static httpd_uri_t gpio_get_uri = {
    .uri = "/api/gpio",
    .method = HTTP_GET,
    .handler = gpio_get_handler,
    .user_ctx = NULL
};

static httpd_uri_t gpio_toggle_uri = {
    .uri = "/api/gpio/*/toggle",
    .method = HTTP_POST,
    .handler = gpio_toggle_handler,
    .user_ctx = NULL
};

static httpd_uri_t activity_trigger_uri = {
    .uri = "/api/activity/*",
    .method = HTTP_POST,
    .handler = activity_trigger_handler,
    .user_ctx = NULL
};

static esp_err_t dashboard_handler(httpd_req_t *req)
{
    char *dashboard_json;
    esp_err_t err = web_view_manager_generate_dashboard(&dashboard_json);
    if (err == ESP_OK) {
        web_server_send_json(req, dashboard_json);
        free(dashboard_json);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to generate dashboard");
    }
    return err;
}

static esp_err_t resources_handler(httpd_req_t *req)
{
    char *resources_json;
    esp_err_t err = web_view_manager_get_resource_list(&resources_json);
    if (err == ESP_OK) {
        web_server_send_json(req, resources_json);
        free(resources_json);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get resources");
    }
    return err;
}

static esp_err_t monitor_start_handler(httpd_req_t *req)
{
    esp_err_t err = realtime_monitor_start(1000); // 1 second interval
    cJSON *response = cJSON_CreateObject();
    
    if (err == ESP_OK) {
        cJSON_AddStringToObject(response, "message", "Real-time monitoring started");
        cJSON_AddBoolToObject(response, "enabled", true);
    } else {
        cJSON_AddStringToObject(response, "message", "Failed to start monitoring");
        cJSON_AddBoolToObject(response, "enabled", false);
    }
    
    char *json_string = cJSON_Print(response);
    web_server_send_json(req, json_string);
    
    free(json_string);
    cJSON_Delete(response);
    return err;
}

static esp_err_t monitor_stop_handler(httpd_req_t *req)
{
    esp_err_t err = realtime_monitor_stop();
    cJSON *response = cJSON_CreateObject();
    
    if (err == ESP_OK) {
        cJSON_AddStringToObject(response, "message", "Real-time monitoring stopped");
        cJSON_AddBoolToObject(response, "enabled", false);
    } else {
        cJSON_AddStringToObject(response, "message", "Failed to stop monitoring");
    }
    
    char *json_string = cJSON_Print(response);
    web_server_send_json(req, json_string);
    
    free(json_string);
    cJSON_Delete(response);
    return err;
}

static esp_err_t monitor_data_handler(httpd_req_t *req)
{
    char *monitor_json;
    esp_err_t err = realtime_monitor_get_json(&monitor_json);
    
    if (err == ESP_OK) {
        web_server_send_json(req, monitor_json);
        free(monitor_json);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get monitor data");
    }
    return err;
}

static esp_err_t auth_login_handler(httpd_req_t *req)
{
    char *buf = NULL;
    size_t buf_len = 0;
    
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char token_str[128];
            if (httpd_query_key_value(buf, "token", token_str, sizeof(token_str)) == ESP_OK) {
                const char *session = auth_create_session(token_str);
                
                cJSON *response = cJSON_CreateObject();
                if (session) {
                    cJSON_AddStringToObject(response, "message", "Login successful");
                    cJSON_AddStringToObject(response, "session", session);
                    cJSON_AddBoolToObject(response, "success", true);
                } else {
                    cJSON_AddStringToObject(response, "message", "Invalid token");
                    cJSON_AddBoolToObject(response, "success", false);
                }
                
                char *json_string = cJSON_Print(response);
                web_server_send_json(req, json_string);
                
                free(json_string);
                cJSON_Delete(response);
                free(buf);
                return ESP_OK;
            }
        }
        free(buf);
    }
    
    httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Authentication required");
    return ESP_FAIL;
}

static esp_err_t auth_logout_handler(httpd_req_t *req)
{
    auth_invalidate_session();
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "message", "Logged out successfully");
    cJSON_AddBoolToObject(response, "success", true);
    
    char *json_string = cJSON_Print(response);
    web_server_send_json(req, json_string);
    
    free(json_string);
    cJSON_Delete(response);
    return ESP_OK;
}

static httpd_uri_t index_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_handler,
    .user_ctx = NULL
};

static httpd_uri_t dashboard_uri = {
    .uri = "/api/dashboard",
    .method = HTTP_GET,
    .handler = dashboard_handler,
    .user_ctx = NULL
};

static httpd_uri_t resources_uri = {
    .uri = "/api/resources",
    .method = HTTP_GET,
    .handler = resources_handler,
    .user_ctx = NULL
};

static esp_err_t resource_handler(httpd_req_t *req)
{
    char *resource_name = strrchr(req->uri, '/');
    if (resource_name && strlen(resource_name) > 1) {
        resource_name++; // Skip the '/'
        return web_view_manager_serve_resource(req, resource_name);
    }
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid resource name");
    return ESP_ERR_NOT_FOUND;
}

static httpd_uri_t resource_uri = {
    .uri = "/resource/*",
    .method = HTTP_GET,
    .handler = resource_handler,
    .user_ctx = NULL
};

static httpd_uri_t monitor_start_uri = {
    .uri = "/api/monitor/start",
    .method = HTTP_POST,
    .handler = monitor_start_handler,
    .user_ctx = NULL
};

static httpd_uri_t monitor_stop_uri = {
    .uri = "/api/monitor/stop",
    .method = HTTP_POST,
    .handler = monitor_stop_handler,
    .user_ctx = NULL
};

static httpd_uri_t monitor_data_uri = {
    .uri = "/api/monitor/data",
    .method = HTTP_GET,
    .handler = monitor_data_handler,
    .user_ctx = NULL
};

static httpd_uri_t auth_login_uri = {
    .uri = "/api/auth/login",
    .method = HTTP_POST,
    .handler = auth_login_handler,
    .user_ctx = NULL
};

static httpd_uri_t auth_logout_uri = {
    .uri = "/api/auth/logout",
    .method = HTTP_POST,
    .handler = auth_logout_handler,
    .user_ctx = NULL
};

void web_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.lru_purge_enable = true;
    
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server");
        return;
    }
    
    httpd_register_uri_handler(server, &index_uri);
    httpd_register_uri_handler(server, &status_uri);
    httpd_register_uri_handler(server, &restart_uri);
    httpd_register_uri_handler(server, &shutdown_uri);
    httpd_register_uri_handler(server, &gpio_get_uri);
    httpd_register_uri_handler(server, &gpio_toggle_uri);
    httpd_register_uri_handler(server, &activity_trigger_uri);
    httpd_register_uri_handler(server, &dashboard_uri);
    httpd_register_uri_handler(server, &resources_uri);
    httpd_register_uri_handler(server, &resource_uri);
    httpd_register_uri_handler(server, &monitor_start_uri);
    httpd_register_uri_handler(server, &monitor_stop_uri);
    httpd_register_uri_handler(server, &monitor_data_uri);
    httpd_register_uri_handler(server, &auth_login_uri);
    httpd_register_uri_handler(server, &auth_logout_uri);
    
    ESP_LOGI(TAG, "Web server started on port 80");
    ESP_LOGI(TAG, "Access the OS Manager at: http://192.168.1.100/");
}
