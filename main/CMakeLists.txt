idf_component_register(SRCS "main.c" "scheduler.c" "gpio_manager.c" "network_manager.c" "storage.c" "auth.c" "activity_manager.c" "web_server.c" "web_view_manager.c" "realtime_monitor.c"
                    INCLUDE_DIRS "."
                    REQUIRES cJSON mbedtls)

spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)
