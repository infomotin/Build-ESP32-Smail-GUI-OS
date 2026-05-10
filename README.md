# ESP32 Small OS - Web GUI Manager

A comprehensive embedded operating system for ESP32 with web-based management interface, real-time monitoring, and GPIO control capabilities.

## Features

### Core System
- **Real-time Operating System**: Built on FreeRTOS with task scheduling
- **Web-based GUI**: Modern responsive interface for system management
- **GPIO Management**: Full control over ESP32 GPIO pins with digital and PWM support
- **Activity Manager**: Event-driven task execution system
- **Storage System**: Persistent configuration and data storage
- **Network Management**: WiFi connectivity and network services

### Web Interface
- **System Dashboard**: Real-time monitoring of system status
- **GPIO Control**: Interactive pin control and monitoring
- **Activity Management**: Trigger and manage system activities
- **System Logs**: Real-time log viewing and management
- **Authentication**: Secure token-based authentication with sessions

### API Endpoints

#### System Management
- `GET /api/status` - System status and metrics
- `POST /api/restart` - Restart the system
- `POST /api/shutdown` - Shutdown the system

#### GPIO Control
- `GET /api/gpio` - Get all GPIO pin states
- `POST /api/gpio/{pin}/toggle` - Toggle specific GPIO pin

#### Activity Management
- `POST /api/activity/{id}` - Trigger activity by ID

#### Real-time Monitoring
- `POST /api/monitor/start` - Start real-time monitoring
- `POST /api/monitor/stop` - Stop real-time monitoring
- `GET /api/monitor/data` - Get current monitoring data

#### Authentication
- `POST /api/auth/login` - Login with authentication token
- `POST /api/auth/logout` - Logout and invalidate session

#### Web Interface
- `GET /` - Main web interface
- `GET /api/dashboard` - Dashboard configuration
- `GET /api/resources` - Available web resources
- `GET /resource/{name}` - Serve web resources

## Hardware Requirements

- ESP32 development board
- WiFi network connectivity
- Minimum 4MB flash storage
- 520KB RAM minimum

## Software Dependencies

- ESP-IDF Framework
- cJSON library
- mbedTLS library
- FreeRTOS

## Installation

### Prerequisites
1. Install ESP-IDF development environment
2. Configure ESP-IDF paths and tools

### Build and Flash
```bash
# Clone or download the project
cd "Build ESP32 Smail GUI OS"

# Configure the project
idf.py menuconfig

# Build the firmware
idf.py build

# Flash to ESP32
idf.py -p PORT flash monitor
```

### Configuration
1. Run `idf.py menuconfig`
2. Configure WiFi settings under "WiFi Configuration"
3. Set up GPIO pin configurations if needed
4. Adjust system parameters as required

## Usage

### Web Interface Access
1. Connect ESP32 to your WiFi network
2. Open a web browser and navigate to the ESP32's IP address
3. Use the authentication token from the serial output for login

### Authentication
- The system generates a secure authentication token on first boot
- Token is stored in persistent storage and logged to serial output
- Use the token to create sessions via the login API

### GPIO Control
- Configure pins as input/output or PWM
- Control pin states through the web interface
- Monitor real-time pin status

### Activity Management
- Define custom activities in the activity manager
- Trigger activities via web interface or API
- Monitor activity execution through system logs

## Architecture

### Core Components

#### Main System (`main.c`)
- System initialization and main loop
- Component coordination and startup sequence

#### Web Server (`web_server.c/.h`)
- HTTP server implementation
- API endpoint handlers
- Static resource serving

#### Web View Manager (`web_view_manager.c/.h`)
- Dynamic web interface generation
- Resource management
- Dashboard configuration

#### Real-time Monitor (`realtime_monitor.c/.h`)
- System metrics collection
- Performance monitoring
- GPIO state tracking

#### GPIO Manager (`gpio_manager.c/.h`)
- Pin configuration and control
- PWM signal generation
- ADC reading support

#### Activity Manager (`activity_manager.c/.h`)
- Event-driven task execution
- Activity scheduling
- Event handling

#### Authentication (`auth.c/.h`)
- Token-based security
- Session management
- Secure token generation

#### Scheduler (`scheduler.c/.h`)
- Event queue management
- Task scheduling
- Inter-component communication

#### Storage (`storage.c/.h`)
- Persistent data storage
- Configuration management
- NVS integration

## Security Features

- **Token-based Authentication**: Secure random token generation
- **Session Management**: Time-limited sessions with automatic expiration
- **Input Validation**: Comprehensive input sanitization
- **Secure Storage**: Encrypted configuration storage

## Monitoring Capabilities

- **System Metrics**: CPU usage, memory usage, uptime
- **GPIO Monitoring**: Real-time pin state tracking
- **Performance Monitoring**: System performance metrics
- **Logging**: Comprehensive system logging

## Development

### Adding New Features
1. Implement core functionality in appropriate component
2. Add API endpoints in web server
3. Update web interface if needed
4. Add authentication protection for sensitive operations

### Custom Activities
1. Define activity in activity manager
2. Implement activity execution logic
3. Add web interface controls
4. Update API documentation

## Troubleshooting

### Common Issues

1. **WiFi Connection Issues**
   - Check WiFi credentials in configuration
   - Verify network availability
   - Monitor serial output for connection status

2. **Web Interface Not Accessible**
   - Verify ESP32 IP address
   - Check network connectivity
   - Ensure web server is running

3. **Authentication Failures**
   - Check authentication token from serial output
   - Verify token format and validity
   - Clear stored configuration if needed

### Debugging
- Enable verbose logging in menuconfig
- Monitor serial output for system messages
- Use web interface logs for API debugging

## License

This project is provided as-is for educational and development purposes.

## Contributing

Contributions are welcome for bug fixes, feature enhancements, and documentation improvements.

## Support

For issues and questions, please refer to the ESP-IDF documentation and project logs.
