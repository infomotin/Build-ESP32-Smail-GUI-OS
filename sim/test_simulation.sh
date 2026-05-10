#!/bin/bash

# ESP32 Small OS Simulation Test Script
# Tests the simulation environment

echo "ESP32 Small OS Simulation Test"
echo "=============================="

# Check if required tools are available
echo "Checking prerequisites..."

# Check for gcc
if ! command -v gcc &> /dev/null; then
    echo "ERROR: gcc not found. Please install build-essential package."
    exit 1
fi

# Check for make
if ! command -v make &> /dev/null; then
    echo "ERROR: make not found. Please install make package."
    exit 1
fi

echo "✓ Prerequisites check passed"

# Clean previous build
echo "Cleaning previous build..."
make clean > /dev/null 2>&1

# Build the simulation
echo "Building simulation..."
if make > build.log 2>&1; then
    echo "✓ Build successful"
else
    echo "✗ Build failed. Check build.log for details."
    echo "Last 10 lines of build.log:"
    tail -10 build.log
    exit 1
fi

# Check if executable was created
if [ -f "esp32_small_os_sim" ]; then
    echo "✓ Executable created"
else
    echo "✗ Executable not found"
    exit 1
fi

# Test basic functionality (short run)
echo "Testing basic functionality (5 second run)..."
timeout 5s ./esp32_small_os_sim > test_output.log 2>&1
SIM_EXIT_CODE=$?

if [ $SIM_EXIT_CODE -eq 124 ]; then
    echo "✓ Simulation ran successfully (timeout as expected)"
elif [ $SIM_EXIT_CODE -eq 0 ]; then
    echo "✓ Simulation completed successfully"
else
    echo "✗ Simulation failed with exit code $SIM_EXIT_CODE"
    echo "Last 10 lines of test_output.log:"
    tail -10 test_output.log
    exit 1
fi

# Check output for expected patterns
echo "Validating output..."
if grep -q "ESP32 Small OS" test_output.log; then
    echo "✓ System startup detected"
else
    echo "✗ System startup not detected"
    exit 1
fi

if grep -q "GPIO" test_output.log; then
    echo "✓ GPIO operations detected"
else
    echo "✗ GPIO operations not detected"
    exit 1
fi

if grep -q "scheduler" test_output.log; then
    echo "✓ Task scheduler detected"
else
    echo "✗ Task scheduler not detected"
    exit 1
fi

echo "✓ All tests passed"
echo ""
echo "Simulation environment is ready!"
echo "Run './esp32_small_os_sim' to start the full simulation."
echo ""
echo "Simulation features:"
echo "- 30-second runtime (adjustable in sim_main.c)"
echo "- Real-time system monitoring"
echo "- GPIO, WiFi, HTTP server simulation"
echo "- Memory allocation tracking"
echo "- Press Ctrl+C to stop early"

# Cleanup
rm -f build.log test_output.log

exit 0
