#include "web_view_manager.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "web_view_manager";

typedef struct {
    char *name;
    char *content;
    char *mime_type;
} web_resource_t;

static web_resource_t resources[MAX_WEB_RESOURCES];
static int resource_count = 0;

esp_err_t web_view_manager_init(void)
{
    memset(resources, 0, sizeof(resources));
    resource_count = 0;
    ESP_LOGI(TAG, "Web view manager initialized");
    return ESP_OK;
}

esp_err_t web_view_manager_add_resource(const char *name, const char *content, const char *mime_type)
{
    if (resource_count >= MAX_WEB_RESOURCES) {
        ESP_LOGE(TAG, "Maximum resources reached");
        return ESP_ERR_NO_MEM;
    }
    
    resources[resource_count].name = malloc(strlen(name) + 1);
    resources[resource_count].content = malloc(strlen(content) + 1);
    resources[resource_count].mime_type = malloc(strlen(mime_type) + 1);
    
    if (!resources[resource_count].name || !resources[resource_count].content || !resources[resource_count].mime_type) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }
    
    strcpy(resources[resource_count].name, name);
    strcpy(resources[resource_count].content, content);
    strcpy(resources[resource_count].mime_type, mime_type);
    
    resource_count++;
    ESP_LOGI(TAG, "Added resource: %s", name);
    return ESP_OK;
}

esp_err_t web_view_manager_serve_resource(httpd_req_t *req, const char *resource_name)
{
    for (int i = 0; i < resource_count; i++) {
        if (strcmp(resources[i].name, resource_name) == 0) {
            httpd_resp_set_type(req, resources[i].mime_type);
            httpd_resp_send(req, resources[i].content, strlen(resources[i].content));
            return ESP_OK;
        }
    }
    
    ESP_LOGW(TAG, "Resource not found: %s", resource_name);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Resource not found");
    return ESP_ERR_NOT_FOUND;
}

esp_err_t web_view_manager_generate_dashboard(char **output)
{
    cJSON *dashboard = cJSON_CreateObject();
    cJSON *widgets = cJSON_CreateArray();
    
    // System Status Widget
    cJSON *system_widget = cJSON_CreateObject();
    cJSON_AddStringToObject(system_widget, "type", "system_status");
    cJSON_AddStringToObject(system_widget, "title", "System Status");
    cJSON_AddItemToArray(widgets, system_widget);
    
    // GPIO Control Widget
    cJSON *gpio_widget = cJSON_CreateObject();
    cJSON_AddStringToObject(gpio_widget, "type", "gpio_control");
    cJSON_AddStringToObject(gpio_widget, "title", "GPIO Control");
    cJSON_AddItemToArray(widgets, gpio_widget);
    
    // Activity Manager Widget
    cJSON *activity_widget = cJSON_CreateObject();
    cJSON_AddStringToObject(activity_widget, "type", "activity_manager");
    cJSON_AddStringToObject(activity_widget, "title", "Activity Manager");
    cJSON_AddItemToArray(widgets, activity_widget);
    
    // System Logs Widget
    cJSON *logs_widget = cJSON_CreateObject();
    cJSON_AddStringToObject(logs_widget, "type", "system_logs");
    cJSON_AddStringToObject(logs_widget, "title", "System Logs");
    cJSON_AddItemToArray(widgets, logs_widget);
    
    cJSON_AddItemToObject(dashboard, "widgets", widgets);
    cJSON_AddStringToObject(dashboard, "title", "ESP32 OS Dashboard");
    
    *output = cJSON_Print(dashboard);
    cJSON_Delete(dashboard);
    
    return ESP_OK;
}

esp_err_t web_view_manager_get_resource_list(char **output)
{
    cJSON *list = cJSON_CreateArray();
    
    for (int i = 0; i < resource_count; i++) {
        cJSON *resource = cJSON_CreateObject();
        cJSON_AddStringToObject(resource, "name", resources[i].name);
        cJSON_AddStringToObject(resource, "mime_type", resources[i].mime_type);
        cJSON_AddNumberToObject(resource, "size", strlen(resources[i].content));
        cJSON_AddItemToArray(list, resource);
    }
    
    *output = cJSON_Print(list);
    cJSON_Delete(list);
    
    return ESP_OK;
}

void web_view_manager_cleanup(void)
{
    for (int i = 0; i < resource_count; i++) {
        if (resources[i].name) free(resources[i].name);
        if (resources[i].content) free(resources[i].content);
        if (resources[i].mime_type) free(resources[i].mime_type);
    }
    resource_count = 0;
    ESP_LOGI(TAG, "Web view manager cleaned up");
}
