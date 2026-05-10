#include "nvs_storage.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "mbedtls/md5.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

#define TAG "NVS_STORAGE"

// Global variables
static nvs_handle_t nvs_handle = 0;
static nvs_config_t nvs_config;
static nvs_stats_t nvs_stats;
static SemaphoreHandle_t nvs_mutex = NULL;
static bool nvs_initialized = false;
static uint8_t encryption_key[NVS_ENCRYPTION_KEY_LENGTH];
static bool encryption_enabled = false;
static uint32_t init_time = 0;

// Forward declarations
static esp_err_t nvs_storage_open_handle(void);
static esp_err_t nvs_storage_close_handle(void);
static esp_err_t nvs_storage_encrypt_data(const void *input, size_t input_len, void *output, size_t *output_len);
static esp_err_t nvs_storage_decrypt_data(const void *input, size_t input_len, void *output, size_t *output_len);
static void update_statistics(void);

// Open NVS handle
static esp_err_t nvs_storage_open_handle(void)
{
    if (nvs_handle != 0) {
        return ESP_OK;
    }
    
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace '%s': %s", NVS_NAMESPACE, esp_err_to_name(err));
        return err;
    }
    
    return ESP_OK;
}

// Close NVS handle
static esp_err_t nvs_storage_close_handle(void)
{
    if (nvs_handle == 0) {
        return ESP_OK;
    }
    
    esp_err_t err = nvs_close(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to close NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    nvs_handle = 0;
    return ESP_OK;
}

// Initialize NVS storage
esp_err_t nvs_storage_init(void)
{
    if (nvs_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing NVS storage");
    
    // Create mutex
    nvs_mutex = xSemaphoreCreateMutex();
    if (!nvs_mutex) {
        ESP_LOGE(TAG, "Failed to create NVS mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Set default configuration
    memset(&nvs_config, 0, sizeof(nvs_config_t));
    nvs_config.namespace = NVS_NAMESPACE;
    nvs_config.enable_encryption = false;
    nvs_config.enable_statistics = true;
    nvs_config.enable_backup = true;
    nvs_config.backup_interval_ms = 60000; // 1 minute
    nvs_config.max_key_length = NVS_MAX_KEY_LENGTH;
    nvs_config.max_value_length = NVS_MAX_VALUE_LENGTH;
    
    // Reset statistics
    memset(&nvs_stats, 0, sizeof(nvs_stats_t));
    init_time = esp_timer_get_time() / 1000;
    
    // Open NVS handle
    esp_err_t err = nvs_storage_open_handle();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    // Update statistics
    update_statistics();
    
    nvs_initialized = true;
    ESP_LOGI(TAG, "NVS storage initialized successfully");
    return ESP_OK;
}

// Set uint8_t value
esp_err_t nvs_storage_set_u8(const char *key, uint8_t value)
{
    if (!key || !nvs_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(nvs_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t err = nvs_set_u8(nvs_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set u8 value for key '%s': %s", key, esp_err_to_name(err));
        xSemaphoreGive(nvs_mutex);
        return err;
    }
    
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        xSemaphoreGive(nvs_mutex);
        return err;
    }
    
    nvs_stats.used_entries++;
    xSemaphoreGive(nvs_mutex);
    
    ESP_LOGD(TAG, "Set u8 value %d for key '%s'", value, key);
    return ESP_OK;
}

// Get uint8_t value
esp_err_t nvs_storage_get_u8(const char *key, uint8_t *value)
{
    if (!key || !value || !nvs_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(nvs_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t err = nvs_get_u8(nvs_handle, key, value);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to get u8 value for key '%s': %s", key, esp_err_to_name(err));
        xSemaphoreGive(nvs_mutex);
        return err;
    }
    
    if (err == ESP_OK) {
        nvs_stats.access_count++;
    }
    
    xSemaphoreGive(nvs_mutex);
    
    ESP_LOGD(TAG, "Get u8 value %d for key '%s'", err == ESP_OK ? *value : 0, key);
    return err;
}

// Get uint8_t value with default
uint8_t nvs_storage_get_u8_default(const char *key, uint8_t default_value)
{
    uint8_t value;
    esp_err_t err = nvs_storage_get_u8(key, &value);
    return (err == ESP_OK) ? value : default_value;
}

// Set uint32_t value
esp_err_t nvs_storage_set_u32(const char *key, uint32_t value)
{
    if (!key || !nvs_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(nvs_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t err = nvs_set_u32(nvs_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set u32 value for key '%s': %s", key, esp_err_to_name(err));
        xSemaphoreGive(nvs_mutex);
        return err;
    }
    
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        xSemaphoreGive(nvs_mutex);
        return err;
    }
    
    nvs_stats.used_entries++;
    xSemaphoreGive(nvs_mutex);
    
    ESP_LOGD(TAG, "Set u32 value %d for key '%s'", value, key);
    return ESP_OK;
}

// Get uint32_t value
esp_err_t nvs_storage_get_u32(const char *key, uint32_t *value)
{
    if (!key || !value || !nvs_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(nvs_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t err = nvs_get_u32(nvs_handle, key, value);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to get u32 value for key '%s': %s", key, esp_err_to_name(err));
        xSemaphoreGive(nvs_mutex);
        return err;
    }
    
    if (err == ESP_OK) {
        nvs_stats.access_count++;
    }
    
    xSemaphoreGive(nvs_mutex);
    
    ESP_LOGD(TAG, "Get u32 value %d for key '%s'", err == ESP_OK ? *value : 0, key);
    return err;
}

// Get uint32_t value with default
uint32_t nvs_storage_get_u32_default(const char *key, uint32_t default_value)
{
    uint32_t value;
    esp_err_t err = nvs_storage_get_u32(key, &value);
    return (err == ESP_OK) ? value : default_value;
}

// Set string value
esp_err_t nvs_storage_set_str(const char *key, const char *value)
{
    if (!key || !value || !nvs_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t value_len = strlen(value);
    if (value_len > nvs_config.max_value_length) {
        ESP_LOGE(TAG, "String value too long for key '%s': %d > %d", key, value_len, nvs_config.max_value_length);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(nvs_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t err = nvs_set_str(nvs_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set string value for key '%s': %s", key, esp_err_to_name(err));
        xSemaphoreGive(nvs_mutex);
        return err;
    }
    
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        xSemaphoreGive(nvs_mutex);
        return err;
    }
    
    nvs_stats.used_entries++;
    xSemaphoreGive(nvs_mutex);
    
    ESP_LOGD(TAG, "Set string value '%s' for key '%s'", value, key);
    return ESP_OK;
}

// Get string value
esp_err_t nvs_storage_get_str(const char *key, char *value, size_t max_length)
{
    if (!key || !value || !nvs_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(nvs_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    size_t required_size = max_length;
    esp_err_t err = nvs_get_str(nvs_handle, key, value, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to get string value for key '%s': %s", key, esp_err_to_name(err));
        xSemaphoreGive(nvs_mutex);
        return err;
    }
    
    if (err == ESP_OK) {
        nvs_stats.access_count++;
    } else {
        value[0] = '\0';
    }
    
    xSemaphoreGive(nvs_mutex);
    
    ESP_LOGD(TAG, "Get string value '%s' for key '%s'", err == ESP_OK ? value : "", key);
    return err;
}

// Get string value with default
const char* nvs_storage_get_str_default(const char *key, const char *default_value)
{
    static char buffer[NVS_MAX_VALUE_LENGTH + 1];
    esp_err_t err = nvs_storage_get_str(key, buffer, sizeof(buffer));
    return (err == ESP_OK) ? buffer : default_value;
}

// Delete key
esp_err_t nvs_storage_delete_key(const char *key)
{
    if (!key || !nvs_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(nvs_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t err = nvs_erase_key(nvs_handle, key);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to delete key '%s': %s", key, esp_err_to_name(err));
        xSemaphoreGive(nvs_mutex);
        return err;
    }
    
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
            xSemaphoreGive(nvs_mutex);
            return err;
        }
        
        if (nvs_stats.used_entries > 0) {
            nvs_stats.used_entries--;
        }
    }
    
    xSemaphoreGive(nvs_mutex);
    
    ESP_LOGD(TAG, "Deleted key '%s'", key);
    return ESP_OK;
}

// Check if key exists
esp_err_t nvs_storage_key_exists(const char *key, bool *exists)
{
    if (!key || !exists || !nvs_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(nvs_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Try to get the key length
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    
    *exists = (err == ESP_ERR_NVS_NOT_FOUND) ? false : true;
    
    xSemaphoreGive(nvs_mutex);
    
    ESP_LOGD(TAG, "Key '%s' exists: %s", key, *exists ? "yes" : "no");
    return ESP_OK;
}

// Get statistics
esp_err_t nvs_storage_get_statistics(nvs_stats_t *stats)
{
    if (!stats || !nvs_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(nvs_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Update statistics
    update_statistics();
    
    *stats = nvs_stats;
    stats->initialized = nvs_initialized;
    
    xSemaphoreGive(nvs_mutex);
    return ESP_OK;
}

// Update statistics
static void update_statistics(void)
{
    if (!nvs_initialized) {
        return;
    }
    
    // Get NVS statistics
    nvs_stats_t nvs_info;
    esp_err_t err = nvs_get_stats(NVS_NAMESPACE, &nvs_info);
    if (err == ESP_OK) {
        nvs_stats.total_entries = nvs_info.total_entries;
        nvs_stats.used_entries = nvs_info.used_entries;
        nvs_stats.free_entries = nvs_info.free_entries;
        nvs_stats.total_space = nvs_info.total_entries * nvs_config.max_value_length;
        nvs_stats.used_space = nvs_info.used_entries * nvs_config.max_value_length;
        nvs_stats.free_space = nvs_info.free_entries * nvs_config.max_value_length;
    }
    
    nvs_stats.page_count = 1; // Simplified
    nvs_stats.free_entries_per_page = nvs_stats.free_entries;
    nvs_stats.namespace_count = 1; // Simplified
}

// Print statistics
void nvs_storage_print_statistics(void)
{
    nvs_stats_t stats;
    if (nvs_storage_get_statistics(&stats) == ESP_OK) {
        ESP_LOGI(TAG, "=== NVS Storage Statistics ===");
        ESP_LOGI(TAG, "Initialized: %s", stats.initialized ? "yes" : "no");
        ESP_LOGI(TAG, "Namespace: %s", NVS_NAMESPACE);
        ESP_LOGI(TAG, "Total entries: %d", stats.total_entries);
        ESP_LOGI(TAG, "Used entries: %d", stats.used_entries);
        ESP_LOGI(TAG, "Free entries: %d", stats.free_entries);
        ESP_LOGI(TAG, "Total space: %d bytes", stats.total_space);
        ESP_LOGI(TAG, "Used space: %d bytes", stats.used_space);
        ESP_LOGI(TAG, "Free space: %d bytes", stats.free_space);
        ESP_LOGI(TAG, "Page count: %d", stats.page_count);
        ESP_LOGI(TAG, "Free entries per page: %d", stats.free_entries_per_page);
        ESP_LOGI(TAG, "Namespace count: %d", stats.namespace_count);
        ESP_LOGI(TAG, "Access count: %d", stats.access_count);
        ESP_LOGI(TAG, "Uptime: %d seconds", (esp_timer_get_time() / 1000) - init_time);
        ESP_LOGI(TAG, "===============================");
    }
}

// Self test
esp_err_t nvs_storage_self_test(void)
{
    ESP_LOGI(TAG, "Starting NVS storage self-test...");
    
    esp_err_t result = ESP_OK;
    
    // Test basic operations
    const char *test_key = "test_key";
    const char *test_value = "test_value";
    char read_value[64];
    
    // Test string operations
    esp_err_t err = nvs_storage_set_str(test_key, test_value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set test string: %s", esp_err_to_name(err));
        result = ESP_FAIL;
    }
    
    err = nvs_storage_get_str(test_key, read_value, sizeof(read_value));
    if (err != ESP_OK || strcmp(read_value, test_value) != 0) {
        ESP_LOGE(TAG, "Failed to get test string");
        result = ESP_FAIL;
    }
    
    // Test numeric operations
    uint32_t test_num = 12345678;
    uint32_t read_num = 0;
    
    err = nvs_storage_set_u32("test_num", test_num);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set test number: %s", esp_err_to_name(err));
        result = ESP_FAIL;
    }
    
    err = nvs_storage_get_u32("test_num", &read_num);
    if (err != ESP_OK || read_num != test_num) {
        ESP_LOGE(TAG, "Failed to get test number");
        result = ESP_FAIL;
    }
    
    // Test key existence
    bool exists = false;
    err = nvs_storage_key_exists(test_key, &exists);
    if (err != ESP_OK || !exists) {
        ESP_LOGE(TAG, "Failed to check key existence");
        result = ESP_FAIL;
    }
    
    // Test default values
    uint32_t default_value = nvs_storage_get_u32_default("nonexistent_key", 999);
    if (default_value != 999) {
        ESP_LOGE(TAG, "Failed to get default value");
        result = ESP_FAIL;
    }
    
    // Cleanup test data
    nvs_storage_delete_key(test_key);
    nvs_storage_delete_key("test_num");
    nvs_storage_delete_key("nonexistent_key");
    
    ESP_LOGI(TAG, "NVS storage self-test %s", result == ESP_OK ? "PASSED" : "FAILED");
    return result;
}

// Set bool value
esp_err_t nvs_storage_set_bool(const char *key, bool value)
{
    return nvs_storage_set_u8(key, value ? 1 : 0);
}

// Get bool value
esp_err_t nvs_storage_get_bool(const char *key, bool *value)
{
    uint8_t u8_value;
    esp_err_t err = nvs_storage_get_u8(key, &u8_value);
    if (err == ESP_OK) {
        *value = (u8_value != 0);
    }
    return err;
}

// Get bool value with default
bool nvs_storage_get_bool_default(const char *key, bool default_value)
{
    uint8_t value;
    esp_err_t err = nvs_storage_get_u8(key, &value);
    return (err == ESP_OK) ? (value != 0) : default_value;
}

// Get type size
size_t nvs_storage_get_type_size(nvs_type_t type)
{
    switch (type) {
        case NVS_TYPE_U8:
        case NVS_TYPE_I8:
            return 1;
        case NVS_TYPE_U16:
        case NVS_TYPE_I16:
            return 2;
        case NVS_TYPE_U32:
        case NVS_TYPE_I32:
        case NVS_TYPE_FLOAT:
            return 4;
        case NVS_TYPE_U64:
        case NVS_TYPE_I64:
        case NVS_TYPE_DOUBLE:
            return 8;
        case NVS_TYPE_BOOL:
            return 1;
        case NVS_TYPE_STRING:
        case NVS_TYPE_BLOB:
            return NVS_MAX_VALUE_LENGTH;
        default:
            return 0;
    }
}

// Get type name
const char* nvs_storage_get_type_name(nvs_type_t type)
{
    switch (type) {
        case NVS_TYPE_U8: return "uint8_t";
        case NVS_TYPE_I8: return "int8_t";
        case NVS_TYPE_U16: return "uint16_t";
        case NVS_TYPE_I16: return "int16_t";
        case NVS_TYPE_U32: return "uint32_t";
        case NVS_TYPE_I32: return "int32_t";
        case NVS_TYPE_U64: return "uint64_t";
        case NVS_TYPE_I64: return "int64_t";
        case NVS_TYPE_FLOAT: return "float";
        case NVS_TYPE_DOUBLE: return "double";
        case NVS_TYPE_STRING: return "string";
        case NVS_TYPE_BLOB: return "blob";
        case NVS_TYPE_BOOL: return "bool";
        default: return "unknown";
    }
}
