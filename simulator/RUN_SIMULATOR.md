# ESP32 Virtual Hardware Simulator - Quick Start Guide

## 🚀 Running the Desktop Simulator

The ESP32 Virtual Hardware Simulation Environment is now ready to run! This professional-grade simulator provides a complete ESP32 development environment with visual debugging capabilities.

### Prerequisites

#### Windows
1. **Qt6** (v6.2 or later)
   - Download from: https://www.qt.io/download-qt-installer
   - Select Qt6 with Qt Widgets and Qt Charts modules
   - Add Qt6 to your system PATH

2. **CMake** (v3.20 or later)
   - Download from: https://cmake.org/download/
   - Add CMake to your system PATH

3. **MinGW** (for Windows)
   - Download from: https://www.mingw-w64.org/
   - Add MinGW to your system PATH

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install qt6-base-dev qt6-charts-dev cmake build-essential
```

#### macOS
```bash
brew install qt@6 cmake
```

### Quick Start

#### Method 1: Use the Build Script (Recommended)
```bash
# Navigate to simulator directory
cd "d:/laragon/www/Build ESP32 Smail GUI OS/simulator"

# Run the build script
build_simulator.bat
```

#### Method 2: Manual Build
```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the simulator
cmake --build . --config Release -j4

# Run the simulator
./esp32_simulator  # Linux/macOS
# or
Release/esp32_simulator.exe  # Windows
```

## 🎮 Using the Simulator

### Main Interface

When you start the simulator, you'll see:

1. **Pinout Panel** (left) - Interactive ESP32 pin diagram
2. **Workspace** (center-left) - Drag-and-drop virtual components
3. **Logic Analyzer** (center-right) - Real-time signal analysis
4. **Simulation Controls** (top-right) - Start/stop/pause controls
5. **Status Panel** (bottom-right) - System information and logs

### Loading Firmware

1. **File → Load Firmware** or click the toolbar button
2. Select your ESP32 ELF binary file
3. The simulator will validate and load the firmware
4. Status will show "Firmware loaded: [filename]"

### Adding Virtual Components

1. In the **Workspace** panel, click component buttons:
   - **Add LED** - Creates a virtual LED on the next available GPIO
   - **Add Button** - Creates a virtual push button
   - **Add Potentiometer** - Creates an analog input sensor

2. Components automatically connect to available GPIO pins
3. Click and interact with components during simulation

### Running Simulation

1. Click **Start** (or press F5) to begin simulation
2. The simulator will execute your ESP32 firmware
3. Watch virtual components respond to GPIO changes
4. Monitor signals in the Logic Analyzer
5. Adjust simulation speed with the speed slider

### Pinout Panel Features

- **Color-coded pins**: Green=HIGH, Gray=LOW, Blue=ADC, Orange=DAC, Magenta=PWM
- **Hover tooltips**: Shows pin mode, level, and connected components
- **Right-click**: Context menu for pin configuration
- **Real-time updates**: Pins update as firmware runs

### Logic Analyzer

- **Real-time capture**: Monitors GPIO signal changes
- **Time scale adjustment**: 1μs to 1s time ranges
- **Channel selection**: Enable/disable specific GPIO channels
- **Interactive cursors**: Measure timing between events
- **Zoom controls**: Mouse wheel to zoom in/out

### Simulation Controls

- **Start/Pause/Stop**: Control simulation execution
- **Reset**: Reset simulation to initial state
- **Step**: Single-step execution
- **Speed Control**: 0.1x to 10x simulation speed

## 🎯 Example Usage

### Basic LED Blink Test

1. **Load firmware** that blinks GPIO2
2. **Add LED component** (automatically connects to GPIO2)
3. **Start simulation**
4. **Watch the LED blink** as your firmware runs
5. **Monitor GPIO2** in the Logic Analyzer

### Button Input Test

1. **Load firmware** that reads GPIO0
2. **Add Button component** (connects to GPIO0)
3. **Start simulation**
4. **Click the virtual button** to test input handling
5. **Observe signal changes** in the Logic Analyzer

### PWM Control Test

1. **Load firmware** that outputs PWM on GPIO4
2. **Add LED component** (connects to GPIO4)
3. **Start simulation**
4. **Observe LED brightness** changes based on PWM duty cycle
5. **Monitor PWM waveform** in the Logic Analyzer

## 🔧 Advanced Features

### GPIO Configuration

Right-click any pin in the Pinout Panel to:
- **Configure pin mode** (Input, Output, ADC, DAC, PWM)
- **Set output level** (HIGH/LOW)
- **Connect/disconnect components**
- **View pin statistics**

### Component Interaction

- **LEDs**: Automatically respond to GPIO output changes
- **Buttons**: Click to drive GPIO low (simulate button press)
- **Potentiometers**: Drag slider to change analog input voltage

### Status Monitoring

The Status Panel shows:
- **Simulation state**: Running/Paused/Stopped
- **Execution time**: Real-time simulation clock
- **Memory usage**: Simulated memory consumption
- **Instruction count**: Total instructions executed
- **Log messages**: Real-time event logging

## 🛠️ Troubleshooting

### Common Issues

**"Qt6 qmake not found"**
- Install Qt6 and add to PATH
- Use the Qt Maintenance Tool to ensure Qt Widgets is installed

**"CMake configuration failed"**
- Ensure CMake is in PATH
- Check that all Qt6 components are installed

**"Firmware loading failed"**
- Verify the file is a valid ESP32 ELF binary
- Check that the file was compiled for Xtensa LX6 architecture

**"Simulation not responding"**
- Check that firmware was loaded successfully
- Try resetting the simulation
- Verify virtual components are properly connected

### Performance Tips

- **Reduce Logic Analyzer channels** if simulation is slow
- **Adjust simulation speed** for better performance
- **Use single-step mode** for debugging timing-sensitive code

## 📚 Supported Features

### ✅ Currently Implemented
- Complete Xtensa LX6 instruction set simulation
- Full ESP32 memory map (IRAM, DRAM, ROM, Flash, MMIO)
- All 34 GPIO pins with digital, ADC, DAC, PWM modes
- Interactive virtual components (LEDs, buttons, potentiometers)
- Real-time logic analyzer with waveform capture
- Professional Qt6 GUI with dark theme
- ELF firmware loading and validation
- Simulation speed control (0.1x to 10x)
- Breakpoint and debugging support

### 🔄 In Development
- I2C/SPI/UART peripheral simulation
- WiFi and Bluetooth connectivity simulation
- Advanced virtual components (LCDs, sensors)
- GDB server integration
- VCD waveform export/import
- Arduino sketch compilation

## 🎯 Getting Help

- **Documentation**: Check the comprehensive requirements document
- **Examples**: Look at the simple_test.c for basic usage
- **Issues**: Report bugs and request features on GitHub
- **Community**: Join the ESP32 Small OS development community

---

**The ESP32 Virtual Hardware Simulation Environment is now ready for professional embedded development!** 🚀

Run `build_simulator.bat` to start your simulation journey!
