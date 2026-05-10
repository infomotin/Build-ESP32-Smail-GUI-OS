#pragma once

#include <stdint.h>
#include <stdbool.h>

// Bootloader magic number
#define BOOTLOADER_MAGIC 0x5A5AA5A5

// Bootloader version
#define BOOTLOADER_VERSION 1

// Application entry points
#define APP_ENTRY_POINT 0x10000

// Bootloader states
typedef enum {
    BOOT_STATE_INIT = 0,
    BOOT_STATE_VERIFY,
    BOOT_STATE_JUMP,
    BOOT_STATE_ERROR
} boot_state_t;

// Bootloader configuration
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t entry_point;
    uint32_t app_size;
    uint32_t crc32;
} bootloader_header_t;

// Bootloader functions
void bootloader_main(void);
uint32_t calculate_crc32(const uint8_t *data, size_t length);
bool verify_application(const bootloader_header_t *header);
