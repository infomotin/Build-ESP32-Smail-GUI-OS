# ESP32 Virtual Hardware Simulation Environment

A high-fidelity, desktop-based simulator that enables developers to compile, load, and execute ESP32 firmware (ELF binaries or Arduino sketches) without physical hardware.

## Architecture Overview

The simulator consists of several key components:

### Core Emulation Engine
- **ISS (Instruction Set Simulator)**: Executes Xtensa LX6 instructions
- **Memory Model**: Simulates ESP32 memory map (IRAM, DRAM, ROM, I/O)
- **Event Scheduler**: Manages timing, interrupts, and synchronization
- **ELF Loader**: Loads ESP32 ELF binaries into simulated memory

### Peripheral Controllers
- **GPIO Model**: 34 pins with all modes (digital, ADC, DAC, PWM)
- **I2C Controller**: I2C0/I2C1 master/slave simulation
- **SPI Controller**: SPI1/HSPI/VSPI full-duplex simulation
- **UART Controller**: UART0/UART1/UART2 with FIFO simulation
- **WiFi Stack**: Virtual AP with internet routing
- **BLE Stack**: Bluetooth Low Energy simulation

### GUI Components
- **Pinout Panel**: Interactive ESP32 pin diagram
- **Workspace**: Canvas for virtual components
- **Logic Analyzer**: Real-time digital waveform capture
- **Oscilloscope**: Analog signal visualization
- **Debug Controller**: Breakpoints, single-step, GDB server

### Virtual Components
- LEDs (single/RGB), LCDs (16x2, 128x64 OLED)
- Buttons, switches, sensors (temperature, light, ultrasonic)
- Protocol analyzers and signal generators

## Build Requirements

- **Qt 6.x** (Widgets, Charts modules)
- **CMake 3.20+**
- **C++17** compatible compiler
- **ESP-IDF** (for building test firmware)

## Project Structure

```
simulator/
├── core/               # Core emulation engine
│   ├── iss/           # Instruction Set Simulator
│   ├── memory/        # Memory model
│   ├── elf_loader/    # ELF binary loader
│   └── scheduler/     # Event scheduler
├── peripherals/        # Hardware peripheral models
│   ├── gpio/          # GPIO simulation
│   ├── i2c/           # I2C controller
│   ├── spi/           # SPI controller
│   ├── uart/          # UART controller
│   ├── wifi/          # WiFi stack
│   └── ble/           # Bluetooth stack
├── gui/               # Qt6 GUI application
│   ├── pinout/        # Pinout panel
│   ├── workspace/     # Component workspace
│   ├── analyzer/      # Logic analyzer
│   ├── scope/         # Oscilloscope
│   └── debug/         # Debug controller
├── components/        # Virtual components
│   ├── leds/          # LED components
│   ├── displays/      # LCD/OLED displays
│   ├── sensors/       # Virtual sensors
│   └── buttons/       # Switches and buttons
├── protocols/         # Protocol decoders
│   ├── i2c_decoder/   # I2C protocol decoder
│   ├── spi_decoder/   # SPI protocol decoder
│   └── uart_decoder/  # UART protocol decoder
├── CMakeLists.txt     # Main build configuration
└── README.md          # This file
```

## Getting Started

### Prerequisites
```bash
# Install Qt6 (Ubuntu/Debian)
sudo apt-get install qt6-base-dev qt6-charts-dev

# Install Qt6 (macOS)
brew install qt@6

# Install Qt6 (Windows)
# Download from https://www.qt.io/download-qt-installer
```

### Building
```bash
# Configure and build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run simulator
./esp32_simulator
```

## Usage

1. **Load Firmware**: File → Load ELF/Arduino Sketch
2. **Place Components**: Drag virtual components to workspace
3. **Connect Pins**: Wire components to GPIO pins
4. **Run Simulation**: Press Run button or F5
5. **Debug**: Set breakpoints, use logic analyzer
6. **Export Data**: Save waveforms and configurations

## Features

### High-Fidelity Simulation
- Cycle-accurate Xtensa LX6 instruction execution
- Complete ESP32 peripheral modeling
- Real-time signal analysis (10MHz sample rate)
- Accurate timing and interrupt simulation

### Interactive GUI
- Visual ESP32 pinout with real-time state
- Drag-and-drop virtual components
- Multi-channel logic analyzer (up to 34 channels)
- 4-channel oscilloscope for analog signals
- Integrated debugging with GDB server

### Advanced Debugging
- Source-level debugging with breakpoints
- Register and memory inspection
- Call stack analysis
- Protocol decoding (I2C, SPI, UART)
- Performance profiling

### Connectivity Simulation
- Virtual WiFi access point with internet routing
- Bluetooth Low Energy simulation
- TCP/IP packet capture and analysis
- RSSI and packet loss simulation

## Performance

- **Execution Speed**: 0.5× to 10× real-time
- **GUI Frame Rate**: 30+ FPS during simulation
- **Waveform Buffer**: 10 seconds per channel
- **Memory Usage**: <500MB typical

## Integration

The simulator integrates with the existing ESP32 Small OS project:

- Conditional compilation with `SIMULATION` macro
- HAL abstraction layer routing to simulation models
- Same source code runs on hardware and in simulation
- CMake integration for seamless builds

## License

This simulator is part of the ESP32 Small OS project and follows the same licensing terms.

## Contributing

See CONTRIBUTING.md for guidelines on contributing to the simulator project.

## Support

For issues and questions:
1. Check the documentation
2. Search existing issues
3. Create new issue with detailed description
4. Include firmware binary and reproduction steps
