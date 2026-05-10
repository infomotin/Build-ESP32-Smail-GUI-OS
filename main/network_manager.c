#include "network_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "network_manager";

void network_manager_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "SmallOS",
            .ssid_len = 0,
            .password = "smallos123",
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    ESP_LOGI(TAG, "WiFi initialized");
}
