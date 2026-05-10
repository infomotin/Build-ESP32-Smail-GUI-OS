#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Memory pool block sizes
#define POOL_BLOCK_SIZE_SMALL    64
#define POOL_BLOCK_SIZE_MEDIUM   256
#define POOL_BLOCK_SIZE_LARGE    1024
#define POOL_BLOCK_SIZE_XLARGE   4096
#define POOL_BLOCKS_SMALL        32
#define POOL_BLOCKS_MEDIUM       16
#define POOL_BLOCKS_LARGE        8
#define POOL_BLOCKS_XLARGE       4

// Memory allocation strategies
#define ALLOC_STRATEGY_BEST_FIT   0
#define ALLOC_STRATEGY_FIRST_FIT  1
#define ALLOC_STRATEGY_WORST_FIT  2
#define ALLOC_STRATEGY_BUDDY     3

// Memory pool types
typedef enum {
    MEMORY_POOL_SMALL = 0,
    MEMORY_POOL_MEDIUM,
    MEMORY_POOL_LARGE,
    MEMORY_POOL_XLARGE,
    MEMORY_POOL_COUNT
} memory_pool_type_t;

// Memory block status
typedef enum {
    BLOCK_STATUS_FREE = 0,
    BLOCK_STATUS_ALLOCATED,
    BLOCK_STATUS_RESERVED
} block_status_t;

// Memory block header
typedef struct memory_block {
    struct memory_block *next;
    struct memory_block *prev;
    size_t size;
    block_status_t status;
    uint32_t magic;
    void *owner;
    uint32_t alloc_time;
    uint32_t alloc_count;
} memory_block_t;

// Memory pool structure
typedef struct {
    void *memory;
    size_t total_size;
    size_t block_size;
    uint32_t block_count;
    uint32_t free_blocks;
    uint32_t allocated_blocks;
    uint32_t *free_bitmap;
    memory_block_t *free_list;
    memory_block_t *allocated_list;
    uint32_t total_allocations;
    uint32_t total_deallocations;
    uint32_t peak_usage;
    uint32_t fragmentation;
    bool initialized;
    char name[16];
} memory_pool_t;

// Memory statistics
typedef struct {
    uint32_t total_memory;
    uint32_t free_memory;
    uint32_t allocated_memory;
    uint32_t peak_memory;
    uint32_t fragmentation_percent;
    uint32_t total_allocations;
    uint32_t total_deallocations;
    uint32_t failed_allocations;
    uint32_t largest_free_block;
    uint32_t smallest_free_block;
    uint32_t average_block_size;
    uint32_t memory_leaks;
    uint32_t init_time;
    uint32_t uptime;
} memory_stats_t;

// Memory configuration
typedef struct {
    uint32_t small_pool_size;
    uint32_t medium_pool_size;
    uint32_t large_pool_size;
    uint32_t xlarge_pool_size;
    uint32_t allocation_strategy;
    bool enable_debugging;
    bool enable_statistics;
    bool enable_leak_detection;
    bool enable_defragmentation;
    uint32_t defragmentation_threshold;
    uint32_t leak_detection_interval;
} memory_config_t;

// Memory allocation tracking
typedef struct {
    void *ptr;
    size_t size;
    uint32_t timestamp;
    const char *file;
    int line;
    const char *function;
} allocation_record_t;

// Function declarations
esp_err_t memory_pool_init(const memory_config_t *config);
esp_err_t memory_pool_deinit(void);
void* memory_pool_alloc(size_t size);
void* memory_pool_alloc_debug(size_t size, const char *file, int line, const char *function);
void memory_pool_free(void *ptr);
void memory_pool_free_debug(void *ptr, const char *file, int line, const char *function);
void* memory_pool_realloc(void *ptr, size_t new_size);
void* memory_pool_calloc(size_t count, size_t size);

// Pool-specific functions
esp_err_t memory_pool_create(memory_pool_t *pool, size_t block_size, uint32_t block_count, const char *name);
esp_err_t memory_pool_destroy(memory_pool_t *pool);
void* memory_pool_alloc_from_pool(memory_pool_t *pool, size_t size);
void memory_pool_free_to_pool(memory_pool_t *pool, void *ptr);
esp_err_t memory_pool_get_stats(memory_pool_t *pool, memory_stats_t *stats);
esp_err_t memory_pool_defragment(memory_pool_t *pool);
bool memory_pool_is_valid_ptr(memory_pool_t *pool, void *ptr);

// Utility functions
size_t memory_pool_get_free_memory(void);
size_t memory_pool_get_allocated_memory(void);
size_t memory_pool_get_total_memory(void);
uint32_t memory_pool_get_fragmentation(void);
esp_err_t memory_pool_get_statistics(memory_stats_t *stats);
void memory_pool_print_statistics(void);
esp_err_t memory_pool_reset_statistics(void);

// Debug functions
void memory_pool_dump_pools(void);
void memory_pool_dump_pool(memory_pool_t *pool);
void memory_pool_dump_allocations(void);
esp_err_t memory_pool_detect_leaks(void);
esp_err_t memory_pool_validate(void);
esp_err_t memory_pool_self_test(void);

// Performance monitoring
esp_err_t memory_pool_get_performance(uint32_t *avg_alloc_time, uint32_t *max_alloc_time);
esp_err_t memory_pool_get_fragmentation_info(uint32_t *internal_frag, uint32_t *external_frag);

// Advanced allocation strategies
void* memory_best_fit_alloc(size_t size);
void* memory_first_fit_alloc(size_t size);
void* memory_worst_fit_alloc(size_t size);
void* memory_buddy_alloc(size_t size);

// Memory protection
esp_err_t memory_pool_set_protection(void *ptr, size_t size, bool read_only);
esp_err_t memory_pool_check_bounds(void *ptr);
esp_err_t memory_pool_set_guard_pattern(void *ptr, size_t size);
esp_err_t memory_pool_check_guard_pattern(void *ptr);

// Memory alignment
void* memory_pool_aligned_alloc(size_t size, size_t alignment);
esp_err_t memory_pool_aligned_free(void *ptr);

// Memory mapping
esp_err_t memory_pool_map_region(void *addr, size_t size, uint32_t flags);
esp_err_t memory_pool_unmap_region(void *addr, size_t size);

// Memory compression
esp_err_t memory_pool_compress(memory_pool_t *pool);
esp_err_t memory_pool_decompress(memory_pool_t *pool, void *compressed_data, size_t compressed_size);

// Memory sharing
esp_err_t memory_pool_share_block(void *ptr, uint32_t *share_id);
esp_err_t memory_pool_unshare_block(uint32_t share_id);
void* memory_pool_get_shared_block(uint32_t share_id);

// Memory pools for specific use cases
esp_err_t memory_pool_create_network_pool(size_t size);
esp_err_t memory_pool_create_http_pool(size_t size);
esp_err_t memory_pool_create_gpio_pool(size_t size);
esp_err_t memory_pool_create_task_pool(size_t size);

// Memory pool configuration
#define MEMORY_POOL_DEBUG 1
#define MEMORY_POOL_STATS 1
#define MEMORY_POOL_LEAK_DETECTION 1
#define MEMORY_POOL_DEFRAGMENTATION 1
#define MEMORY_POOL_ALIGNMENT 8
#define MEMORY_POOL_GUARD_PATTERN 0xDEADBEEF
#define MEMORY_POOL_FREE_PATTERN 0xFEFEFEFE
#define MEMORY_POOL_ALLOCATED_PATTERN 0xABABABAB

// Debug macros
#if MEMORY_POOL_DEBUG
    #define MEMORY_ALLOC(size) memory_pool_alloc_debug(size, __FILE__, __LINE__, __FUNCTION__)
    #define MEMORY_FREE(ptr) memory_pool_free_debug(ptr, __FILE__, __LINE__, __FUNCTION__)
    #define MEMORY_REALLOC(ptr, size) memory_pool_realloc_debug(ptr, size, __FILE__, __LINE__, __FUNCTION__)
#else
    #define MEMORY_ALLOC(size) memory_pool_alloc(size)
    #define MEMORY_FREE(ptr) memory_pool_free(ptr)
    #define MEMORY_REALLOC(ptr, size) memory_pool_realloc(ptr, size)
#endif

// Performance macros
#define MEMORY_POOL_TIMING 1
#if MEMORY_POOL_TIMING
    #define MEMORY_TIME_START() uint32_t start_time = esp_timer_get_time()
    #define MEMORY_TIME_END() (esp_timer_get_time() - start_time)
#else
    #define MEMORY_TIME_START() 0
    #define MEMORY_TIME_END() 0
#endif
