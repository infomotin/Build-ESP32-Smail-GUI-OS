#!/bin/bash

# ESP32 Small OS Build and Flash Script
# Builds and flashes the ESP32 Small OS to an ESP32 board

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
PORT="/dev/ttyUSB0"
BAUD="921600"
TARGET="esp32"
BUILD_DIR="build"
CLEAN_BUILD=false
FLASH_ONLY=false
MONITOR_ONLY=false

# Help function
show_help() {
    echo "ESP32 Small OS Build and Flash Script"
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -p, --port PORT      Serial port (default: /dev/ttyUSB0)"
    echo "  -b, --baud BAUD      Baud rate (default: 921600)"
    echo "  -t, --target TARGET  ESP32 target (esp32, esp32s2, esp32s3, esp32c3)"
    echo "  -c, --clean          Clean build before compiling"
    echo "  -f, --flash-only     Flash only (skip build)"
    echo "  -m, --monitor-only   Monitor only (skip build and flash)"
    echo "  -h, --help           Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                    # Build and flash with default settings"
    echo "  $0 -p /dev/ttyUSB1    # Use different serial port"
    echo "  $0 -c                 # Clean build and flash"
    echo "  $0 -f                 # Flash only (skip build)"
    echo "  $0 -m                 # Monitor only"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        -b|--baud)
            BAUD="$2"
            shift 2
            ;;
        -t|--target)
            TARGET="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -f|--flash-only)
            FLASH_ONLY=true
            shift
            ;;
        -m|--monitor-only)
            MONITOR_ONLY=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Check if ESP-IDF environment is set up
check_idf() {
    if [ -z "$IDF_PATH" ]; then
        echo -e "${RED}Error: ESP-IDF environment not set up${NC}"
        echo "Please run:"
        echo "  source /opt/esp/idf/export.sh"
        echo "or your ESP-IDF export script"
        exit 1
    fi
    
    if [ ! -d "$IDF_PATH" ]; then
        echo -e "${RED}Error: IDF_PATH not found: $IDF_PATH${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}ESP-IDF found at: $IDF_PATH${NC}"
}

# Check if serial port exists
check_port() {
    if [ ! -e "$PORT" ]; then
        echo -e "${RED}Error: Serial port not found: $PORT${NC}"
        echo "Available serial ports:"
        ls -la /dev/tty* | grep -E "(ttyUSB|ttyACM|ttyS)"
        exit 1
    fi
    
    echo -e "${GREEN}Using serial port: $PORT${NC}"
}

# Clean build directory
clean_build() {
    if [ "$CLEAN_BUILD" = true ]; then
        echo -e "${YELLOW}Cleaning build directory...${NC}"
        rm -rf "$BUILD_DIR"
    fi
}

# Build the project
build_project() {
    if [ "$FLASH_ONLY" = true ] || [ "$MONITOR_ONLY" = true ]; then
        echo -e "${YELLOW}Skipping build...${NC}"
        return
    fi
    
    echo -e "${BLUE}Building ESP32 Small OS...${NC}"
    
    # Set target
    idf.py set-target "$TARGET"
    
    # Build
    idf.py build
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Build successful!${NC}"
    else
        echo -e "${RED}Build failed!${NC}"
        exit 1
    fi
}

# Flash the firmware
flash_firmware() {
    if [ "$MONITOR_ONLY" = true ]; then
        echo -e "${YELLOW}Skipping flash...${NC}"
        return
    fi
    
    echo -e "${BLUE}Flashing firmware to ESP32...${NC}"
    echo -e "${YELLOW}Port: $PORT, Baud: $BAUD${NC}"
    
    # Flash
    idf.py -p "$PORT" -b "$BAUD" flash
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Flash successful!${NC}"
    else
        echo -e "${RED}Flash failed!${NC}"
        exit 1
    fi
}

# Monitor serial output
monitor_serial() {
    echo -e "${BLUE}Starting serial monitor...${NC}"
    echo -e "${YELLOW}Press Ctrl+C to exit monitor${NC}"
    
    idf.py -p "$PORT" monitor
}

# Main execution
main() {
    echo -e "${BLUE}ESP32 Small OS Build and Flash Script${NC}"
    echo "=========================================="
    
    # Check environment
    check_idf
    
    # Check port (unless monitor-only)
    if [ "$MONITOR_ONLY" = false ]; then
        check_port
    fi
    
    # Clean build if requested
    clean_build
    
    # Build project
    build_project
    
    # Flash firmware
    flash_firmware
    
    # Start monitor
    monitor_serial
}

# Run main function
main "$@"
