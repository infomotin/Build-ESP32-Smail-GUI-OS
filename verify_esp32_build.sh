#!/bin/bash

# ESP32 Small OS Build Verification Script
# Verifies that all components are ready for ESP32 deployment

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}ESP32 Small OS Build Verification${NC}"
echo "===================================="

# Check ESP-IDF environment
echo -e "\n${YELLOW}1. Checking ESP-IDF environment...${NC}"
if [ -z "$IDF_PATH" ]; then
    echo -e "${RED}✗ ESP-IDF not found. Please source export.sh${NC}"
    exit 1
fi
if [ ! -d "$IDF_PATH" ]; then
    echo -e "${RED}✗ IDF_PATH invalid: $IDF_PATH${NC}"
    exit 1
fi
echo -e "${GREEN}✓ ESP-IDF found at: $IDF_PATH${NC}"

# Check required files
echo -e "\n${YELLOW}2. Checking required files...${NC}"
required_files=(
    "CMakeLists.txt"
    "main/main.c"
    "sdkconfig.defaults"
    "partitions.csv"
    "scheduler/task_scheduler.h"
    "scheduler/task_scheduler.c"
    "gpio/gpio_hal.h"
    "gpio/gpio_hal.c"
    "http/http_server.h"
    "http/http_server.c"
    "memory/memory_pool.h"
    "memory/memory_pool.c"
    "network/wifi_manager.h"
    "network/wifi_manager.c"
    "storage/nvs_storage.h"
    "storage/nvs_storage.c"
    "utils/logger.h"
    "utils/logger.c"
    "security/auth.h"
    "security/auth.c"
)

missing_files=()
for file in "${required_files[@]}"; do
    if [ ! -f "$file" ]; then
        missing_files+=("$file")
    fi
done

if [ ${#missing_files[@]} -eq 0 ]; then
    echo -e "${GREEN}✓ All required files present${NC}"
else
    echo -e "${RED}✗ Missing files:${NC}"
    for file in "${missing_files[@]}"; do
        echo -e "  ${RED}- $file${NC}"
    done
    exit 1
fi

# Check CMakeLists.txt configuration
echo -e "\n${YELLOW}3. Checking CMakeLists.txt configuration...${NC}"
if grep -q "idf_component_register" CMakeLists.txt; then
    echo -e "${GREEN}✓ Component registration found${NC}"
else
    echo -e "${RED}✗ Component registration missing${NC}"
    exit 1
fi

if grep -q "security/auth.c" CMakeLists.txt; then
    echo -e "${GREEN}✓ Security component included${NC}"
else
    echo -e "${RED}✗ Security component missing${NC}"
    exit 1
fi

# Check sdkconfig.defaults
echo -e "\n${YELLOW}4. Checking sdkconfig.defaults...${NC}"
if [ -f "sdkconfig.defaults" ]; then
    if grep -q "CONFIG_ESP32_WIFI_ENABLED=y" sdkconfig.defaults; then
        echo -e "${GREEN}✓ WiFi configuration present${NC}"
    else
        echo -e "${RED}✗ WiFi configuration missing${NC}"
        exit 1
    fi
    
    if grep -q "CONFIG_FREERTOS_HZ=1000" sdkconfig.defaults; then
        echo -e "${GREEN}✓ FreeRTOS configuration present${NC}"
    else
        echo -e "${RED}✗ FreeRTOS configuration missing${NC}"
        exit 1
    fi
else
    echo -e "${RED}✗ sdkconfig.defaults not found${NC}"
    exit 1
fi

# Check partitions.csv
echo -e "\n${YELLOW}5. Checking partitions.csv...${NC}"
if [ -f "partitions.csv" ]; then
    if grep -q "factory" partitions.csv; then
        echo -e "${GREEN}✓ Factory partition found${NC}"
    else
        echo -e "${RED}✗ Factory partition missing${NC}"
        exit 1
    fi
    
    if grep -q "nvs" partitions.csv; then
        echo -e "${GREEN}✓ NVS partition found${NC}"
    else
        echo -e "${RED}✗ NVS partition missing${NC}"
        exit 1
    fi
else
    echo -e "${RED}✗ partitions.csv not found${NC}"
    exit 1
fi

# Check build scripts
echo -e "\n${YELLOW}6. Checking build scripts...${NC}"
if [ -f "build_esp32.sh" ]; then
    echo -e "${GREEN}✓ Linux build script found${NC}"
    chmod +x build_esp32.sh
else
    echo -e "${YELLOW}⚠ Linux build script not found${NC}"
fi

if [ -f "build_esp32.bat" ]; then
    echo -e "${GREEN}✓ Windows build script found${NC}"
else
    echo -e "${YELLOW}⚠ Windows build script not found${NC}"
fi

# Try dry run build
echo -e "\n${YELLOW}7. Testing build configuration...${NC}"
idf.py set-target esp32 > /dev/null 2>&1 || {
    echo -e "${RED}✗ Failed to set ESP32 target${NC}"
    exit 1
}
echo -e "${GREEN}✓ ESP32 target set successfully${NC}"

# Check component dependencies
echo -e "\n${YELLOW}8. Checking component dependencies...${NC}"
required_components=(
    "esp_wifi"
    "esp_netif"
    "nvs_flash"
    "freertos"
    "lwip"
    "cJSON"
    "mbedtls"
    "esp_timer"
    "esp_event"
    "esp_http_server"
)

for component in "${required_components[@]}"; do
    if grep -q "$component" CMakeLists.txt; then
        echo -e "${GREEN}✓ $component dependency found${NC}"
    else
        echo -e "${YELLOW}⚠ $component dependency missing${NC}"
    fi
done

# Check for common build issues
echo -e "\n${YELLOW}9. Checking for common issues...${NC}"

# Check for simulation defines
if grep -r "DSIMULATION" . --exclude-dir=sim --exclude="*.md"; then
    echo -e "${RED}✗ Simulation defines found in production code${NC}"
    echo -e "  Remove DSIMULATION defines from production files${NC}"
    exit 1
else
    echo -e "${GREEN}✓ No simulation defines in production code${NC}"
fi

# Check for recursive malloc calls
if grep -r "malloc.*malloc" . --exclude-dir=sim --exclude="*.md"; then
    echo -e "${RED}✗ Recursive malloc calls found${NC}"
    exit 1
else
    echo -e "${GREEN}✓ No recursive malloc calls${NC}"
fi

# Check for missing includes
if grep -r "#include.*sim_" . --exclude-dir=sim --exclude="*.md"; then
    echo -e "${RED}✗ Simulation includes found in production code${NC}"
    exit 1
else
    echo -e "${GREEN}✓ No simulation includes in production code${NC}"
fi

echo -e "\n${GREEN}====================================${NC}"
echo -e "${GREEN}✓ All checks passed!${NC}"
echo -e "${GREEN}ESP32 Small OS is ready for deployment!${NC}"
echo -e "\n${BLUE}Next steps:${NC}"
echo "1. Connect ESP32 board via USB"
echo "2. Run: ./build_esp32.sh (Linux) or build_esp32.bat (Windows)"
echo "3. Access web interface at http://[ESP32_IP]"
echo "4. Configure WiFi and test GPIO controls"

echo -e "\n${BLUE}Build commands:${NC}"
echo "  idf.py build              # Build firmware"
echo "  idf.py flash              # Flash to ESP32"
echo "  idf.py monitor            # Serial monitor"
echo "  idf.py flash monitor      # Flash and monitor"

echo -e "\n${BLUE}For troubleshooting, see: ESP32_DEPLOYMENT_GUIDE.md${NC}"
