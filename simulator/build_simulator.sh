#!/bin/bash

echo "ESP32 Virtual Hardware Simulator Build Script"
echo "============================================"
echo

# Check for Qt6
if ! command -v qmake &> /dev/null; then
    echo "ERROR: Qt6 qmake not found in PATH"
    echo "Please install Qt6 and add it to your PATH"
    echo "Download from: https://www.qt.io/download-qt-installer"
    exit 1
fi

echo "Qt6 found:"
qmake -version
echo

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found in PATH"
    echo "Please install CMake and add it to your PATH"
    echo "Download from: https://cmake.org/download/"
    exit 1
fi

echo "CMake found:"
cmake --version | head -n 1
echo

# Create build directory
if [ ! -d "build" ]; then
    mkdir build
fi
cd build

echo "Configuring project..."
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed"
    exit 1
fi

echo
echo "Building simulator..."
cmake --build . --config Release -j4
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo
echo "Build successful!"
echo
echo "Starting ESP32 Virtual Hardware Simulator..."
echo

# Run the simulator
if [ -f "Release/esp32_simulator.exe" ]; then
    echo "Running simulator..."
    ./Release/esp32_simulator.exe
elif [ -f "esp32_simulator.exe" ]; then
    echo "Running simulator..."
    ./esp32_simulator.exe
else
    echo "ERROR: Simulator executable not found"
    echo "Please check the build output for errors"
    exit 1
fi

echo
echo "Simulator execution completed."
