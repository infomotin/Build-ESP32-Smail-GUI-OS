#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Log levels
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_VERBOSE
} log_level_t;

// Log output destinations
typedef enum {
    LOG_OUTPUT_UART = 0,
    LOG_OUTPUT_FILE,
    LOG_OUTPUT_NETWORK,
    LOG_OUTPUT_MEMORY,
    LOG_OUTPUT_ALL
} log_output_t;

// Log categories
typedef enum {
    LOG_CATEGORY_SYSTEM = 0,
    LOG_CATEGORY_WIFI,
    LOG_CATEGORY_GPIO,
    LOG_CATEGORY_HTTP,
    LOG_CATEGORY_MEMORY,
    LOG_CATEGORY_SCHEDULER,
    LOG_CATEGORY_INTERRUPT,
    LOG_CATEGORY_STORAGE,
    LOG_CATEGORY_ALL
} log_category_t;

// Log entry structure
typedef struct {
    uint32_t timestamp;
    log_level_t level;
    log_category_t category;
    const char *tag;
    const char *file;
    int line;
    const char *function;
    char message[256];
    uint32_t sequence;
} log_entry_t;

// Log configuration
typedef struct {
    log_level_t min_level;
    log_output_t output;
    log_category_t enabled_categories;
    bool enable_timestamp;
    bool enable_colors;
    bool enable_file_line;
    bool enable_function;
    bool enable_sequence;
    uint32_t buffer_size;
    uint32_t max_entries;
    bool enable_rotation;
    uint32_t rotation_size;
    bool enable_compression;
    const char *file_path;
    const char *network_host;
    uint16_t network_port;
} log_config_t;

// Log statistics
typedef struct {
    uint32_t total_entries;
    uint32_t entries_per_level[6];
    uint32_t entries_per_category[9];
    uint32_t dropped_entries;
    uint32_t buffer_overruns;
    uint32_t file_errors;
    uint32_t network_errors;
    uint32_t last_rotation_time;
    uint32_t current_file_size;
    uint32_t total_bytes_written;
    uint32_t init_time;
    uint32_t uptime;
} log_stats_t;

// Function declarations
esp_err_t logger_init(void);
esp_err_t logger_deinit(void);
esp_err_t logger_set_config(const log_config_t *config);
esp_err_t logger_get_config(log_config_t *config);
esp_err_t logger_set_level(log_level_t level);
log_level_t logger_get_level(void);
esp_err_t logger_set_output(log_output_t output);
log_output_t logger_get_output(void);
esp_err_t logger_enable_category(log_category_t category, bool enable);
bool logger_is_category_enabled(log_category_t category);

// Logging functions
void logger_log(log_level_t level, log_category_t category, const char *tag, 
               const char *file, int line, const char *function, const char *format, ...);
void logger_error(log_category_t category, const char *tag, const char *format, ...);
void logger_warn(log_category_t category, const char *tag, const char *format, ...);
void logger_info(log_category_t category, const char *tag, const char *format, ...);
void logger_debug(log_category_t category, const char *tag, const char *format, ...);
void logger_verbose(log_category_t category, const char *tag, const char *format, ...);

// Buffer management
esp_err_t logger_flush(void);
esp_err_t logger_clear(void);
esp_err_t logger_get_entries(log_entry_t *entries, size_t max_entries, size_t *count);
esp_err_t logger_get_latest_entries(log_entry_t *entries, size_t max_entries, size_t *count);
esp_err_t logger_search_entries(const char *pattern, log_entry_t *entries, size_t max_entries, size_t *count);

// File operations
esp_err_t logger_save_to_file(const char *filename);
esp_err_t logger_load_from_file(const char *filename);
esp_err_t logger_rotate_files(void);
esp_err_t logger_compress_logs(void);

// Network operations
esp_err_t logger_start_network_logging(const char *host, uint16_t port);
esp_err_t logger_stop_network_logging(void);
bool logger_is_network_logging_active(void);

// Statistics and monitoring
esp_err_t logger_get_statistics(log_stats_t *stats);
void logger_print_statistics(void);
esp_err_t logger_reset_statistics(void);
esp_err_t logger_get_log_level_name(log_level_t level, char *name, size_t max_length);
esp_err_t logger_get_category_name(log_category_t category, char *name, size_t max_length);

// Advanced features
esp_err_t logger_set_filter(const char *tag_filter);
esp_err_t logger_set_callback(void (*callback)(const log_entry_t *entry));
esp_err_t logger_start_capture(void);
esp_err_t logger_stop_capture(void);
esp_err_t logger_get_captured_logs(log_entry_t *entries, size_t max_entries, size_t *count);

// Debug functions
void logger_dump_buffer(void);
void logger_dump_config(void);
esp_err_t logger_self_test(void);

// Macros for convenience
#define LOG_ERROR(cat, tag, ...) logger_error(cat, tag, __VA_ARGS__)
#define LOG_WARN(cat, tag, ...) logger_warn(cat, tag, __VA_ARGS__)
#define LOG_INFO(cat, tag, ...) logger_info(cat, tag, __VA_ARGS__)
#define LOG_DEBUG(cat, tag, ...) logger_debug(cat, tag, __VA_ARGS__)
#define LOG_VERBOSE(cat, tag, ...) logger_verbose(cat, tag, __VA_ARGS__)

#define LOGE(tag, ...) logger_error(LOG_CATEGORY_SYSTEM, tag, __VA_ARGS__)
#define LOGW(tag, ...) logger_warn(LOG_CATEGORY_SYSTEM, tag, __VA_ARGS__)
#define LOGI(tag, ...) logger_info(LOG_CATEGORY_SYSTEM, tag, __VA_ARGS__)
#define LOGD(tag, ...) logger_debug(LOG_CATEGORY_SYSTEM, tag, __VA_ARGS__)
#define LOGV(tag, ...) logger_verbose(LOG_CATEGORY_SYSTEM, tag, __VA_ARGS__)

// Constants
#define LOGGER_MAX_MESSAGE_LENGTH 256
#define LOGGER_MAX_TAG_LENGTH 32
#define LOGGER_MAX_FILE_LENGTH 64
#define LOGGER_MAX_FUNCTION_LENGTH 32
#define LOGGER_DEFAULT_BUFFER_SIZE 1024
#define LOGGER_DEFAULT_MAX_ENTRIES 1000
#define LOGGER_DEFAULT_ROTATION_SIZE 1024 * 1024  // 1MB
#define LOGGER_NETWORK_TIMEOUT_MS 5000
#define LOGGER_COLOR_RED     "\033[31m"
#define LOGGER_COLOR_GREEN   "\033[32m"
#define LOGGER_COLOR_YELLOW  "\033[33m"
#define LOGGER_COLOR_BLUE    "\033[34m"
#define LOGGER_COLOR_MAGENTA "\033[35m"
#define LOGGER_COLOR_CYAN    "\033[36m"
#define LOGGER_COLOR_WHITE   "\033[37m"
#define LOGGER_COLOR_RESET   "\033[0m"
