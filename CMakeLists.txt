cmake_minimum_required(VERSION 3.20)

# ESP-IDF extensions
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project definition
project(esp32_small_os)

# Option to build Desktop Simulator
option(BUILD_DESKTOP_SIMULATOR "Build the ESP32 Virtual Hardware Desktop Simulator" OFF)
option(BUILD_DESKTOP_SIMULATOR_QT6 "Use Qt6 for desktop simulator (requires Qt6)" ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2 -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -ffunction-sections -fdata-sections")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# Linker script
set(LD_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/esp32_small_os.ld")

# Memory configuration
add_definitions(-DHEAP_SIZE_MAIN=200*1024)
add_definitions(-DHEAP_SIZE_NETWORK=50*1024)
add_definitions(-DHEAP_SIZE_WEB=30*1024)
add_definitions(-DSTACK_SIZE_MAIN=8192)
add_definitions(-DSTACK_SIZE_NETWORK=4096)
add_definitions(-DSTACK_SIZE_WEB=4096)

# Debug configuration
add_definitions(-DDEBUG=1)
add_definitions(-DLOG_LOCAL_LEVEL=ESP_LOG_INFO)

# Component registration
idf_component_register(
    SRCS
        "main/main.c"
        "bootloader/bootloader.c"
        "scheduler/task_scheduler.c"
        "gpio/gpio_hal.c"
        "interrupt/interrupt_handler.c"
        "http/http_server.c"
        "memory/memory_pool.c"
        "network/wifi_manager.c"
        "storage/nvs_storage.c"
        "utils/logger.c"
        "security/auth.c"
    INCLUDE_DIRS
        "."
        "main"
        "bootloader"
        "scheduler"
        "gpio"
        "interrupt"
        "http"
        "memory"
        "network"
        "storage"
        "utils"
        "security"
    REQUIRES
        esp_wifi
        esp_netif
        nvs_flash
        freertos
        lwip
        cJSON
        mbedtls
        esp_timer
        esp_common
        driver
        esp_system
        esp_event
        esp_http_server
         esp_http_client
     )

# Optional Desktop Simulator
if(BUILD_DESKTOP_SIMULATOR)
    message(STATUS "Desktop simulator build enabled")
    add_subdirectory(DesktopApp)
endif()
