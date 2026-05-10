@echo off
echo ESP32 Small OS Simulation Runner
echo ================================
echo.

REM Check if build directory exists
if not exist build mkdir build

echo Building simulation...
gcc -DSIMULATION -Wall -Wextra -std=c99 -O2 -I. -I.. -I../main -I../scheduler -I../gpio -I../interrupt -I../http -I../memory -I../network -I../storage -I../utils -I../security -o build/esp32_small_os_sim sim_runner.c sim_main.c ../scheduler/task_scheduler.c ../gpio/gpio_hal.c ../interrupt/interrupt_handler.c ../http/http_server.c ../memory/memory_pool.c ../network/wifi_manager.c ../storage/nvs_storage.c ../utils/logger.c ../security/auth.c -lpthread -lm

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful!
echo.
echo Running ESP32 Small OS Simulation...
echo Duration: 30 seconds
echo Press Ctrl+C to stop early
echo.
build\esp32_small_os_sim

echo.
echo Simulation completed!
pause
