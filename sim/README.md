# ESP32 Small OS Simulation

This directory contains the simulation environment for running the ESP32 Small OS on a PC for testing and debugging.

## Features

- **Hardware Simulation**: Simulates ESP32 GPIO, WiFi, HTTP server, and other peripherals
- **Task Scheduler**: Full cooperative task scheduler simulation
- **Memory Management**: Simulated memory allocation with tracking
- **Web Interface**: Simulated HTTP server and web interface
- **Real-time Monitoring**: Live system statistics and debugging output
- **Cross-platform**: Works on Linux, macOS, and Windows (with WSL)

## Prerequisites

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y build-essential libcjson-dev
```

### macOS
```bash
brew install gcc
```

### Windows (WSL)
```bash
# Install WSL and Ubuntu
wsl --install
# Then follow Linux instructions
```

## Building and Running

### Quick Start
```bash
# Build and run the simulation
make run
```

### Build Only
```bash
make
```

### Debug Build
```bash
make debug
```

### Clean Build
```bash
make clean
```

## Simulation Output

The simulation provides real-time output showing:
- System initialization
- Task scheduling
- GPIO operations (LED blinking, button presses)
- WiFi connection status
- HTTP server operations
- Memory allocation tracking
- System monitoring data

### Example Output
```
ESP32 Small OS Simulation Runner
================================
Starting ESP32 Small OS in simulation mode...

========================================
  ESP32 Small OS (Simulation) v1.0.0
========================================

[INIT] Initializing ESP32 Small OS (Simulation) v1.0.0
[INIT] Initializing NVS...
[INIT] NVS Flash initialized (simulated)
[INIT] Initializing GPIO...
[INIT] GPIO HAL initialized
[INIT] Configured GPIO 0 as mode 1
[INIT] Configured GPIO 2 as mode 2
[INIT] Configured GPIO 4 as mode 3
[INIT] Configured GPIO 36 as mode 4
[INIT] Initializing scheduler...
[INIT] Scheduler initialized
[INIT] System initialized successfully
[TASKS] Creating system tasks...
[TASKS] All tasks created successfully

[SIM] Starting simulation loop...
[SIM] Duration: 30 seconds
[SIM] Press Ctrl+C to stop early

[MONITOR] System Status (tick 1):
  Uptime: 100 ms
  Free Heap: 327680 bytes
  CPU Freq: 240000000 Hz
  LED State: OFF
  Button State: RELEASED
[GPIO] Pin 2 set to HIGH
[LED] LED toggled to ON
[WIFI] Initializing WiFi...
[WIFI] WiFi initialized (simulated)
[GPIO] ADC Read: 2148 (1.73V)
[HTTP] Initializing HTTP server...
[HTTP] HTTP server initialized (simulated)
...
```

## Simulation Duration

The simulation runs for 30 seconds by default. You can modify this by changing `SIMULATION_DURATION_MS` in `sim_main.c`.

To stop early, press `Ctrl+C`.

## System Components Simulated

### GPIO
- **GPIO 0**: Button input (simulated with automatic presses)
- **GPIO 2**: LED output (toggles every second)
- **GPIO 4**: PWM output (simulated duty cycle changes)
- **GPIO 36**: ADC input (simulated sensor readings)

### WiFi
- Simulated WiFi initialization and connection
- Network interface setup
- Event handling

### HTTP Server
- Simulated HTTP server on port 80
- Mock API responses for GPIO control
- Web interface simulation

### Task Scheduler
- Cooperative multitasking
- Priority-based scheduling
- Task delay and yield operations

### Memory Management
- Memory allocation tracking
- Leak detection simulation
- Usage statistics

## Debugging

### Debug Build
```bash
make debug
./esp32_small_os_sim
```

### Verbose Logging
The simulation automatically provides detailed logging for:
- Memory allocations
- GPIO operations
- Task scheduling
- System events

## File Structure

```
sim/
├── sim_runner.c      # Main simulation runner and ESP32 API simulation
├── sim_esp32.h       # ESP32 API compatibility header
├── sim_main.c        # Simulation-specific main application
├── Makefile          # Build configuration
└── README.md         # This file
```

## Customization

### Adding New Components
1. Add simulation functions to `sim_runner.c`
2. Add API declarations to `sim_esp32.h`
3. Update `sim_main.c` to use the new components

### Modifying Simulation Parameters
- Edit `sim_main.c` to change GPIO configurations
- Modify `SIMULATION_DURATION_MS` for different run times
- Adjust task priorities and timing in task functions

## Troubleshooting

### Build Issues
```bash
# Install missing dependencies
make install-deps

# Clean and rebuild
make clean
make
```

### Runtime Issues
- Ensure all dependencies are installed
- Check system permissions
- Verify sufficient memory available

### Simulation Not Starting
- Check that all source files are present
- Verify build completed successfully
- Check for any error messages during initialization

## Integration with Real ESP32

The simulation uses the same API as the real ESP32 Small OS, making it easy to:
- Test code logic without hardware
- Debug algorithms and data structures
- Validate system behavior
- Develop and test new features

To switch to real hardware, simply compile without the `-DSIMULATION` flag and flash to ESP32.

## Performance

The simulation is designed to be lightweight and efficient:
- Minimal CPU overhead
- Fast startup time
- Real-time scheduling simulation
- Memory-efficient operation

## Contributing

To contribute to the simulation:
1. Test changes thoroughly in simulation first
2. Ensure compatibility with real ESP32 hardware
3. Update documentation as needed
4. Follow coding standards

## License

This simulation environment is part of the ESP32 Small OS project and follows the same licensing terms.
