#include "logger.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/ringbuf.h"
#include "lwip/sockets.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define TAG "LOGGER"

// Global variables
static log_config_t log_config;
static log_stats_t log_stats;
static RingbufHandle_t log_buffer = NULL;
static SemaphoreHandle_t log_mutex = NULL;
static TaskHandle_t log_task_handle = NULL;
static int network_socket = -1;
static bool network_logging_active = false;
static FILE *log_file = NULL;
static void (*log_callback)(const log_entry_t *entry) = NULL;
static bool capture_active = false;
static uint32_t sequence_number = 0;

// Forward declarations
static void log_task(void *arg);
static void write_log_to_uart(const log_entry_t *entry);
static void write_log_to_file(const log_entry_t *entry);
static void write_log_to_network(const log_entry_t *entry);
static void write_log_to_memory(const log_entry_t *entry);
static const char* get_level_string(log_level_t level);
static const char* get_category_string(log_category_t category);
static void format_log_entry(const log_entry_t *entry, char *buffer, size_t buffer_size);

// Log task
static void log_task(void *arg)
{
    ESP_LOGI(TAG, "Logger task started");
    
    while (1) {
        if (log_buffer && xSemaphoreTake(log_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            size_t entry_size;
            log_entry_t *entry = (log_entry_t*)xRingbufferReceive(log_buffer, &entry_size, pdMS_TO_TICKS(10));
            
            while (entry) {
                // Write to configured outputs
                if (log_config.output == LOG_OUTPUT_UART || log_config.output == LOG_OUTPUT_ALL) {
                    write_log_to_uart(entry);
                }
                
                if (log_config.output == LOG_OUTPUT_FILE || log_config.output == LOG_OUTPUT_ALL) {
                    write_log_to_file(entry);
                }
                
                if ((log_config.output == LOG_OUTPUT_NETWORK || log_config.output == LOG_OUTPUT_ALL) && network_logging_active) {
                    write_log_to_network(entry);
                }
                
                if (log_config.output == LOG_OUTPUT_MEMORY || log_config.output == LOG_OUTPUT_ALL) {
                    write_log_to_memory(entry);
                }
                
                // Call user callback if registered
                if (log_callback) {
                    log_callback(entry);
                }
                
                // Update statistics
                log_stats.total_entries++;
                if (entry->level < 6) {
                    log_stats.entries_per_level[entry->level]++;
                }
                if (entry->category < 9) {
                    log_stats.entries_per_category[entry->category]++;
                }
                
                // Free entry memory
                free(entry);
                
                // Get next entry
                entry = (log_entry_t*)xRingbufferReceive(log_buffer, &entry_size, pdMS_TO_TICKS(0));
            }
            
            xSemaphoreGive(log_mutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Write log to UART
static void write_log_to_uart(const log_entry_t *entry)
{
    char formatted_entry[512];
    format_log_entry(entry, formatted_entry, sizeof(formatted_entry));
    
    printf("%s\n", formatted_entry);
    fflush(stdout);
}

// Write log to file
static void write_log_to_file(const char *filename)
{
    if (!log_file) {
        log_file = fopen(log_config.file_path ? log_config.file_path : "/spiffs/log.txt", "a");
        if (!log_file) {
            log_stats.file_errors++;
            return;
        }
    }
    
    char formatted_entry[512];
    format_log_entry(entry, formatted_entry, sizeof(formatted_entry));
    
    size_t written = fprintf(log_file, "%s\n", formatted_entry);
    if (written > 0) {
        log_stats.total_bytes_written += written;
        log_stats.current_file_size += written;
        
        // Check if rotation is needed
        if (log_config.enable_rotation && log_stats.current_file_size >= log_config.rotation_size) {
            logger_rotate_files();
        }
    } else {
        log_stats.file_errors++;
    }
    
    fflush(log_file);
}

// Write log to network
static void write_log_to_network(const log_entry_t *entry)
{
    if (network_socket < 0) {
        log_stats.network_errors++;
        return;
    }
    
    char formatted_entry[512];
    format_log_entry(entry, formatted_entry, sizeof(formatted_entry));
    
    // Add newline for network transmission
    strncat(formatted_entry, "\n", sizeof(formatted_entry) - strlen(formatted_entry) - 1);
    
    int sent = send(network_socket, formatted_entry, strlen(formatted_entry), 0);
    if (sent < 0) {
        log_stats.network_errors++;
        ESP_LOGE(TAG, "Failed to send log to network: %d", errno);
    }
}

// Write log to memory
static void write_log_to_memory(const log_entry_t *entry)
{
    // Entry is already in memory buffer, just update statistics
    // This is handled in the log task
}

// Format log entry
static void format_log_entry(const log_entry_t *entry, char *buffer, size_t buffer_size)
{
    const char *level_str = get_level_string(entry->level);
    const char *category_str = get_category_string(entry->category);
    
    if (log_config.enable_colors) {
        const char *color = "";
        switch (entry->level) {
            case LOG_LEVEL_ERROR: color = LOGGER_COLOR_RED; break;
            case LOG_LEVEL_WARN: color = LOGGER_COLOR_YELLOW; break;
            case LOG_LEVEL_INFO: color = LOGGER_COLOR_GREEN; break;
            case LOG_LEVEL_DEBUG: color = LOGGER_COLOR_BLUE; break;
            case LOG_LEVEL_VERBOSE: color = LOGGER_COLOR_CYAN; break;
            default: break;
        }
        
        if (log_config.enable_timestamp) {
            snprintf(buffer, buffer_size,
                    "%s[%lu] [%s] [%s] %s%s%s: %s%s",
                    color, entry->timestamp, level_str, category_str,
                    log_config.enable_file_line ? entry->file : "",
                    log_config.enable_file_line ? ":" : "",
                    log_config.enable_file_line ? entry->function : "",
                    entry->tag, entry->message, LOGGER_COLOR_RESET);
        } else {
            snprintf(buffer, buffer_size,
                    "%s[%s] [%s] %s%s%s: %s%s",
                    color, level_str, category_str,
                    log_config.enable_file_line ? entry->file : "",
                    log_config.enable_file_line ? ":" : "",
                    log_config.enable_file_line ? entry->function : "",
                    entry->tag, entry->message, LOGGER_COLOR_RESET);
        }
    } else {
        if (log_config.enable_timestamp) {
            snprintf(buffer, buffer_size,
                    "[%lu] [%s] [%s] %s%s%s: %s",
                    entry->timestamp, level_str, category_str,
                    log_config.enable_file_line ? entry->file : "",
                    log_config.enable_file_line ? ":" : "",
                    log_config.enable_file_line ? entry->function : "",
                    entry->tag, entry->message);
        } else {
            snprintf(buffer, buffer_size,
                    "[%s] [%s] %s%s%s: %s",
                    level_str, category_str,
                    log_config.enable_file_line ? entry->file : "",
                    log_config.enable_file_line ? ":" : "",
                    log_config.enable_file_line ? entry->function : "",
                    entry->tag, entry->message);
        }
    }
    
    if (log_config.enable_sequence) {
        char temp[512];
        snprintf(temp, sizeof(temp), "[%lu] %s", entry->sequence, buffer);
        strncpy(buffer, temp, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
}

// Get level string
static const char* get_level_string(log_level_t level)
{
    switch (level) {
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_WARN: return "WARN";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_VERBOSE: return "VERBOSE";
        default: return "UNKNOWN";
    }
}

// Get category string
static const char* get_category_string(log_category_t category)
{
    switch (category) {
        case LOG_CATEGORY_SYSTEM: return "SYSTEM";
        case LOG_CATEGORY_WIFI: return "WIFI";
        case LOG_CATEGORY_GPIO: return "GPIO";
        case LOG_CATEGORY_HTTP: return "HTTP";
        case LOG_CATEGORY_MEMORY: return "MEMORY";
        case LOG_CATEGORY_SCHEDULER: return "SCHEDULER";
        case LOG_CATEGORY_INTERRUPT: return "INTERRUPT";
        case LOG_CATEGORY_STORAGE: return "STORAGE";
        default: return "UNKNOWN";
    }
}

// Initialize logger
esp_err_t logger_init(void)
{
    ESP_LOGI(TAG, "Initializing logger");
    
    // Create mutex
    log_mutex = xSemaphoreCreateMutex();
    if (!log_mutex) {
        ESP_LOGE(TAG, "Failed to create logger mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Set default configuration
    memset(&log_config, 0, sizeof(log_config_t));
    log_config.min_level = LOG_LEVEL_INFO;
    log_config.output = LOG_OUTPUT_UART;
    log_config.enabled_categories = LOG_CATEGORY_ALL;
    log_config.enable_timestamp = true;
    log_config.enable_colors = true;
    log_config.enable_file_line = false;
    log_config.enable_function = false;
    log_config.enable_sequence = true;
    log_config.buffer_size = LOGGER_DEFAULT_BUFFER_SIZE;
    log_config.max_entries = LOGGER_DEFAULT_MAX_ENTRIES;
    log_config.enable_rotation = true;
    log_config.rotation_size = LOGGER_DEFAULT_ROTATION_SIZE;
    log_config.enable_compression = false;
    log_config.file_path = "/spiffs/log.txt";
    log_config.network_host = "192.168.1.100";
    log_config.network_port = 514;
    
    // Reset statistics
    memset(&log_stats, 0, sizeof(log_stats_t));
    log_stats.init_time = esp_timer_get_time() / 1000;
    
    // Create log buffer
    log_buffer = xRingbufferCreate(log_config.buffer_size, RINGBUF_TYPE_NOSPLIT);
    if (!log_buffer) {
        ESP_LOGE(TAG, "Failed to create log buffer");
        return ESP_ERR_NO_MEM;
    }
    
    // Create logger task
    BaseType_t ret = xTaskCreate(log_task, "logger_task", 4096, NULL, 5, &log_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create logger task");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Logger initialized successfully");
    return ESP_OK;
}

// Log function
void logger_log(log_level_t level, log_category_t category, const char *tag, 
               const char *file, int line, const char *function, const char *format, ...)
{
    if (!tag || !format || !log_buffer) {
        return;
    }
    
    // Check if level is enabled
    if (level > log_config.min_level) {
        return;
    }
    
    // Check if category is enabled
    if (!(log_config.enabled_categories & (1 << category)) && log_config.enabled_categories != LOG_CATEGORY_ALL) {
        return;
    }
    
    // Create log entry
    log_entry_t *entry = malloc(sizeof(log_entry_t));
    if (!entry) {
        log_stats.dropped_entries++;
        return;
    }
    
    memset(entry, 0, sizeof(log_entry_t));
    entry->timestamp = esp_timer_get_time() / 1000;
    entry->level = level;
    entry->category = category;
    entry->tag = tag;
    entry->file = file;
    entry->line = line;
    entry->function = function;
    entry->sequence = sequence_number++;
    
    // Format message
    va_list args;
    va_start(args, format);
    vsnprintf(entry->message, sizeof(entry->message), format, args);
    va_end(args);
    
    // Add to buffer
    BaseType_t result = xRingbufferSend(log_buffer, entry, 0);
    if (result != pdTRUE) {
        free(entry);
        log_stats.dropped_entries++;
        log_stats.buffer_overruns++;
    }
}

// Convenience functions
void logger_error(log_category_t category, const char *tag, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char message[256];
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    logger_log(LOG_LEVEL_ERROR, category, tag, __FILE__, __LINE__, __FUNCTION__, "%s", message);
}

void logger_warn(log_category_t category, const char *tag, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char message[256];
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    logger_log(LOG_LEVEL_WARN, category, tag, __FILE__, __LINE__, __FUNCTION__, "%s", message);
}

void logger_info(log_category_t category, const char *tag, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char message[256];
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    logger_log(LOG_LEVEL_INFO, category, tag, __FILE__, __LINE__, __FUNCTION__, "%s", message);
}

void logger_debug(log_category_t category, const char *tag, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char message[256];
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    logger_log(LOG_LEVEL_DEBUG, category, tag, __FILE__, __LINE__, __FUNCTION__, "%s", message);
}

void logger_verbose(log_category_t category, const char *tag, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char message[256];
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    logger_log(LOG_LEVEL_VERBOSE, category, tag, __FILE__, __LINE__, __FUNCTION__, "%s", message);
}

// Set log level
esp_err_t logger_set_level(log_level_t level)
{
    if (level > LOG_LEVEL_VERBOSE) {
        return ESP_ERR_INVALID_ARG;
    }
    
    log_config.min_level = level;
    return ESP_OK;
}

// Get log level
log_level_t logger_get_level(void)
{
    return log_config.min_level;
}

// Get statistics
esp_err_t logger_get_statistics(log_stats_t *stats)
{
    if (!stats) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *stats = log_stats;
    stats->uptime = (esp_timer_get_time() / 1000) - log_stats.init_time;
    
    return ESP_OK;
}

// Print statistics
void logger_print_statistics(void)
{
    log_stats_t stats;
    if (logger_get_statistics(&stats) == ESP_OK) {
        ESP_LOGI(TAG, "=== Logger Statistics ===");
        ESP_LOGI(TAG, "Total entries: %d", stats.total_entries);
        ESP_LOGI(TAG, "Dropped entries: %d", stats.dropped_entries);
        ESP_LOGI(TAG, "Buffer overruns: %d", stats.buffer_overruns);
        ESP_LOGI(TAG, "File errors: %d", stats.file_errors);
        ESP_LOGI(TAG, "Network errors: %d", stats.network_errors);
        ESP_LOGI(TAG, "Total bytes written: %d", stats.total_bytes_written);
        ESP_LOGI(TAG, "Current file size: %d", stats.current_file_size);
        ESP_LOGI(TAG, "Uptime: %d seconds", stats.uptime);
        
        ESP_LOGI(TAG, "Entries by level:");
        ESP_LOGI(TAG, "  ERROR: %d", stats.entries_per_level[LOG_LEVEL_ERROR]);
        ESP_LOGI(TAG, "  WARN: %d", stats.entries_per_level[LOG_LEVEL_WARN]);
        ESP_LOGI(TAG, "  INFO: %d", stats.entries_per_level[LOG_LEVEL_INFO]);
        ESP_LOGI(TAG, "  DEBUG: %d", stats.entries_per_level[LOG_LEVEL_DEBUG]);
        ESP_LOGI(TAG, "  VERBOSE: %d", stats.entries_per_level[LOG_LEVEL_VERBOSE]);
        
        ESP_LOGI(TAG, "Entries by category:");
        ESP_LOGI(TAG, "  SYSTEM: %d", stats.entries_per_level[LOG_CATEGORY_SYSTEM]);
        ESP_LOGI(TAG, "  WIFI: %d", stats.entries_per_level[LOG_CATEGORY_WIFI]);
        ESP_LOGI(TAG, "  GPIO: %d", stats.entries_per_level[LOG_CATEGORY_GPIO]);
        ESP_LOGI(TAG, "  HTTP: %d", stats.entries_per_level[LOG_CATEGORY_HTTP]);
        ESP_LOGI(TAG, "  MEMORY: %d", stats.entries_per_level[LOG_CATEGORY_MEMORY]);
        ESP_LOGI(TAG, "  SCHEDULER: %d", stats.entries_per_level[LOG_CATEGORY_SCHEDULER]);
        ESP_LOGI(TAG, "  INTERRUPT: %d", stats.entries_per_level[LOG_CATEGORY_INTERRUPT]);
        ESP_LOGI(TAG, "  STORAGE: %d", stats.entries_per_level[LOG_CATEGORY_STORAGE]);
        
        ESP_LOGI(TAG, "========================");
    }
}

// Self test
esp_err_t logger_self_test(void)
{
    ESP_LOGI(TAG, "Starting logger self-test...");
    
    esp_err_t result = ESP_OK;
    
    // Test basic logging
    logger_info(LOG_CATEGORY_SYSTEM, "test", "Logger self-test started");
    logger_error(LOG_CATEGORY_SYSTEM, "test", "Test error message");
    logger_warn(LOG_CATEGORY_SYSTEM, "test", "Test warning message");
    logger_debug(LOG_CATEGORY_SYSTEM, "test", "Test debug message");
    
    // Test level filtering
    log_level_t original_level = logger_get_level();
    logger_set_level(LOG_LEVEL_ERROR);
    logger_info(LOG_CATEGORY_SYSTEM, "test", "This should not appear");
    logger_set_level(original_level);
    
    // Test statistics
    log_stats_t stats;
    esp_err_t err = logger_get_statistics(&stats);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get logger statistics: %s", esp_err_to_name(err));
        result = ESP_FAIL;
    }
    
    if (stats.total_entries < 4) {
        ESP_LOGE(TAG, "Not enough log entries recorded");
        result = ESP_FAIL;
    }
    
    logger_info(LOG_CATEGORY_SYSTEM, "test", "Logger self-test %s", result == ESP_OK ? "PASSED" : "FAILED");
    return result;
}
