#include "memory_pool.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include <string.h>
#include <stdlib.h>

#define TAG "MEMORY_POOL"

// Global memory pools
static memory_pool_t pools[MEMORY_POOL_COUNT];
static memory_config_t memory_config;
static memory_stats_t memory_stats;
static bool memory_pool_initialized = false;
static uint32_t memory_init_time = 0;

// Static memory pools
static uint8_t small_memory[POOL_BLOCK_SIZE_SMALL * POOL_BLOCKS_SMALL];
static uint8_t medium_memory[POOL_BLOCK_SIZE_MEDIUM * POOL_BLOCKS_MEDIUM];
static uint8_t large_memory[POOL_BLOCK_SIZE_LARGE * POOL_BLOCKS_LARGE];
static uint8_t xlarge_memory[POOL_BLOCK_SIZE_XLARGE * POOL_BLOCKS_XLARGE];

// Bitmaps for pool management
static uint32_t small_bitmap[(POOL_BLOCKS_SMALL + 31) / 32];
static uint32_t medium_bitmap[(POOL_BLOCKS_MEDIUM + 31) / 32];
static uint32_t large_bitmap[(POOL_BLOCKS_LARGE + 31) / 32];
static uint32_t xlarge_bitmap[(POOL_BLOCKS_XLARGE + 31) / 32];

// Allocation tracking
static allocation_record_t allocation_records[256];
static uint32_t allocation_count = 0;

// Performance tracking
static uint32_t total_alloc_time = 0;
static uint32_t max_alloc_time = 0;
static uint32_t alloc_count = 0;

esp_err_t memory_pool_init(const memory_config_t *config)
{
    if (!config) {
        ESP_LOGE(TAG, "Invalid memory configuration");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (memory_pool_initialized) {
        ESP_LOGW(TAG, "Memory pool already initialized");
        return ESP_OK;
    }
    
    // Store configuration
    memory_config = *config;
    
    // Initialize statistics
    memset(&memory_stats, 0, sizeof(memory_stats_t));
    memory_init_time = esp_timer_get_time() / 1000;
    memory_stats.init_time = memory_init_time;
    
    // Clear allocation records
    memset(allocation_records, 0, sizeof(allocation_records));
    allocation_count = 0;
    
    // Clear performance tracking
    total_alloc_time = 0;
    max_alloc_time = 0;
    alloc_count = 0;
    
    // Initialize small pool
    esp_err_t err = memory_pool_create(&pools[MEMORY_POOL_SMALL], 
                                    POOL_BLOCK_SIZE_SMALL, POOL_BLOCKS_SMALL, "small");
    if (err != ESP_OK) return err;
    
    // Initialize medium pool
    err = memory_pool_create(&pools[MEMORY_POOL_MEDIUM], 
                           POOL_BLOCK_SIZE_MEDIUM, POOL_BLOCKS_MEDIUM, "medium");
    if (err != ESP_OK) return err;
    
    // Initialize large pool
    err = memory_pool_create(&pools[MEMORY_POOL_LARGE], 
                           POOL_BLOCK_SIZE_LARGE, POOL_BLOCKS_LARGE, "large");
    if (err != ESP_OK) return err;
    
    // Initialize xlarge pool
    err = memory_pool_create(&pools[MEMORY_POOL_XLARGE], 
                           POOL_BLOCK_SIZE_XLARGE, POOL_BLOCKS_XLARGE, "xlarge");
    if (err != ESP_OK) return err;
    
    memory_pool_initialized = true;
    
    ESP_LOGI(TAG, "Memory pool initialized: %d KB total", 
              (POOL_BLOCK_SIZE_SMALL * POOL_BLOCKS_SMALL + 
               POOL_BLOCK_SIZE_MEDIUM * POOL_BLOCKS_MEDIUM + 
               POOL_BLOCK_SIZE_LARGE * POOL_BLOCKS_LARGE + 
               POOL_BLOCK_SIZE_XLARGE * POOL_BLOCKS_XLARGE) / 1024);
    
    return ESP_OK;
}

esp_err_t memory_pool_create(memory_pool_t *pool, size_t block_size, uint32_t block_count, const char *name)
{
    if (!pool || block_size == 0 || block_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Determine memory source based on pool type
    void *memory = NULL;
    uint32_t *bitmap = NULL;
    
    if (strcmp(name, "small") == 0) {
        memory = small_memory;
        bitmap = small_bitmap;
    } else if (strcmp(name, "medium") == 0) {
        memory = medium_memory;
        bitmap = medium_bitmap;
    } else if (strcmp(name, "large") == 0) {
        memory = large_memory;
        bitmap = large_bitmap;
    } else if (strcmp(name, "xlarge") == 0) {
        memory = xlarge_memory;
        bitmap = xlarge_bitmap;
    } else {
        // Dynamic allocation for custom pools
        memory = malloc(block_size * block_count);
        bitmap = malloc(((block_count + 31) / 32) * sizeof(uint32_t));
        if (!memory || !bitmap) {
            if (memory) free(memory);
            if (bitmap) free(bitmap);
            return ESP_ERR_NO_MEM;
        }
    }
    
    // Initialize pool structure
    memset(pool, 0, sizeof(memory_pool_t));
    pool->memory = memory;
    pool->total_size = block_size * block_count;
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->free_blocks = block_count;
    pool->allocated_blocks = 0;
    pool->free_bitmap = bitmap;
    pool->free_list = NULL;
    pool->allocated_list = NULL;
    pool->total_allocations = 0;
    pool->total_deallocations = 0;
    pool->peak_usage = 0;
    pool->fragmentation = 0;
    pool->initialized = true;
    strncpy(pool->name, name, sizeof(pool->name) - 1);
    pool->name[sizeof(pool->name) - 1] = '\0';
    
    // Initialize bitmap (all blocks free)
    uint32_t bitmap_size = (block_count + 31) / 32;
    memset(bitmap, 0xFF, bitmap_size * sizeof(uint32_t));
    
    // Set guard patterns
    memset(memory, MEMORY_POOL_FREE_PATTERN, pool->total_size);
    
    ESP_LOGD(TAG, "Created memory pool '%s': %d blocks of %d bytes", 
              name, block_count, block_size);
    
    return ESP_OK;
}

void* memory_pool_alloc(size_t size)
{
    if (!memory_pool_initialized || size == 0) {
        return NULL;
    }
    
    uint32_t start_time = MEMORY_TIME_START();
    void *ptr = NULL;
    
    // Determine appropriate pool based on size
    memory_pool_t *pool = NULL;
    
    if (size <= POOL_BLOCK_SIZE_SMALL) {
        pool = &pools[MEMORY_POOL_SMALL];
    } else if (size <= POOL_BLOCK_SIZE_MEDIUM) {
        pool = &pools[MEMORY_POOL_MEDIUM];
    } else if (size <= POOL_BLOCK_SIZE_LARGE) {
        pool = &pools[MEMORY_POOL_LARGE];
    } else if (size <= POOL_BLOCK_SIZE_XLARGE) {
        pool = &pools[MEMORY_POOL_XLARGE];
    } else {
        // Fall back to heap allocation for large requests
        ptr = malloc(size);
        if (ptr) {
            // Track allocation
            if (allocation_count < sizeof(allocation_records) / sizeof(allocation_records[0])) {
                allocation_records[allocation_count].ptr = ptr;
                allocation_records[allocation_count].size = size;
                allocation_records[allocation_count].timestamp = esp_timer_get_time() / 1000;
                allocation_records[allocation_count].file = "heap";
                allocation_records[allocation_count].line = 0;
                allocation_records[allocation_count].function = "malloc";
                allocation_count++;
            }
        }
        return ptr;
    }
    
    // Allocate from selected pool
    ptr = memory_pool_alloc_from_pool(pool, size);
    
    // Update performance statistics
    uint32_t alloc_time = MEMORY_TIME_END();
    total_alloc_time += alloc_time;
    if (alloc_time > max_alloc_time) {
        max_alloc_time = alloc_time;
    }
    alloc_count++;
    
    return ptr;
}

void* memory_pool_alloc_debug(size_t size, const char *file, int line, const char *function)
{
    void *ptr = memory_pool_alloc(size);
    
    if (ptr && memory_config.enable_debugging) {
        // Track allocation with debug info
        if (allocation_count < sizeof(allocation_records) / sizeof(allocation_records[0])) {
            allocation_records[allocation_count].ptr = ptr;
            allocation_records[allocation_count].size = size;
            allocation_records[allocation_count].timestamp = esp_timer_get_time() / 1000;
            allocation_records[allocation_count].file = file;
            allocation_records[allocation_count].line = line;
            allocation_records[allocation_count].function = function;
            allocation_count++;
        }
        
        ESP_LOGD(TAG, "Allocated %d bytes at %p from %s:%d (%s)", 
                  size, ptr, file, line, function);
    }
    
    return ptr;
}

void* memory_pool_alloc_from_pool(memory_pool_t *pool, size_t size)
{
    if (!pool || !pool->initialized || size > pool->block_size) {
        return NULL;
    }
    
    // Find free block using configured strategy
    uint32_t block_index = 0;
    bool found = false;
    
    switch (memory_config.allocation_strategy) {
        case ALLOC_STRATEGY_FIRST_FIT:
            found = memory_first_fit_find(pool, size, &block_index);
            break;
        case ALLOC_STRATEGY_BEST_FIT:
            found = memory_best_fit_find(pool, size, &block_index);
            break;
        case ALLOC_STRATEGY_WORST_FIT:
            found = memory_worst_fit_find(pool, size, &block_index);
            break;
        default:
            found = memory_first_fit_find(pool, size, &block_index);
            break;
    }
    
    if (!found) {
        ESP_LOGW(TAG, "No free blocks in pool '%s'", pool->name);
        return NULL;
    }
    
    // Mark block as allocated
    uint32_t bitmap_index = block_index / 32;
    uint32_t bit_index = block_index % 32;
    pool->free_bitmap[bitmap_index] &= ~(1 << bit_index);
    
    // Get block pointer
    uint8_t *block_ptr = (uint8_t*)pool->memory + (block_index * pool->block_size);
    
    // Set guard pattern
    if (memory_config.enable_debugging) {
        memory_pool_set_guard_pattern(block_ptr, size);
    }
    
    // Update pool statistics
    pool->free_blocks--;
    pool->allocated_blocks++;
    pool->total_allocations++;
    
    if (pool->allocated_blocks > pool->peak_usage) {
        pool->peak_usage = pool->allocated_blocks;
    }
    
    // Update global statistics
    memory_stats.total_allocations++;
    memory_stats.allocated_memory += pool->block_size;
    memory_stats.free_memory -= pool->block_size;
    
    ESP_LOGD(TAG, "Allocated block %d from pool '%s' (%p)", 
              block_index, pool->name, block_ptr);
    
    return block_ptr;
}

void memory_pool_free(void *ptr)
{
    if (!ptr || !memory_pool_initialized) {
        return;
    }
    
    // Find which pool contains this pointer
    memory_pool_t *pool = NULL;
    uint32_t block_index = 0;
    
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        if (memory_pool_is_valid_ptr(&pools[i], ptr)) {
            pool = &pools[i];
            block_index = ((uint8_t*)ptr - (uint8_t*)pool->memory) / pool->block_size;
            break;
        }
    }
    
    if (pool) {
        memory_pool_free_to_pool(pool, ptr);
    } else {
        // Assume heap allocation
        free(ptr);
        
        // Remove from allocation records
        for (uint32_t i = 0; i < allocation_count; i++) {
            if (allocation_records[i].ptr == ptr) {
                // Shift remaining records
                for (uint32_t j = i; j < allocation_count - 1; j++) {
                    allocation_records[j] = allocation_records[j + 1];
                }
                allocation_count--;
                break;
            }
        }
    }
}

void memory_pool_free_debug(void *ptr, const char *file, int line, const char *function)
{
    if (memory_config.enable_debugging) {
        ESP_LOGD(TAG, "Freeing %p from %s:%d (%s)", ptr, file, line, function);
    }
    
    memory_pool_free(ptr);
}

void memory_pool_free_to_pool(memory_pool_t *pool, void *ptr)
{
    if (!pool || !pool->initialized || !ptr) {
        return;
    }
    
    // Calculate block index
    uint32_t block_index = ((uint8_t*)ptr - (uint8_t*)pool->memory) / pool->block_size;
    
    if (block_index >= pool->block_count) {
        ESP_LOGE(TAG, "Invalid pointer %p for pool '%s'", ptr, pool->name);
        return;
    }
    
    // Check guard pattern
    if (memory_config.enable_debugging) {
        if (memory_pool_check_guard_pattern(ptr) != ESP_OK) {
            ESP_LOGE(TAG, "Memory corruption detected in pool '%s', block %d", 
                      pool->name, block_index);
        }
    }
    
    // Mark block as free
    uint32_t bitmap_index = block_index / 32;
    uint32_t bit_index = block_index % 32;
    pool->free_bitmap[bitmap_index] |= (1 << bit_index);
    
    // Clear memory
    memset(ptr, MEMORY_POOL_FREE_PATTERN, pool->block_size);
    
    // Update pool statistics
    pool->free_blocks++;
    pool->allocated_blocks--;
    pool->total_deallocations++;
    
    // Update global statistics
    memory_stats.total_deallocations++;
    memory_stats.allocated_memory -= pool->block_size;
    memory_stats.free_memory += pool->block_size;
    
    ESP_LOGD(TAG, "Freed block %d from pool '%s' (%p)", 
              block_index, pool->name, ptr);
}

bool memory_pool_is_valid_ptr(memory_pool_t *pool, void *ptr)
{
    if (!pool || !pool->initialized || !ptr) {
        return false;
    }
    
    return (ptr >= pool->memory && 
            ptr < (void*)((uint8_t*)pool->memory + pool->total_size));
}

esp_err_t memory_pool_get_statistics(memory_stats_t *stats)
{
    if (!stats) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *stats = memory_stats;
    stats->uptime = (esp_timer_get_time() / 1000) - memory_init_time;
    
    // Calculate fragmentation
    stats->fragmentation_percent = memory_pool_get_fragmentation();
    
    // Calculate average block size
    if (memory_stats.total_allocations > 0) {
        stats->average_block_size = memory_stats.allocated_memory / memory_stats.total_allocations;
    }
    
    return ESP_OK;
}

size_t memory_pool_get_free_memory(void)
{
    size_t free_memory = 0;
    
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        free_memory += pools[i].free_blocks * pools[i].block_size;
    }
    
    return free_memory;
}

size_t memory_pool_get_allocated_memory(void)
{
    size_t allocated_memory = 0;
    
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        allocated_memory += pools[i].allocated_blocks * pools[i].block_size;
    }
    
    return allocated_memory;
}

size_t memory_pool_get_total_memory(void)
{
    size_t total_memory = 0;
    
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        total_memory += pools[i].total_size;
    }
    
    return total_memory;
}

uint32_t memory_pool_get_fragmentation(void)
{
    uint32_t total_blocks = 0;
    uint32_t free_blocks = 0;
    uint32_t largest_free = 0;
    uint32_t current_free = 0;
    
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        total_blocks += pools[i].block_count;
        free_blocks += pools[i].free_blocks;
        
        // Find largest contiguous free block
        for (uint32_t j = 0; j < pools[i].block_count; j++) {
            uint32_t bitmap_index = j / 32;
            uint32_t bit_index = j % 32;
            
            if (pools[i].free_bitmap[bitmap_index] & (1 << bit_index)) {
                current_free++;
                if (current_free > largest_free) {
                    largest_free = current_free;
                }
            } else {
                current_free = 0;
            }
        }
    }
    
    if (free_blocks == 0) return 0;
    if (total_blocks == 0) return 100;
    
    // Fragmentation = (1 - (largest_free / total_free_blocks)) * 100
    return (100 - (largest_free * 100 / free_blocks));
}

void memory_pool_print_statistics(void)
{
    memory_stats_t stats;
    memory_pool_get_statistics(&stats);
    
    ESP_LOGI(TAG, "=== Memory Pool Statistics ===");
    ESP_LOGI(TAG, "Uptime: %d seconds", stats.uptime);
    ESP_LOGI(TAG, "Total memory: %d KB", stats.total_memory / 1024);
    ESP_LOGI(TAG, "Free memory: %d KB", stats.free_memory / 1024);
    ESP_LOGI(TAG, "Allocated memory: %d KB", stats.allocated_memory / 1024);
    ESP_LOGI(TAG, "Peak memory: %d KB", stats.peak_memory / 1024);
    ESP_LOGI(TAG, "Fragmentation: %d%%", stats.fragmentation_percent);
    ESP_LOGI(TAG, "Total allocations: %d", stats.total_allocations);
    ESP_LOGI(TAG, "Total deallocations: %d", stats.total_deallocations);
    ESP_LOGI(TAG, "Failed allocations: %d", stats.failed_allocations);
    ESP_LOGI(TAG, "Memory leaks: %d", stats.memory_leaks);
    ESP_LOGI(TAG, "Average block size: %d bytes", stats.average_block_size);
    
    // Per-pool statistics
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        ESP_LOGI(TAG, "Pool '%s': %d/%d blocks free (%d KB)", 
                  pools[i].name, pools[i].free_blocks, pools[i].block_count,
                  (pools[i].free_blocks * pools[i].block_size) / 1024);
    }
    
    ESP_LOGI(TAG, "Performance: avg alloc %d us, max alloc %d us", 
              alloc_count > 0 ? total_alloc_time / alloc_count : 0, max_alloc_time);
    ESP_LOGI(TAG, "================================");
}

esp_err_t memory_pool_detect_leaks(void)
{
    if (!memory_config.enable_leak_detection) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint32_t leak_count = 0;
    uint32_t total_leaked = 0;
    
    for (uint32_t i = 0; i < allocation_count; i++) {
        bool found = false;
        
        // Check if allocation is still valid
        for (int j = 0; j < MEMORY_POOL_COUNT; j++) {
            if (memory_pool_is_valid_ptr(&pools[j], allocation_records[i].ptr)) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            // This might be a heap allocation, check if it's still allocated
            // (simplified check - in reality would need more sophisticated tracking)
            continue;
        }
        
        // Found a leak
        leak_count++;
        total_leaked += allocation_records[i].size;
        
        ESP_LOGW(TAG, "Memory leak detected: %d bytes at %p allocated at %s:%d (%s)", 
                  allocation_records[i].size, allocation_records[i].ptr,
                  allocation_records[i].file, allocation_records[i].line,
                  allocation_records[i].function);
    }
    
    memory_stats.memory_leaks = leak_count;
    
    ESP_LOGI(TAG, "Leak detection complete: %d leaks, %d bytes total", 
              leak_count, total_leaked);
    
    return leak_count > 0 ? ESP_ERR_INVALID_STATE : ESP_OK;
}

esp_err_t memory_pool_validate(void)
{
    uint32_t corruption_count = 0;
    
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        memory_pool_t *pool = &pools[i];
        
        if (!pool->initialized) continue;
        
        // Check each allocated block
        for (uint32_t j = 0; j < pool->block_count; j++) {
            uint32_t bitmap_index = j / 32;
            uint32_t bit_index = j % 32;
            
            if (!(pool->free_bitmap[bitmap_index] & (1 << bit_index))) {
                // Block is allocated, check guard pattern
                uint8_t *block_ptr = (uint8_t*)pool->memory + (j * pool->block_size);
                
                if (memory_pool_check_guard_pattern(block_ptr) != ESP_OK) {
                    corruption_count++;
                    ESP_LOGE(TAG, "Memory corruption in pool '%s', block %d", 
                              pool->name, j);
                }
            }
        }
    }
    
    if (corruption_count > 0) {
        ESP_LOGE(TAG, "Memory validation failed: %d corruptions detected", corruption_count);
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Memory validation passed");
    return ESP_OK;
}

esp_err_t memory_pool_set_guard_pattern(void *ptr, size_t size)
{
    if (!ptr || size < sizeof(uint32_t)) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Set guard pattern at end of block
    uint32_t *guard = (uint32_t*)((uint8_t*)ptr + size - sizeof(uint32_t));
    *guard = MEMORY_POOL_GUARD_PATTERN;
    
    return ESP_OK;
}

esp_err_t memory_pool_check_guard_pattern(void *ptr)
{
    if (!ptr) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check guard pattern (simplified - would need size info)
    uint32_t *guard = (uint32_t*)((uint8_t*)ptr + 60); // Approximate location
    if (*guard != MEMORY_POOL_GUARD_PATTERN) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return ESP_OK;
}

// Allocation strategy implementations
bool memory_first_fit_find(memory_pool_t *pool, size_t size, uint32_t *block_index)
{
    for (uint32_t i = 0; i < pool->block_count; i++) {
        uint32_t bitmap_index = i / 32;
        uint32_t bit_index = i % 32;
        
        if (pool->free_bitmap[bitmap_index] & (1 << bit_index)) {
            *block_index = i;
            return true;
        }
    }
    return false;
}

bool memory_best_fit_find(memory_pool_t *pool, size_t size, uint32_t *block_index)
{
    // For fixed-size pools, best_fit is same as first_fit
    return memory_first_fit_find(pool, size, block_index);
}

bool memory_worst_fit_find(memory_pool_t *pool, size_t size, uint32_t *block_index)
{
    // For fixed-size pools, worst_fit is same as first_fit
    return memory_first_fit_find(pool, size, block_index);
}

esp_err_t memory_pool_self_test(void)
{
    ESP_LOGI(TAG, "Starting memory pool self-test...");
    
    esp_err_t result = ESP_OK;
    
    // Test allocation and deallocation
    void *ptr1 = memory_pool_alloc(32);
    if (!ptr1) {
        ESP_LOGE(TAG, "Failed to allocate 32 bytes");
        result = ESP_FAIL;
    }
    
    void *ptr2 = memory_pool_alloc(128);
    if (!ptr2) {
        ESP_LOGE(TAG, "Failed to allocate 128 bytes");
        result = ESP_FAIL;
    }
    
    void *ptr3 = memory_pool_alloc(512);
    if (!ptr3) {
        ESP_LOGE(TAG, "Failed to allocate 512 bytes");
        result = ESP_FAIL;
    }
    
    // Test deallocation
    if (ptr1) memory_pool_free(ptr1);
    if (ptr2) memory_pool_free(ptr2);
    if (ptr3) memory_pool_free(ptr3);
    
    // Test statistics
    size_t free_memory = memory_pool_get_free_memory();
    size_t total_memory = memory_pool_get_total_memory();
    
    if (free_memory != total_memory) {
        ESP_LOGE(TAG, "Memory leak detected: %d bytes not freed", total_memory - free_memory);
        result = ESP_FAIL;
    }
    
    // Test validation
    esp_err_t err = memory_pool_validate();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Memory validation failed");
        result = ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Memory pool self-test %s", result == ESP_OK ? "PASSED" : "FAILED");
    return result;
}
