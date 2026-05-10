# ESP32 Virtual Hardware Simulator

A comprehensive desktop-based simulation environment for ESP32 firmware development. This simulator enables you to compile, load, and execute ESP32 firmware (ELF binaries or Arduino sketches) without physical hardware.

## Features

### Core Simulation
- **xtensa ISS** - Full instruction-accurate simulation of ESP32's dual-core Xtensa LX6 CPUs
- **Memory Model** - Accurate ESP32 memory map (IRAM, DRAM, ROM, MMIO)
- **ELF Loader** - Load standard ESP-IDF compiled binaries
- **Event Scheduler** - Precise timing and interrupt handling

### Peripheral Simulation
- **GPIO** - All 34 GPIO pins with input/output modes, pull-ups, interrupts
- **PWM (LEDC)** - 16 channels, up to 40 MHz, 1-20 bit resolution
- **ADC** - 12-bit, 4 attenuation levels, with noise injection
- **DAC** - 8-bit on GPIO25/26
- **I2C** - Two controllers with master/slave, clock stretching, ACK/NACK
- **SPI** - Three controllers (SPI0, SPI1/HSPI, SPI2/VSPI) with full-duplex support
- **UART** - Three UARTs with configurable baud, parity, flow control

### Wireless Simulation
- **Wi-Fi** - Virtual AP with association, DHCP, RSSI config, packet loss simulation
- **BLE** - GATT server simulation with advertising, connections, notifications

### Debugging Tools
- **Software breakpoints** with hit counting
- **Memory watchpoints** with value change detection
- **Register viewer** - All 64 Xtensa registers
- **Memory viewer** - Hex/ASCII display
- **Call stack** - Automatic unwinding with symbol resolution
- **GDB server** - Remote debugging support

### GUI Features
- **Pinout Panel** - Interactive ESP32-WROOM-32 diagram with color-coded pins
- **Workspace** - Drag-and-drop virtual components grid canvas
- **Virtual Components:**
  - Single-color LED
  - RGB LED
  - 16x2 Character LCD (HD44780)
  - 128x64 OLED (SSD1306 I2C)
  - Push buttons, toggle switches
  - NTC thermistor, LDR light sensor
  - HC-SR04 ultrasonic distance sensor
  - DHT22 temperature/humidity sensor

### Signal Analysis
- **Logic Analyzer** - Up to 34 channels, 10 MS/s, trigger support, VCD export
- **Oscilloscope** - 4 channels, timebase 1 µs-1 s/div, auto measurements (freq, duty, min/max, RMS)

## Requirements

### Dependencies

#### Required
- Qt6 (Components: Core, Widgets, Charts, OpenGLWidgets)
- CMake 3.20 or later
- C++17 compatible compiler
  - Windows: MSVC 2022, MinGW-w64
  - Linux: GCC 11+, Clang 14+
  - macOS: Xcode 14+ (Apple Clang)

#### Optional
- ESP-IDF toolchain (for Arduino sketch compilation, future feature)

### Qt Installation
```bash
# Linux (Ubuntu/Debian)
sudo apt-get install qt6-base-dev qt6-charts-dev

# macOS with Homebrew
brew install qt6

# Windows
# Download from https://www.qt.io/download-qt-installer
# Or use vcpkg: vcpkg install qt6-base qt6-charts
```

## Building

### From Root Project

```bash
cd D:\laragon\www\Build ESP32 Smail GUI OS

# Configure with desktop build enabled
cmake -S . -B build -DBUILD_DESKTOP_SIMULATOR=ON

# Build
cmake --build build --target esp32_virtual_simulator --config Release
```

### Standalone Build

```bash
cd DesktopApp

# Configure
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.9.0/msvc2022_64"

# Build
cmake --build build --config Release

# Run
cd build/Release
esp32_virtual_simulator.exe
```

### Windows Build Script

```bash
build_desktop.bat
```

This will:
1. Configure CMake with Qt6
2. Build the application
3. Place the executable in `DesktopApp/cmake-build-debug/`

## Usage

### Loading Firmware
1. Start the simulator
2. File → Load Firmware
3. Select a compiled `.elf` file from ESP-IDF build output
4. Firmware symbol table will be loaded for debugging

### Placing Components
1. Drag components from the Component Library (left panel) onto the Workspace
2. Right-click components to configure properties
3. Click on a component's connection point, then click a GPIO pin on the Pinout panel

### Running Simulation
- **Start** (F5) - Begin execution at full speed
- **Pause** (F6) - Pause simulation
- **Stop** (F7) - Stop and reset
- **Step** (F8) - Execute one instruction
- **Reset** (F9) - Reset all peripherals and CPU

### Debugging
- Set breakpoints by double-clicking in the Breakpoints panel
- View registers and memory in the Debug dock
- Watch variables with the watchpoint manager
- Connect external GDB: `target remote :3333`

### Signal Analysis
- Enable channels in Logic Analyzer
- Configure trigger conditions
- Capture waveforms and export to VCD for GTKWave
- Use Oscilloscope to view analog signals (PWM, ADC)

### Wi-Fi Simulation
1. Go to Simulation → Wi-Fi Settings
2. Configure virtual AP SSID/password
3. Firmware connects using `esp_wifi_connect()`
4. Adjust RSSI and packet loss for testing

## Architecture

```
DesktopApp/
├── src/
│   ├── core/              # Core engine & application
│   │   ├── main.cpp
│   │   ├── application.cpp
│   │   └── simulation_engine.cpp
│   ├── gui/               # Qt widgets
│   │   ├── main_window.cpp
│   │   ├── pinout_panel.cpp
│   │   ├── workspace.cpp
│   │   ├── console_panel.cpp
│   │   ├── logic_analyzer.cpp
│   │   ├── oscilloscope.cpp
│   │   └── debug_panel.cpp
│   ├── peripherals/       # Peripheral models
│   │   ├── gpio_controller.cpp
│   │   ├── i2c_controller.cpp
│   │   ├── spi_controller.cpp
│   │   ├── uart_controller.cpp
│   │   ├── adc_controller.cpp
│   │   ├── dac_controller.cpp
│   │   └── ledc_controller.cpp
│   ├── virtual_components/ # GUI components
│   │   ├── led_component.cpp
│   │   ├── button_component.cpp
│   │   └── ...
│   ├── debug/             # Debugging system
│   │   └── debug_controller.cpp
│   └── network/           # Wireless simulation
│       ├── wifi_simulator.cpp
│       └── ble_simulator.cpp
└── include/              # All headers
```

## Integration with Existing Project

The DesktopApp extends the existing ESP32 Small OS project:

1. Shares simulation core (`simulator/core/`) with ISS, memory model, scheduler
2. Conditionally compiles HAL for simulation vs hardware
3. New HAL stub redirects `gpio_hal_*` calls to `GPIOController`
4. Logs redirected from `ESP_LOG*` to ConsolePanel

To build the firmware with simulation support:
```cmake
add_compile_definitions(SIMULATION=1)
target_link_libraries(your_target simulator)
```

## Testing

Unit tests are in `tests/` directory. Currently implemented:

- **ELF Loader tests**: validates magic, architecture, segment loading
- **Memory Model tests**: MMIO, alignment, exceptions
- **GPIO tests**: Mode configuration, level changes, Schmitt trigger
- **Peripheral tests**: Basic I2C/SPI transactions

To run tests:
```bash
cd DesktopApp/build
ctest -C Debug
```

## Performance

Target performance on modern hardware (4-core, 3 GHz):
- **0.5x real-time** for typical IoT firmware (Wi-Fi + sensors)
- **0.1x real-time** with Logic Analyzer capturing all 34 channels at 10 MS/s
- **60 FPS** GUI updates under normal load

### Optimization Tips
- Reduce logic analyzer sampling rate
- Disable unused channels
- Limit waveform history depth
- Use Release build for performance testing

## Known Limitations

- Classic Bluetooth (BR/EDR) not supported (BLE only)
- Full ESP-IDF complex flash mapping partially implemented
- Some advanced peripherals (RMT, PCNT, TWAI) pending implementation
- Arduino sketch compilation integration is planned, not yet functional

## Future Work

- [ ] Complete peripheral set (RMT, TWAI, USB-Serial)
- [ ] Arduino IDE integration
- [ ] Python scripting API
- [ ] Cloud simulation sharing
- [ ] Virtual JTAG debugging
- [ ] Power consumption modeling

## Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Add tests for new code
4. Follow existing coding style (4-space indent, camelCase functions, Qt naming)
5. Submit a pull request

## License

This project is licensed under MIT License - see LICENSE file for details.

## Acknowledgments

- ESP-IDF team for the excellent hardware abstraction layer
- Qt Project for the cross-platform GUI framework
- Xtensa documentation for ISA reference

---

**Note:** This simulator is intended for development and testing, not as a replacement for real hardware verification. Always test on actual ESP32 before production deployment.
