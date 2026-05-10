#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

// NVS storage namespace
#define NVS_NAMESPACE "esp32_small_os"

// NVS key types
typedef enum {
    NVS_TYPE_U8 = 0,
    NVS_TYPE_I8,
    NVS_TYPE_U16,
    NVS_TYPE_I16,
    NVS_TYPE_U32,
    NVS_TYPE_I32,
    NVS_TYPE_U64,
    NVS_TYPE_I64,
    NVS_TYPE_FLOAT,
    NVS_TYPE_DOUBLE,
    NVS_TYPE_STRING,
    NVS_TYPE_BLOB,
    NVS_TYPE_BOOL
} nvs_type_t;

// NVS storage statistics
typedef struct {
    uint32_t total_entries;
    uint32_t used_entries;
    uint32_t free_entries;
    uint32_t total_space;
    uint32_t used_space;
    uint32_t free_space;
    uint32_t page_count;
    uint32_t free_entries_per_page;
    uint32_t namespace_count;
    bool initialized;
} nvs_stats_t;

// NVS storage configuration
typedef struct {
    const char *namespace;
    bool enable_encryption;
    bool enable_statistics;
    bool enable_backup;
    uint32_t backup_interval_ms;
    uint32_t max_key_length;
    uint32_t max_value_length;
} nvs_config_t;

// NVS entry information
typedef struct {
    char key[64];
    nvs_type_t type;
    size_t size;
    uint32_t timestamp;
    uint32_t access_count;
    bool is_encrypted;
} nvs_entry_info_t;

// Function declarations
esp_err_t nvs_storage_init(void);
esp_err_t nvs_storage_deinit(void);
esp_err_t nvs_storage_set_config(const nvs_config_t *config);
esp_err_t nvs_storage_get_config(nvs_config_t *config);

// Basic data type operations
esp_err_t nvs_storage_set_u8(const char *key, uint8_t value);
esp_err_t nvs_storage_set_i8(const char *key, int8_t value);
esp_err_t nvs_storage_set_u16(const char *key, uint16_t value);
esp_err_t nvs_storage_set_i16(const char *key, int16_t value);
esp_err_t nvs_storage_set_u32(const char *key, uint32_t value);
esp_err_t nvs_storage_set_i32(const char *key, int32_t value);
esp_err_t nvs_storage_set_u64(const char *key, uint64_t value);
esp_err_t nvs_storage_set_i64(const char *key, int64_t value);
esp_err_t nvs_storage_set_float(const char *key, float value);
esp_err_t nvs_storage_set_double(const char *key, double value);
esp_err_t nvs_storage_set_bool(const char *key, bool value);
esp_err_t nvs_storage_set_str(const char *key, const char *value);
esp_err_t nvs_storage_set_blob(const char *key, const void *value, size_t length);

esp_err_t nvs_storage_get_u8(const char *key, uint8_t *value);
esp_err_t nvs_storage_get_i8(const char *key, int8_t *value);
esp_err_t nvs_storage_get_u16(const char *key, uint16_t *value);
esp_err_t nvs_storage_get_i16(const char *key, int16_t *value);
esp_err_t nvs_storage_get_u32(const char *key, uint32_t *value);
esp_err_t nvs_storage_get_i32(const char *key, int32_t *value);
esp_err_t nvs_storage_get_u64(const char *key, uint64_t *value);
esp_err_t nvs_storage_get_i64(const char *key, int64_t *value);
esp_err_t nvs_storage_get_float(const char *key, float *value);
esp_err_t nvs_storage_get_double(const char *key, double *value);
esp_err_t nvs_storage_get_bool(const char *key, bool *value);
esp_err_t nvs_storage_get_str(const char *key, char *value, size_t max_length);
esp_err_t nvs_storage_get_blob(const char *key, void *value, size_t *length);

// Operations with default values
uint8_t nvs_storage_get_u8_default(const char *key, uint8_t default_value);
int8_t nvs_storage_get_i8_default(const char *key, int8_t default_value);
uint16_t nvs_storage_get_u16_default(const char *key, uint16_t default_value);
int16_t nvs_storage_get_i16_default(const char *key, int16_t default_value);
uint32_t nvs_storage_get_u32_default(const char *key, uint32_t default_value);
int32_t nvs_storage_get_i32_default(const char *key, int32_t default_value);
uint64_t nvs_storage_get_u64_default(const char *key, uint64_t default_value);
int64_t nvs_storage_get_i64_default(const char *key, int64_t default_value);
float nvs_storage_get_float_default(const char *key, float default_value);
double nvs_storage_get_double_default(const char *key, double default_value);
bool nvs_storage_get_bool_default(const char *key, bool default_value);
const char* nvs_storage_get_str_default(const char *key, const char *default_value);

// Key management
esp_err_t nvs_storage_delete_key(const char *key);
esp_err_t nvs_storage_key_exists(const char *key, bool *exists);
esp_err_t nvs_storage_get_key_info(const char *key, nvs_entry_info_t *info);
esp_err_t nvs_storage_list_keys(char *keys[], size_t max_keys, size_t *found_count);

// Namespace operations
esp_err_t nvs_storage_create_namespace(const char *namespace);
esp_err_t nvs_storage_delete_namespace(const char *namespace);
esp_err_t nvs_storage_list_namespaces(char *namespaces[], size_t max_namespaces, size_t *found_count);

// Batch operations
esp_err_t nvs_storage_begin_transaction(void);
esp_err_t nvs_storage_commit_transaction(void);
esp_err_t nvs_storage_rollback_transaction(void);
esp_err_t nvs_storage_backup_namespace(const char *namespace, const char *backup_key);
esp_err_t nvs_storage_restore_namespace(const char *namespace, const char *backup_key);

// Statistics and monitoring
esp_err_t nvs_storage_get_statistics(nvs_stats_t *stats);
esp_err_t nvs_storage_get_namespace_statistics(const char *namespace, nvs_stats_t *stats);
void nvs_storage_print_statistics(void);
void nvs_storage_print_namespace_statistics(const char *namespace);
esp_err_t nvs_storage_reset_statistics(void);

// Advanced operations
esp_err_t nvs_storage_compact(void);
esp_err_t nvs_storage_format(void);
esp_err_t nvs_storage_validate(void);
esp_err_t nvs_storage_repair(void);

// Security operations
esp_err_t nvs_storage_encrypt_key(const char *key);
esp_err_t nvs_storage_decrypt_key(const char *key);
esp_err_t nvs_storage_set_encryption_key(const uint8_t *key, size_t key_length);
esp_err_t nvs_storage_enable_encryption(bool enable);

// Utility functions
size_t nvs_storage_get_type_size(nvs_type_t type);
const char* nvs_storage_get_type_name(nvs_type_t type);
esp_err_t nvs_storage_get_free_space(size_t *free_space);
esp_err_t nvs_storage_get_used_space(size_t *used_space);
esp_err_t nvs_storage_get_total_space(size_t *total_space);

// Debug functions
void nvs_storage_dump_all_keys(void);
void nvs_storage_dump_namespace(const char *namespace);
esp_err_t nvs_storage_self_test(void);

// Constants
#define NVS_MAX_KEY_LENGTH 64
#define NVS_MAX_VALUE_LENGTH 1984
#define NVS_MAX_NAMESPACES 16
#define NVS_MAX_KEYS_PER_NAMESPACE 256
#define NVS_DEFAULT_NAMESPACE "default"
#define NVS_BACKUP_KEY_PREFIX "backup_"
#define NVS_ENCRYPTION_KEY_LENGTH 32
