# Requirements Document

## Introduction

This document defines the requirements for the **ESP32 Virtual Hardware Simulation Environment** — a high-fidelity, desktop-based simulator that enables developers to compile, load, and execute ESP32 firmware (ELF binaries or Arduino sketches) without physical hardware. The environment provides a graphical user interface with an interactive ESP32 pinout diagram, drag-and-drop virtual components (LEDs, LCD displays, sensors), accurate peripheral simulation (I2C, SPI, UART), Wi-Fi/Bluetooth connectivity simulation, a real-time logic analyzer and oscilloscope view, and comprehensive debugging tools. It integrates with the existing ESP32 Small OS project by extending the `sim/` subsystem and the GPIO HAL.

---

## Glossary

- **Simulator**: The ESP32 Virtual Hardware Simulation Environment desktop application.
- **ISS**: Instruction Set Simulator — the engine that fetches, decodes, and executes Xtensa LX6 instructions from a loaded firmware binary.
- **Emulation_Engine**: The combined ISS and peripheral model subsystem responsible for executing firmware and driving peripheral state.
- **ELF_Loader**: The component that parses and loads compiled ESP32 ELF binary files into the simulated memory space.
- **Memory_Model**: The component that models the ESP32 memory map including IRAM, DRAM, ROM, and memory-mapped I/O regions.
- **GPIO_Model**: The component that simulates all 34 ESP32 GPIO pins including input, output, open-drain, pull-up/pull-down, ADC, DAC, and PWM modes.
- **Peripheral_Controller**: A component that simulates a specific hardware peripheral (I2C, SPI, UART, Timer, LEDC, ADC, DAC).
- **Virtual_Component**: A software-rendered interactive element (LED, LCD, button, sensor) that connects to GPIO pins and responds to electrical state changes.
- **Pinout_Panel**: The GUI panel displaying the ESP32-WROOM-32 module pin diagram with interactive, color-coded pins.
- **Workspace**: The GUI canvas where Virtual_Components are placed, wired to pins, and interacted with.
- **Logic_Analyzer**: The GUI panel that captures and displays digital signal waveforms across selected GPIO pins over time.
- **Oscilloscope**: The GUI panel that renders analog waveforms (PWM, ADC voltage) with configurable timebase and voltage scaling.
- **Protocol_Decoder**: The component that decodes raw I2C, SPI, or UART bit streams into human-readable packet listings.
- **Debug_Controller**: The component that manages breakpoints, single-step execution, register inspection, and memory watch.
- **Event_Scheduler**: The internal component that manages timed events, interrupt delivery, and synchronization between the ISS and Peripheral_Controllers.
- **Simulation_Clock**: The virtual time counter used by the Event_Scheduler to advance simulation time independently of wall-clock time.
- **VCD_File**: Value Change Dump file — a standard waveform interchange format used by tools such as GTKWave.
- **GDB_Server**: A GDB remote serial protocol server embedded in the Simulator that allows external debuggers to connect for source-level debugging.
- **HAL**: Hardware Abstraction Layer — the existing `gpio/gpio_hal.c` and related files in the project that are conditionally compiled for simulation.
- **Arduino_Sketch**: A `.ino` source file written for the ESP32 Arduino core that the Simulator can compile and run.
- **RSSI**: Received Signal Strength Indicator — a simulated metric representing wireless signal quality.
- **BLE**: Bluetooth Low Energy.
- **LEDC**: LED Control peripheral — the ESP32 hardware PWM controller with 16 channels.
- **DMA**: Direct Memory Access — a hardware mechanism for peripheral-to-memory data transfer without CPU involvement.

---

## Requirements

### Requirement 1: Firmware Loading

**User Story:** As an embedded developer, I want to load a compiled ESP32 ELF binary or Arduino sketch into the Simulator, so that I can execute my firmware without physical hardware.

#### Acceptance Criteria

1. THE ELF_Loader SHALL parse ELF32 files compiled for the Xtensa LX6 architecture and map all loadable segments into the Memory_Model at their specified virtual addresses.
2. WHEN an ELF file with an invalid magic number or unsupported architecture is provided, THE ELF_Loader SHALL reject the file and display a descriptive error message identifying the file path and the specific validation failure.
3. THE ELF_Loader SHALL support ELF files produced by ESP-IDF v4.x and v5.x toolchains.
4. WHEN an Arduino sketch `.ino` file is provided, THE Simulator SHALL invoke the ESP32 Arduino core compiler toolchain and produce an ELF binary before loading it into the Memory_Model.
5. IF the Arduino core compiler toolchain is not found on the host system, THEN THE Simulator SHALL display an error message specifying the missing toolchain and the expected installation path.
6. THE ELF_Loader SHALL extract and expose the firmware symbol table to the Debug_Controller for use in breakpoint resolution and variable inspection.

---

### Requirement 2: Instruction Set Simulation

**User Story:** As an embedded developer, I want the Simulator to accurately execute Xtensa LX6 instructions from my firmware, so that my application logic runs correctly in the virtual environment.

#### Acceptance Criteria

1. THE Emulation_Engine SHALL execute the full Xtensa LX6 base instruction set as defined in the Xtensa ISA Reference Manual.
2. THE Emulation_Engine SHALL execute Xtensa windowed register instructions (ENTRY, RETW, ROTW) with correct register window management.
3. THE Emulation_Engine SHALL simulate the ESP32 dual-core architecture by executing firmware tasks on two independent virtual CPU cores (PRO_CPU and APP_CPU).
4. WHEN cycle-accurate mode is enabled, THE Emulation_Engine SHALL track a cycle counter per instruction and advance the Simulation_Clock by the documented cycle count for each executed instruction.
5. WHILE cycle-accurate mode is disabled, THE Emulation_Engine SHALL execute instructions as fast as the host CPU allows and advance the Simulation_Clock by wall-clock elapsed time.
6. THE Emulation_Engine SHALL deliver hardware interrupts to the executing firmware at the correct Xtensa interrupt level within 1 simulated microsecond of the triggering event.
7. IF an unimplemented instruction opcode is encountered during execution, THEN THE Emulation_Engine SHALL halt execution, log the offending program counter and opcode, and transition the Simulator to a paused state.

---

### Requirement 3: Memory Model

**User Story:** As an embedded developer, I want the Simulator to accurately model the ESP32 memory map, so that my firmware's memory accesses behave as they would on real hardware.

#### Acceptance Criteria

1. THE Memory_Model SHALL implement the ESP32 memory map with the following regions: IRAM (192 KB at 0x40080000), DRAM (296 KB at 0x3FFB0000), Internal ROM (448 KB at 0x40000000), and memory-mapped I/O registers (0x3FF00000–0x3FF7FFFF).
2. WHEN firmware writes to a memory-mapped I/O address belonging to a Peripheral_Controller, THE Memory_Model SHALL forward the write to the corresponding Peripheral_Controller within 1 simulated CPU cycle.
3. WHEN firmware reads from a memory-mapped I/O address belonging to a Peripheral_Controller, THE Memory_Model SHALL return the current register value from the corresponding Peripheral_Controller within 1 simulated CPU cycle.
4. IF firmware attempts to read from or write to an address outside all defined memory regions, THEN THE Memory_Model SHALL trigger a LoadStoreError exception in the Emulation_Engine and log the faulting address and program counter.
5. THE Memory_Model SHALL model the ESP32 external flash (up to 16 MB) as a read-only region accessible via the instruction cache at 0x400C2000.
6. THE Memory_Model SHALL support memory watch points that notify the Debug_Controller when a specified address range is read or written.

---

### Requirement 4: GPIO Simulation

**User Story:** As an embedded developer, I want all 34 ESP32 GPIO pins to be accurately simulated, so that my firmware's digital I/O, ADC, DAC, and PWM operations behave correctly.

#### Acceptance Criteria

1. THE GPIO_Model SHALL simulate all 34 GPIO pins (GPIO0–GPIO39) with individually configurable modes: digital input, digital output, open-drain output, input with pull-up, input with pull-down, ADC input, DAC output, and LEDC PWM output.
2. WHEN firmware configures a GPIO pin as a digital output and writes a logic level, THE GPIO_Model SHALL update the pin state within 1 simulated microsecond and notify all connected Virtual_Components of the new state.
3. WHEN firmware reads a GPIO pin configured as digital input, THE GPIO_Model SHALL return the logic level currently driven by the connected Virtual_Component or the pull-up/pull-down default if no component is connected.
4. THE GPIO_Model SHALL simulate Schmitt trigger hysteresis on input pins with a low-to-high threshold of 1.4 V and a high-to-low threshold of 1.0 V (relative to 3.3 V supply).
5. WHEN a GPIO pin configured for edge-triggered interrupt detection transitions from the inactive to active edge, THE GPIO_Model SHALL enqueue an interrupt request to the Emulation_Engine within 100 simulated nanoseconds.
6. THE GPIO_Model SHALL simulate the LEDC peripheral with 16 channels, configurable frequency (1 Hz to 40 MHz), and 1-bit to 20-bit duty cycle resolution.
7. THE GPIO_Model SHALL simulate the ADC with 12-bit resolution, four attenuation levels (0 dB, 2.5 dB, 6 dB, 11 dB), and a configurable noise model producing ±2 LSB random variation.
8. THE GPIO_Model SHALL simulate the DAC with 8-bit resolution on GPIO25 and GPIO26, producing a voltage output proportional to the written value relative to the 3.3 V supply.
9. IF firmware attempts to configure a GPIO pin that is input-only (GPIO34–GPIO39) as an output, THEN THE GPIO_Model SHALL log a warning and leave the pin in its current input state.

---

### Requirement 5: I2C Peripheral Simulation

**User Story:** As an embedded developer, I want accurate I2C bus simulation, so that I can test firmware that communicates with I2C sensors and actuators.

#### Acceptance Criteria

1. THE Peripheral_Controller for I2C SHALL simulate both I2C master and I2C slave roles on the two ESP32 I2C hardware controllers (I2C0 and I2C1).
2. WHEN firmware initiates an I2C master write transaction, THE Peripheral_Controller SHALL generate START, address, data, and STOP bit sequences on the simulated SDA and SCL lines with timing compliant with the I2C specification (tHD;STA ≥ 4.0 µs, tSU;STO ≥ 4.0 µs at 100 kHz standard mode).
3. WHEN a Virtual_Component acting as an I2C slave receives its configured 7-bit address, THE Peripheral_Controller SHALL assert ACK on the SDA line and deliver the data bytes to the Virtual_Component's receive buffer.
4. IF no Virtual_Component acknowledges an I2C address during a master transaction, THEN THE Peripheral_Controller SHALL assert NACK and set the I2C_SR register NACK bit, causing the firmware's I2C driver to receive an ESP_ERR_TIMEOUT or equivalent error code.
5. THE Peripheral_Controller for I2C SHALL simulate clock stretching by holding SCL low for up to 10 ms when a slave Virtual_Component signals it is not ready.
6. THE Protocol_Decoder SHALL decode I2C transactions from the simulated SDA/SCL waveforms and display address, direction, data bytes, and ACK/NACK status in a timestamped packet list.

---

### Requirement 6: SPI Peripheral Simulation

**User Story:** As an embedded developer, I want accurate SPI bus simulation, so that I can test firmware that communicates with SPI displays, flash chips, and sensors.

#### Acceptance Criteria

1. THE Peripheral_Controller for SPI SHALL simulate the three ESP32 SPI controllers (SPI1/HSPI/VSPI) in both master and slave roles.
2. THE Peripheral_Controller for SPI SHALL support all four SPI clock polarity and phase modes (CPOL=0/CPHA=0, CPOL=0/CPHA=1, CPOL=1/CPHA=0, CPOL=1/CPHA=1).
3. WHEN firmware initiates an SPI master transfer, THE Peripheral_Controller SHALL shift data out on MOSI and capture data on MISO simultaneously (full-duplex) at the configured clock frequency.
4. THE Peripheral_Controller for SPI SHALL simulate DMA-linked list transfers by processing descriptor chains and generating a transfer-complete interrupt after the final descriptor is consumed.
5. THE Protocol_Decoder SHALL decode SPI transactions from the simulated MOSI, MISO, SCLK, and CS waveforms and display transaction boundaries, byte values, and timing in a timestamped packet list.

---

### Requirement 7: UART Peripheral Simulation

**User Story:** As an embedded developer, I want accurate UART simulation, so that I can test firmware serial communication and view printf output in the Simulator console.

#### Acceptance Criteria

1. THE Peripheral_Controller for UART SHALL simulate the three ESP32 UART controllers (UART0, UART1, UART2) with configurable baud rate (300 bps to 5 Mbps), data bits (5–8), stop bits (1, 1.5, 2), and parity (none, even, odd).
2. WHEN firmware writes a byte to the UART TX FIFO, THE Peripheral_Controller SHALL serialize the byte at the configured baud rate and deliver it to the connected Virtual_Component or the Simulator console within the calculated bit-period time.
3. THE Peripheral_Controller for UART SHALL simulate TX and RX FIFO buffers of 128 bytes each and generate FIFO-full and FIFO-threshold interrupts at the firmware-configured threshold levels.
4. WHEN UART0 transmits data, THE Simulator SHALL display the decoded ASCII text in a dedicated console panel in real time.
5. THE Protocol_Decoder SHALL decode UART frames from the simulated TX/RX waveforms and display start bit, data bits, parity bit, stop bit, and decoded byte value in a timestamped packet list.
6. WHERE hardware flow control is enabled by firmware, THE Peripheral_Controller for UART SHALL simulate RTS and CTS signal handshaking between the firmware and the connected Virtual_Component.

---

### Requirement 8: Wi-Fi Connectivity Simulation

**User Story:** As an embedded developer, I want Wi-Fi connectivity simulation, so that I can test my firmware's network stack and HTTP/MQTT client behavior without a physical access point.

#### Acceptance Criteria

1. THE Simulator SHALL provide a virtual Wi-Fi access point that firmware can discover, associate with, and obtain an IP address from via DHCP.
2. WHEN firmware calls `esp_wifi_connect()` with valid credentials matching the virtual AP configuration, THE Simulator SHALL complete the association handshake and deliver a `WIFI_EVENT_STA_CONNECTED` event to the firmware within 500 simulated milliseconds.
3. IF firmware calls `esp_wifi_connect()` with credentials that do not match the virtual AP configuration, THEN THE Simulator SHALL deliver a `WIFI_EVENT_STA_DISCONNECTED` event with reason code `WIFI_REASON_AUTH_FAIL` to the firmware.
4. THE Simulator SHALL route TCP/IP packets from the firmware's lwIP stack to the host operating system's network stack, enabling firmware HTTP clients to reach real internet endpoints.
5. THE Simulator SHALL simulate RSSI values between −30 dBm and −90 dBm, configurable by the user in the Simulator control panel, and report the configured value when firmware calls `esp_wifi_sta_get_ap_info()`.
6. THE Simulator SHALL simulate Wi-Fi packet loss by dropping a configurable percentage (0%–100%) of outbound packets to test firmware retry and error-handling logic.
7. WHEN firmware calls `esp_wifi_disconnect()`, THE Simulator SHALL deliver a `WIFI_EVENT_STA_DISCONNECTED` event and clear the firmware's IP address within 100 simulated milliseconds.

---

### Requirement 9: Bluetooth Connectivity Simulation

**User Story:** As an embedded developer, I want Bluetooth Low Energy simulation, so that I can test my firmware's BLE GATT server and client behavior without physical hardware.

#### Acceptance Criteria

1. THE Simulator SHALL simulate a BLE radio that firmware can initialize using the standard ESP-IDF Bluetooth controller and Bluedroid stack APIs.
2. WHEN firmware starts BLE advertising, THE Simulator SHALL make the virtual device discoverable to a built-in virtual BLE scanner panel in the Simulator GUI.
3. WHEN a user initiates a connection from the virtual BLE scanner panel to the advertising firmware device, THE Simulator SHALL complete the BLE connection establishment and deliver a `ESP_GATTS_CONNECT_EVT` to the firmware's GATT server callback.
4. THE Simulator SHALL simulate BLE GATT read and write operations between the virtual BLE scanner panel and the firmware's GATT server, delivering the correct GATT events and data to the firmware callbacks.
5. THE Simulator SHALL simulate BLE RSSI values between −40 dBm and −80 dBm, configurable per virtual connection in the Simulator GUI.
6. IF firmware attempts to use Classic Bluetooth (BR/EDR) APIs, THEN THE Simulator SHALL log a warning indicating that Classic Bluetooth simulation is not supported and return `ESP_ERR_NOT_SUPPORTED` from the relevant API calls.

---

### Requirement 10: GUI — Pinout Panel

**User Story:** As an embedded developer, I want an interactive visual ESP32 pinout diagram, so that I can see and configure pin states at a glance.

#### Acceptance Criteria

1. THE Pinout_Panel SHALL render a 2D diagram of the ESP32-WROOM-32 module showing all 38 exposed pins with their physical positions and labels.
2. THE Pinout_Panel SHALL color-code each pin according to its current configured function: green for digital output HIGH, dark gray for digital output LOW, blue for ADC input, orange for PWM output, yellow for I2C, purple for SPI, cyan for UART, and white for unconfigured.
3. WHEN firmware changes a GPIO pin's mode or output level, THE Pinout_Panel SHALL update the corresponding pin's color and label within 16 milliseconds (one 60 Hz display frame).
4. WHEN a user clicks a pin in the Pinout_Panel, THE Simulator SHALL display a tooltip showing the pin number, current mode, current logic level or analog voltage, and the name of any connected Virtual_Component.
5. WHEN a user right-clicks a pin in the Pinout_Panel, THE Simulator SHALL display a context menu allowing the user to manually drive the pin HIGH or LOW (for input pins), connect a Virtual_Component, or open the pin's waveform in the Logic_Analyzer.
6. THE Pinout_Panel SHALL visually distinguish input-only pins (GPIO34–GPIO39) from bidirectional pins using a distinct border style.

---

### Requirement 11: GUI — Workspace and Virtual Components

**User Story:** As an embedded developer, I want to place and connect virtual components on a workspace canvas, so that I can build a virtual circuit that interacts with my firmware.

#### Acceptance Criteria

1. THE Workspace SHALL allow users to drag Virtual_Components from a component library panel and drop them onto a grid-aligned canvas.
2. WHEN a Virtual_Component is dropped onto the Workspace, THE Simulator SHALL display a connection dialog prompting the user to assign the component's signal pins to ESP32 GPIO pins.
3. THE Workspace SHALL render connection wires between Virtual_Component pins and ESP32 GPIO pins, with wires routed orthogonally and labeled with the GPIO pin number.
4. THE Simulator SHALL provide the following built-in Virtual_Components: single-color LED, RGB LED, 16×2 character LCD (HD44780 compatible), 128×64 OLED display (SSD1306 I2C), push button, toggle switch, NTC thermistor (temperature sensor), LDR (light sensor), HC-SR04 ultrasonic distance sensor, and DHT22 temperature/humidity sensor.
5. WHEN the GPIO_Model drives a GPIO pin connected to an LED Virtual_Component HIGH, THE LED Virtual_Component SHALL illuminate with a configurable color and brightness proportional to the simulated forward current.
6. WHEN firmware sends HD44780-compatible commands over a GPIO bus connected to a 16×2 LCD Virtual_Component, THE LCD Virtual_Component SHALL render the correct characters in the correct cursor positions within 1 simulated millisecond of the enable pulse.
7. WHEN firmware sends SSD1306 I2C commands to a 128×64 OLED Virtual_Component, THE OLED Virtual_Component SHALL update its pixel buffer and re-render the display within 16 milliseconds.
8. WHEN a user clicks a push button Virtual_Component in the Workspace, THE Workspace SHALL drive the connected GPIO pin to the active logic level for the duration of the click and release it on mouse-up, simulating a debounced press with a 5 ms bounce window.
9. THE Workspace SHALL allow users to configure Virtual_Component parameters (LED color, sensor output value, button active level) via a properties panel that opens on double-click.
10. THE Workspace SHALL persist the component layout, wiring, and configuration to a project file so that the simulation environment can be restored across sessions.

---

### Requirement 12: Logic Analyzer

**User Story:** As an embedded developer, I want a real-time logic analyzer view, so that I can capture and inspect digital signal waveforms across GPIO pins and protocol buses.

#### Acceptance Criteria

1. THE Logic_Analyzer SHALL capture digital state transitions on up to 34 GPIO channels simultaneously at a minimum sample rate of 10 MHz (100 ns resolution).
2. THE Logic_Analyzer SHALL display captured waveforms as horizontal time-domain traces, one per channel, with a configurable time window of 100 µs to 10 s.
3. WHEN a user configures a trigger condition (rising edge, falling edge, or logic pattern) on a selected channel, THE Logic_Analyzer SHALL begin recording waveform data from the moment the trigger condition is met.
4. THE Logic_Analyzer SHALL allow users to place two vertical cursor markers on the waveform display and SHALL display the time difference between the cursors and the frequency of any periodic signal between them.
5. THE Logic_Analyzer SHALL overlay Protocol_Decoder annotations on the waveform traces for I2C, SPI, and UART channels when the corresponding protocol decoder is enabled.
6. THE Logic_Analyzer SHALL export captured waveform data to VCD format so that users can open the data in external tools such as GTKWave.
7. WHEN the Simulator is paused, THE Logic_Analyzer SHALL freeze the waveform display at the current simulation time and allow the user to scroll and zoom the captured history.

---

### Requirement 13: Oscilloscope View

**User Story:** As an embedded developer, I want an oscilloscope view for analog signals, so that I can visualize PWM waveforms and ADC input voltages.

#### Acceptance Criteria

1. THE Oscilloscope SHALL display analog voltage waveforms for GPIO pins configured as LEDC PWM output or ADC input, with voltage range 0 V to 3.3 V.
2. THE Oscilloscope SHALL provide a configurable timebase from 1 µs/div to 1 s/div and a voltage scale from 0.1 V/div to 3.3 V/div.
3. THE Oscilloscope SHALL calculate and display the following measurements for the selected channel: frequency, period, duty cycle (for PWM), minimum voltage, maximum voltage, and RMS voltage.
4. WHEN the PWM duty cycle or frequency is changed by firmware, THE Oscilloscope SHALL update the displayed waveform within 100 simulated milliseconds.
5. THE Oscilloscope SHALL support simultaneous display of up to 4 analog channels with independent vertical scaling and color-coded traces.

---

### Requirement 14: Debugging Tools

**User Story:** As an embedded developer, I want comprehensive debugging tools, so that I can identify and fix firmware bugs before deploying to physical hardware.

#### Acceptance Criteria

1. THE Debug_Controller SHALL allow users to set software breakpoints at any instruction address or source-level line number (when debug symbols are present in the ELF file).
2. WHEN execution reaches a breakpoint address, THE Emulation_Engine SHALL halt execution, notify the Debug_Controller, and transition the Simulator to a paused state within 1 simulated CPU cycle of the breakpoint instruction.
3. WHILE the Simulator is in a paused state, THE Debug_Controller SHALL display the current values of all 64 Xtensa general-purpose registers (AR0–AR63), the program counter, and the window base register.
4. WHILE the Simulator is in a paused state, THE Debug_Controller SHALL allow users to inspect and modify any memory address in the Memory_Model.
5. THE Debug_Controller SHALL support single-step execution, advancing the Emulation_Engine by exactly one instruction per user action.
6. THE Debug_Controller SHALL support step-over execution, advancing the Emulation_Engine until the instruction following the current CALL instruction is reached.
7. THE Debug_Controller SHALL support step-out execution, advancing the Emulation_Engine until the current function returns.
8. THE Simulator SHALL embed a GDB_Server implementing the GDB remote serial protocol on a configurable TCP port (default 3333), allowing external GDB clients to connect and perform source-level debugging.
9. THE Debug_Controller SHALL maintain a memory watch list of up to 32 address ranges and SHALL pause execution and display a notification when any watched address is written with a value that differs from its previous value.
10. THE Debug_Controller SHALL display a call stack trace showing the function name, source file, and line number for each frame when debug symbols are available.

---

### Requirement 15: Simulation Control and Timing

**User Story:** As an embedded developer, I want to control simulation speed and execution flow, so that I can run firmware at different speeds for debugging and testing.

#### Acceptance Criteria

1. THE Simulator SHALL provide a control bar with Run, Pause, Reset, and Step buttons that control the Emulation_Engine execution state.
2. THE Simulator SHALL support adjustable simulation speed from 0.1× to 10× real time, configurable via a speed slider in the control bar.
3. WHEN the Reset button is activated, THE Simulator SHALL reset the Emulation_Engine program counter to the firmware reset vector, clear all peripheral state, and restore all GPIO pins to their power-on default states within 100 milliseconds of wall-clock time.
4. THE Simulator SHALL display the current simulation time (in microseconds), the simulated CPU frequency, and an estimated power consumption value (in milliwatts) in the status bar.
5. WHILE the Simulator is running, THE Simulator SHALL update the status bar values at a minimum rate of 4 Hz.
6. THE Event_Scheduler SHALL process all pending timed events in Simulation_Clock order before advancing the Simulation_Clock, ensuring that no event is skipped due to time advancement.

---

### Requirement 16: Performance

**User Story:** As an embedded developer, I want the Simulator to run typical IoT firmware at near real-time speed, so that time-sensitive behavior such as Wi-Fi keep-alive and sensor polling is observable.

#### Acceptance Criteria

1. THE Simulator SHALL execute typical ESP32 IoT firmware (FreeRTOS task scheduler, Wi-Fi stack, one I2C sensor, one UART output) at a minimum of 0.5× real-time speed on a host machine with a 4-core CPU at 3.0 GHz or faster.
2. THE Simulator SHALL maintain a GUI frame rate of at least 30 frames per second while the Emulation_Engine is running at 1× real-time speed.
3. THE Logic_Analyzer SHALL buffer up to 10 seconds of waveform data per channel in memory without causing the Simulator to drop below 0.5× real-time execution speed.
4. THE Simulator SHALL use a multi-threaded architecture with at minimum three threads: one for the Emulation_Engine, one for the GUI rendering, and one for peripheral I/O event processing.

---

### Requirement 17: Build System Integration

**User Story:** As an embedded developer, I want the Simulator to integrate with the existing ESP32 Small OS build system, so that I can build and launch the simulation from the same project.

#### Acceptance Criteria

1. THE Simulator SHALL be buildable as a separate CMake target (`esp32_simulator`) within the existing project's CMakeLists.txt without modifying the firmware build targets.
2. THE HAL SHALL be conditionally compiled: WHEN the `SIMULATION` preprocessor macro is defined, THE HAL SHALL route all `gpio_hal_*` function calls to the GPIO_Model instead of the ESP-IDF hardware drivers.
3. THE Simulator build SHALL depend on Qt 6.x (Widgets and Charts modules) and SHALL produce a self-contained executable for Windows, macOS, and Linux.
4. THE Simulator SHALL load the existing `sim/sim_runner.c` simulation stubs as a compatibility shim and SHALL replace stub implementations with full Peripheral_Controller models as they are implemented.
5. WHEN the `SIMULATION` macro is defined, THE Simulator SHALL redirect all `ESP_LOGI`, `ESP_LOGE`, `ESP_LOGW`, and `ESP_LOGD` log calls to the Simulator's console panel instead of the host standard output.

---

### Requirement 18: Data Export and Import

**User Story:** As an embedded developer, I want to export simulation data and import test vectors, so that I can compare simulation results with hardware measurements and automate regression testing.

#### Acceptance Criteria

1. THE Logic_Analyzer SHALL export captured waveform data to VCD format with nanosecond timestamp resolution.
2. THE Simulator SHALL import VCD files as stimulus input, driving GPIO pin states according to the value changes in the VCD file to enable automated test vector replay.
3. FOR ALL VCD files exported by the Logic_Analyzer, importing the same VCD file as stimulus and re-running the simulation SHALL produce GPIO output waveforms that match the original capture within ±100 ns (round-trip property).
4. THE Simulator SHALL export the complete simulation session — including firmware binary path, component layout, wiring, and Virtual_Component configurations — to a JSON project file.
5. THE Simulator SHALL import a JSON project file and restore the simulation session to the saved state, including reloading the firmware binary and reconnecting all Virtual_Components.
