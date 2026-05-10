# ESP32 Small OS Deployment Guide

This guide provides complete instructions for building and flashing the ESP32 Small OS to actual ESP32 hardware boards.

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Hardware Requirements](#hardware-requirements)
3. [Software Setup](#software-setup)
4. [Building the Firmware](#building-the-firmware)
5. [Flashing to ESP32](#flashing-to-esp32)
6. [Configuration](#configuration)
7. [Troubleshooting](#troubleshooting)
8. [Advanced Options](#advanced-options)

## Prerequisites

### Hardware Requirements
- **ESP32 Development Board** (any ESP32 variant supported)
- **USB Cable** for programming and power
- **Computer** with USB port
- **Optional**: External LEDs, buttons, sensors for testing

### Software Requirements
- **ESP-IDF** v4.4 or later
- **Python** 3.7 or later
- **Git** for version control
- **Serial terminal** (built into ESP-IDF)

## Software Setup

### 1. Install ESP-IDF

#### Linux/macOS
```bash
# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git

# Navigate to ESP-IDF directory
cd esp-idf

# Install dependencies
./install.sh esp32

# Set up environment
source ./export.sh
```

#### Windows
```cmd
# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git

# Navigate to ESP-IDF directory
cd esp-idf

# Run install script
install.bat esp32

# Set up environment
export.bat
```

### 2. Verify Installation
```bash
# Check ESP-IDF version
idf.py --version

# Check connected boards
idf.py -p /dev/ttyUSB0 monitor
```

## Building the Firmware

### Quick Build
```bash
# Navigate to project directory
cd "d:/laragon/www/Build ESP32 Smail GUI OS"

# Build the project
idf.py build
```

### Clean Build
```bash
# Clean and build
idf.py fullclean
idf.py build
```

### Specific Target
```bash
# Set target (esp32, esp32s2, esp32s3, esp32c3)
idf.py set-target esp32

# Build
idf.py build
```

## Flashing to ESP32

### Method 1: Using Build Scripts

#### Linux/macOS
```bash
# Make script executable
chmod +x build_esp32.sh

# Build and flash with defaults
./build_esp32.sh

# Custom port and baud rate
./build_esp32.sh -p /dev/ttyUSB1 -b 460800

# Clean build and flash
./build_esp32.sh -c

# Flash only (skip build)
./build_esp32.sh -f

# Monitor only
./build_esp32.sh -m
```

#### Windows
```cmd
# Build and flash with defaults
build_esp32.bat

# Custom port
build_esp32.bat -p COM4

# Clean build and flash
build_esp32.bat -c
```

### Method 2: Manual ESP-IDF Commands

```bash
# Set target
idf.py set-target esp32

# Build
idf.py build

# Flash (replace with your port)
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor

# Build, flash, and monitor in one command
idf.py -p /dev/ttyUSB0 build flash monitor
```

## Configuration

### 1. WiFi Configuration
The ESP32 Small OS will try to connect to WiFi using credentials stored in NVS. To configure WiFi:

#### Method A: Web Interface (after first boot)
1. Connect to the ESP32's access point (if created)
2. Open web browser to 192.168.4.1
3. Configure WiFi credentials

#### Method B: Pre-configure in code
Edit `main/main.c` and modify the WiFi credentials:
```c
// In app_main() function, add:
wifi_manager_connect("YourWiFiSSID", "YourWiFiPassword");
```

#### Method C: NVS Configuration
Use the NVS browser tool to pre-configure WiFi credentials.

### 2. GPIO Configuration
Default GPIO pin assignments:
- **GPIO 0**: Button input (with pull-up)
- **GPIO 2**: Built-in LED output
- **GPIO 4**: PWM output (1kHz, 50% duty)
- **GPIO 36**: ADC input (sensor reading)

To modify GPIO configuration, edit the `gpio_configs` array in `main/main.c`.

### 3. HTTP Server Configuration
Default settings:
- **Port**: 80
- **Max clients**: 4
- **Request timeout**: 5000ms
- **Enable compression**: true

Access the web interface at: `http://[ESP32_IP_ADDRESS]`

## Hardware Connections

### Basic Setup
```
ESP32 Board
├── USB Cable → Computer (for power and programming)
├── GPIO 0 → Button (to GND)
├── GPIO 2 → Built-in LED (usually on-board)
├── GPIO 4 → LED + Resistor (220Ω) → GND
└── GPIO 36 → Potentiometer or sensor
```

### Advanced Setup
```
ESP32 Board
├── Power: 3.3V and GND
├── Programming: TX, RX, DTR, RTS (via USB)
├── GPIO 0: Boot button (also used for user input)
├── GPIO 2: Status LED
├── GPIO 4: PWM output (LED dimming, motor control)
├── GPIO 36: ADC input (sensors, potentiometers)
├── Additional GPIOs: As needed for expansion
```

## Troubleshooting

### Common Issues

#### 1. Build Errors
```bash
# Clean build
idf.py fullclean
idf.py build

# Check ESP-IDF version
idf.py --version

# Update ESP-IDF
cd $IDF_PATH
git pull
git submodule update --init --recursive
./install.sh esp32
```

#### 2. Flash Errors
```bash
# Check serial port
ls -la /dev/tty* | grep -E "(ttyUSB|ttyACM)"

# Try different baud rates
idf.py -p /dev/ttyUSB0 -b 115200 flash

# Put ESP32 in bootloader mode
# Hold BOOT button, press RESET, release BOOT
```

#### 3. Runtime Issues
```bash
# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor

# Check heap size
# Look for "Free heap" in serial output

# Reset to factory defaults
idf.py -p /dev/ttyUSB0 erase-flash
idf.py -p /dev/ttyUSB0 flash
```

#### 4. WiFi Connection Issues
- Check SSID and password
- Verify 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check signal strength
- Try moving closer to router

#### 5. Web Interface Not Accessible
- Check ESP32 IP address in serial monitor
- Verify WiFi connection
- Check firewall settings
- Try different browser

### Debug Mode
Enable debug logging by editing `sdkconfig.defaults`:
```
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
CONFIG_LOG_MAXIMUM_LEVEL=5
```

Or use menuconfig:
```bash
idf.py menuconfig
# Component config → Log output → Default log level → Debug
```

## Advanced Options

### 1. Custom Board Configuration
Create custom `sdkconfig` for your specific board:
```bash
idf.py menuconfig
# Configure:
# - Serial flasher config
# - Partition table
# - Component config
```

### 2. OTA Updates
Enable OTA updates:
```bash
idf.py menuconfig
# Partition Table → Single factory app → Factory app, two OTA definitions
```

### 3. Performance Optimization
Optimize for performance:
```bash
idf.py menuconfig
# Component config → ESP32 Specific → CPU frequency → 240MHz
# Component config → FreeRTOS → Tick rate (Hz) → 1000
```

### 4. Power Management
Enable power saving:
```bash
idf.py menuconfig
# Component config → ESP32 Specific → Support for low power modes
```

## Production Deployment

### 1. Production Build
```bash
# Optimized build
idf.py build

# Check binary size
ls -la build/esp32_small_os.bin
```

### 2. Batch Flashing
Create a batch flashing script:
```bash
#!/bin/bash
ESP32_PORTS=("/dev/ttyUSB0" "/dev/ttyUSB1" "/dev/ttyUSB2")

for port in "${ESP32_PORTS[@]}"; do
    echo "Flashing $port..."
    idf.py -p "$port" flash
    sleep 2
done
```

### 3. Factory Reset
Implement factory reset functionality:
```c
// Add to main.c
void factory_reset_command() {
    nvs_flash_erase();
    esp_restart();
}
```

## Security Considerations

### 1. Change Default Passwords
- Update admin credentials
- Change WiFi passwords
- Implement API key authentication

### 2. Enable Flash Encryption
```bash
idf.py menuconfig
# Security features → Enable flash encryption
```

### 3. Secure Boot
```bash
idf.py menuconfig
# Security features → Enable secure boot
```

## Performance Monitoring

### 1. System Statistics
Monitor via web interface:
```
http://[ESP32_IP]/api/system/status
```

### 2. Serial Monitoring
Key metrics to watch:
- Free heap size
- CPU usage
- WiFi signal strength
- Task stack usage

### 3. Performance Tuning
Adjust as needed:
- Task priorities
- Stack sizes
- Buffer sizes
- Timer intervals

## Support

### 1. Documentation
- Check `IMPLEMENTATION_GUIDE_COMPLETE.md`
- Review component headers for API details
- Monitor serial output for error messages

### 2. Community Support
- ESP-IDF documentation: https://docs.espressif.com
- ESP32 forums: https://www.esp32.com
- GitHub issues for this project

### 3. Debug Tools
- ESP-IDF debugger integration
- Logic analyzers for GPIO debugging
- Network sniffers for HTTP debugging

---

## Quick Start Summary

1. **Setup ESP-IDF**: Follow software setup instructions
2. **Connect Hardware**: Connect ESP32 via USB
3. **Build Firmware**: `idf.py build`
4. **Flash Board**: `idf.py flash monitor`
5. **Access Web Interface**: Open browser to ESP32 IP
6. **Configure WiFi**: Use web interface or modify code
7. **Test GPIO**: Use web controls to test functionality

The ESP32 Small OS is now ready for deployment on actual hardware!
