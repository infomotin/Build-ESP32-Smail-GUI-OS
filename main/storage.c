#include "storage.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "storage";
static nvs_handle_t nvs_handle;

void storage_init(void)
{
    esp_err_t err = nvs_open("small_os", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
    }
}

bool storage_save_config(const char *key, const char *value)
{
    if (nvs_handle == 0 || key == NULL || value == NULL) {
        return false;
    }
    esp_err_t err = nvs_set_str(nvs_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save config %s: %s", key, esp_err_to_name(err));
        return false;
    }
    err = nvs_commit(nvs_handle);
    return err == ESP_OK;
}

const char *storage_get_config(const char *key)
{
    static char buffer[128];
    size_t len = sizeof(buffer);
    if (nvs_get_str(nvs_handle, key, buffer, &len) != ESP_OK) {
        return NULL;
    }
    return buffer;
}
