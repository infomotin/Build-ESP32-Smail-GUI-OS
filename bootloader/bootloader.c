#include "esp_log.h"
#include "esp_system.h"
#include "soc/cpu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#define TAG "BOOTLOADER"

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t entry_point;
    uint32_t app_size;
    uint32_t crc32;
} bootloader_header_t;

// Bootloader entry point
void bootloader_main(void)
{
    ESP_LOGI(TAG, "ESP32 Small OS Bootloader v1.0");
    
    // 1. Hardware initialization
    esp_cpu_configure_region_protection();
    esp_rom_uart_tx_wait_idle(0);
    
    // 2. Clock configuration
    esp_clk_init();
    ESP_LOGI(TAG, "Clock system initialized");
    
    // 3. Memory initialization
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash due to new version");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "NVS flash initialized");
    
    // 4. Application verification
    bootloader_header_t *app_header = (bootloader_header_t*)0x10000;
    if (app_header->magic != 0x5A5AA5A5) {
        ESP_LOGE(TAG, "Invalid application magic: 0x%08X", app_header->magic);
        esp_restart();
    }
    
    ESP_LOGI(TAG, "Application verified: v%d, size: %d bytes", 
              app_header->version, app_header->app_size);
    
    // 5. Jump to application
    ESP_LOGI(TAG, "Jumping to application at 0x%08X", app_header->entry_point);
    
    // Disable interrupts before jump
    taskDISABLE_INTERRUPTS();
    
    typedef void (*app_entry_t)(void);
    app_entry_t app_entry = (app_entry_t)app_header->entry_point;
    app_entry();
}

// CRC32 calculation for verification
static uint32_t calculate_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}
