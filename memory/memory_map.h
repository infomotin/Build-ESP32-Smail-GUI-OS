#pragma once

#include <stdint.h>

// ESP32 Memory Regions
#define ESP32_FLASH_BASE           0x00000000
#define ESP32_FLASH_SIZE           (4 * 1024 * 1024)  // 4MB
#define ESP32_IRAM_BASE           0x40020000
#define ESP32_IRAM_SIZE           (128 * 1024)        // 128KB
#define ESP32_DRAM_BASE           0x3FF80000
#define ESP32_DRAM_SIZE           (328 * 1024)        // 328KB
#define ESP32_RTC_FAST_BASE       0x40070000
#define ESP32_RTC_FAST_SIZE       (8 * 1024)          // 8KB
#define ESP32_RTC_SLOW_BASE       0x50000000
#define ESP32_RTC_SLOW_SIZE       (8 * 1024)          // 8KB

// Partition Table Layout
#define PARTITION_BOOTLOADER_ADDR  0x0000
#define PARTITION_BOOTLOADER_SIZE  0x2000  // 8KB
#define PARTITION_PHY_ADDR         0x6000  // 24KB
#define PARTITION_PHY_SIZE         0x1000  // 4KB
#define PARTITION_APP_ADDR         0x10000 // 64KB
#define PARTITION_APP_SIZE         0x3E0000 // 3.875MB
#define PARTITION_NVS_ADDR         0x3F0000 // 384KB
#define PARTITION_NVS_SIZE         0x6000  // 24KB
#define PARTITION_NVS_DATA_ADDR    0x3F6000 // 408KB
#define PARTITION_NVS_DATA_SIZE    0x2000  // 8KB

// Heap Configuration
#define HEAP_SIZE_MAIN            (200 * 1024)  // 200KB main heap
#define HEAP_SIZE_NETWORK         (50 * 1024)   // 50KB network heap
#define HEAP_SIZE_WEB            (30 * 1024)   // 30KB web server heap
#define HEAP_SIZE_TOTAL           (HEAP_SIZE_MAIN + HEAP_SIZE_NETWORK + HEAP_SIZE_WEB)

// Stack Sizes
#define STACK_SIZE_IDLE          4096
#define STACK_SIZE_MAIN          8192
#define STACK_SIZE_NETWORK       4096
#define STACK_SIZE_WEB           4096
#define STACK_SIZE_ISR           2048

// Task Queue Sizes
#define TASK_QUEUE_SIZE          32
#define EVENT_QUEUE_SIZE         64
#define HTTP_QUEUE_SIZE          16
#define GPIO_QUEUE_SIZE         32

// Memory alignment
#define MEMORY_ALIGN            4
#define CACHE_LINE_SIZE         32

// Memory regions for MPU
#define MPU_REGION_FLASH         0
#define MPU_REGION_IRAM         1
#define MPU_REGION_DRAM         2
#define MPU_REGION_PERIPH       3

// Memory protection flags
#define MPU_REGION_READ_ONLY    0x01
#define MPU_REGION_READ_WRITE   0x02
#define MPU_REGION_EXECUTE      0x04
#define MPU_REGION_NO_EXECUTE   0x08

// Heap allocation strategies
#define HEAP_STRATEGY_BEST_FIT   0
#define HEAP_STRATEGY_FIRST_FIT  1
#define HEAP_STRATEGY_WORST_FIT  2

// Memory monitoring
#define MEMORY_MONITOR_ENABLED    1
#define MEMORY_STATS_INTERVAL    5000  // 5 seconds

// Critical memory thresholds
#define CRITICAL_HEAP_THRESHOLD  (10 * 1024)  // 10KB
#define WARNING_HEAP_THRESHOLD   (50 * 1024)   // 50KB

// Memory pool configurations
#define POOL_BLOCK_SIZE_SMALL    64
#define POOL_BLOCK_SIZE_MEDIUM   256
#define POOL_BLOCK_SIZE_LARGE    1024
#define POOL_BLOCKS_SMALL        32
#define POOL_BLOCKS_MEDIUM       16
#define POOL_BLOCKS_LARGE        8

// DMA buffer configuration
#define DMA_BUFFER_SIZE         4096
#define DMA_BUFFER_COUNT        4

// Stack overflow protection
#define STACK_CANARY            0xDEADBEEF
#define STACK_GUARD_SIZE        32

// Memory layout verification
#define MEMORY_LAYOUT_MAGIC      0x12345678
#define MEMORY_LAYOUT_VERSION    1
