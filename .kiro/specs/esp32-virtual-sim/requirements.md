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

1. THE ELF_Loader SHALL validate the ELF32 magic number and Xtensa LX6 architecture identifier before mapping any loadable segments, and SHALL only map segments into the Memory_Model after all structural validations pass.
2. WHEN an ELF file with an invalid magic number or unsupported architecture is provided, THE ELF_Loader SHALL reject the file and display an error message that includes the file path, the expected value, and the actual value found for the failing field.
3. THE ELF_Loader SHALL parse ELF files produced by ESP-IDF v4.x and v5.x toolchains, correctly handling both toolchains' section layouts and entry point conventions without producing a parse error.
4. WHEN an Arduino sketch `.ino` file is provided, THE Simulator SHALL invoke the ESP32 Arduino core compiler toolchain and produce an ELF binary before loading it into the Memory_Model.
5. IF the Arduino core compiler toolchain compilation does not complete within 60 seconds of wall-clock time, THEN THE Simulator SHALL cancel the compilation and display an error message indicating a compilation timeout.
6. IF the Arduino core compiler toolchain is not found on the host system, THEN THE Simulator SHALL display an error message specifying the missing toolchain binary name and the platform-specific default installation path (e.g., `%LOCALAPPDATA%\Arduino15` on Windows, `~/.arduino15` on Linux/macOS).
7. THE ELF_Loader SHALL extract the firmware symbol table and make it available to the Debug_Controller; the symbol table SHALL include, for each symbol, its name, start address, size in bytes, and type (function or object).

---

### Requirement 2: Instruction Set Simulation

**User Story:** As an embedded developer, I want the Simulator to accurately execute Xtensa LX6 instructions from my firmware, so that my application logic runs correctly in the virtual environment.

#### Acceptance Criteria

1. THE Emulation_Engine SHALL execute every opcode in the Xtensa LX6 base instruction set as defined in the Xtensa ISA Reference Manual, producing register and memory results identical to those specified in the ISA for each instruction.
2. WHEN the Emulation_Engine executes an ENTRY instruction, THE Emulation_Engine SHALL rotate the register window by the number of registers specified in the instruction operand; WHEN it executes RETW, THE Emulation_Engine SHALL restore the previous window base; WHEN it executes ROTW, THE Emulation_Engine SHALL rotate the window base by the signed immediate operand.
3. THE Emulation_Engine SHALL simulate the ESP32 dual-core architecture by maintaining two independent program counters, register files, and interrupt states — one for PRO_CPU and one for APP_CPU — such that a write to a register on one core does not affect the register file of the other core.
4. WHEN the user enables cycle-accurate mode via the Simulator control panel, THE Emulation_Engine SHALL advance the Simulation_Clock by the documented cycle count for each executed instruction as specified in the Xtensa LX6 pipeline documentation.
5. WHILE cycle-accurate mode is disabled, THE Emulation_Engine SHALL execute instructions without inserting artificial delays; the Simulation_Clock SHALL advance by a fixed 1-nanosecond increment per instruction regardless of wall-clock time elapsed.
6. THE Emulation_Engine SHALL deliver hardware interrupts to the executing firmware at Xtensa interrupt levels 1 through 7 within 1 simulated microsecond of the event that triggers the interrupt.
7. IF an unimplemented instruction opcode is encountered during execution, THEN THE Emulation_Engine SHALL halt execution and transition the Simulator to a paused state; the halt and pause transition SHALL occur even if logging is unavailable, and SHALL log the program counter and opcode value when logging is available.

---

### Requirement 3: Memory Model

**User Story:** As an embedded developer, I want the Simulator to accurately model the ESP32 memory map, so that my firmware's memory accesses behave as they would on real hardware.

#### Acceptance Criteria

1. THE Memory_Model SHALL implement the ESP32 memory map with the following regions: IRAM (192 KB at 0x40080000, read/write), DRAM (296 KB at 0x3FFB0000, read/write), Internal ROM (448 KB at 0x40000000, read-only), and memory-mapped I/O registers (0x3FF00000–0x3FF7FFFF, read/write).
2. WHEN firmware writes to a memory-mapped I/O address belonging to a Peripheral_Controller, THE Memory_Model SHALL forward the write to the corresponding Peripheral_Controller within 1 simulated CPU cycle.
3. WHEN firmware reads from a memory-mapped I/O address belonging to a Peripheral_Controller, THE Memory_Model SHALL return the current register value from the corresponding Peripheral_Controller within 1 simulated CPU cycle.
4. IF firmware attempts to read from or write to an address outside all defined memory regions, THEN THE Memory_Model SHALL trigger a LoadStoreError exception in the Emulation_Engine and log the faulting address and program counter.
5. WHILE the external flash region (up to 16 MB at 0x400C2000) is mapped, THE Memory_Model SHALL return the byte at the corresponding flash offset for any read access and SHALL treat the region as read-only.
6. IF firmware attempts to write to the Internal ROM region (0x40000000–0x4006FFFF) or the external flash region, THEN THE Memory_Model SHALL trigger a LoadStoreError exception in the Emulation_Engine and log the faulting address and program counter.
7. WHEN firmware configures a memory watchpoint on an address range, THE Memory_Model SHALL detect any read or write access to that range and notify the Debug_Controller within 1 simulated CPU cycle of the access; the Memory_Model SHALL support up to 32 simultaneously active watchpoints.

---

### Requirement 4: GPIO Simulation

**User Story:** As an embedded developer, I want all 34 ESP32 GPIO pins to be accurately simulated, so that my firmware's digital I/O, ADC, DAC, and PWM operations behave correctly.

#### Acceptance Criteria

1. THE GPIO_Model SHALL simulate all 34 GPIO pins (GPIO0–GPIO39) with individually configurable modes: digital input, digital output, open-drain output, input with pull-up, input with pull-down, ADC input, DAC output, and LEDC PWM output.
2. WHEN firmware configures a GPIO pin as a digital output and writes a logic level, THE GPIO_Model SHALL update the pin state within 1 simulated microsecond and notify all connected Virtual_Components of the new state.
3. WHEN firmware reads a GPIO pin configured as digital input, THE GPIO_Model SHALL return the logic level currently driven by the connected Virtual_Component; if no Virtual_Component is connected and a pull-up is configured, THE GPIO_Model SHALL return logic HIGH; if no Virtual_Component is connected and a pull-down is configured, THE GPIO_Model SHALL return logic LOW; if no Virtual_Component is connected and no pull resistor is configured, THE GPIO_Model SHALL return logic LOW.
4. THE GPIO_Model SHALL simulate Schmitt trigger hysteresis on input pins with a low-to-high threshold of 1.4 V and a high-to-low threshold of 1.0 V (relative to 3.3 V supply).
5. WHEN a GPIO pin configured for edge-triggered interrupt detection transitions from the inactive to active edge, THE GPIO_Model SHALL enqueue an interrupt request to the Emulation_Engine within 100 simulated nanoseconds.
6. WHILE a GPIO pin configured for level-triggered interrupt detection is held at the active level, THE GPIO_Model SHALL re-enqueue an interrupt request to the Emulation_Engine each time the firmware clears the interrupt pending bit, until the pin transitions away from the active level.
7. THE GPIO_Model SHALL simulate the LEDC peripheral with 16 channels, configurable frequency (1 Hz to 40 MHz), and 1-bit to 20-bit duty cycle resolution; IF firmware configures a channel with a frequency or resolution outside these bounds, THE GPIO_Model SHALL return an error to the firmware and leave the channel in its previous valid state.
8. THE GPIO_Model SHALL simulate the ADC with 12-bit resolution and the following input voltage ranges per attenuation level: 0 dB → 0–1.1 V, 2.5 dB → 0–1.5 V, 6 dB → 0–2.2 V, 11 dB → 0–3.3 V; inputs above the range ceiling SHALL be clamped to 4095; the noise model SHALL add ±2 LSB random variation to each sample.
9. THE GPIO_Model SHALL simulate the DAC with 8-bit resolution on GPIO25 and GPIO26, producing a voltage output proportional to the written value relative to the 3.3 V supply; IF firmware writes a value outside 0–255, THE GPIO_Model SHALL return an error to the firmware and retain the previous output voltage.
10. IF firmware attempts to configure a GPIO pin that is input-only (GPIO34–GPIO39) as an output, THEN THE GPIO_Model SHALL return an error code to the firmware, log a warning, and leave the pin in its current input state.

---

### Requirement 5: I2C Peripheral Simulation

**User Story:** As an embedded developer, I want accurate I2C bus simulation, so that I can test firmware that communicates with I2C sensors and actuators.

#### Acceptance Criteria

1. THE Peripheral_Controller for I2C SHALL simulate both I2C master and I2C slave roles on the two ESP32 I2C hardware controllers (I2C0 and I2C1).
2. WHEN firmware initiates an I2C master write transaction at 100 kHz, THE Peripheral_Controller SHALL generate START, address, data, and STOP bit sequences with timing compliant with the I2C specification (tHD;STA ≥ 4.0 µs, tSU;STO ≥ 4.0 µs); WHEN firmware initiates a transaction at 400 kHz fast mode, THE Peripheral_Controller SHALL use tHD;STA ≥ 0.6 µs and tSU;STO ≥ 0.6 µs.
3. WHEN firmware initiates an I2C master read transaction, THE Peripheral_Controller SHALL generate a repeated START followed by the 7-bit address with the read bit set, clock in the data bytes from the slave Virtual_Component, and deliver them to the firmware's I2C receive buffer.
4. WHEN a Virtual_Component acting as an I2C slave receives its configured 7-bit address, THE Peripheral_Controller SHALL assert ACK on the SDA line and deliver the data bytes to the Virtual_Component's receive buffer.
5. IF no Virtual_Component acknowledges an I2C address during a master transaction, THEN THE Peripheral_Controller SHALL assert NACK and set the I2C_SR register NACK bit, causing the firmware's I2C driver to receive ESP_FAIL.
6. IF a Virtual_Component's receive buffer is full when the Peripheral_Controller attempts to deliver a data byte, THEN THE Peripheral_Controller SHALL assert NACK on that byte and set the I2C_SR register NACK bit.
7. IF a slave Virtual_Component asserts clock stretching, THEN THE Peripheral_Controller SHALL hold SCL low for up to 10 ms of simulated time; IF the slave does not release SCL within 10 ms, THEN THE Peripheral_Controller SHALL abort the transaction and set the I2C_SR TIMEOUT bit.
8. WHEN the Protocol_Decoder is enabled for an I2C channel, THE Protocol_Decoder SHALL decode each completed I2C transaction from the simulated SDA/SCL waveforms and display the 7-bit address, direction (R/W), each data byte in hexadecimal, and the ACK/NACK status for each byte in a timestamped packet list.

---

### Requirement 6: SPI Peripheral Simulation

**User Story:** As an embedded developer, I want accurate SPI bus simulation, so that I can test firmware that communicates with SPI displays, flash chips, and sensors.

#### Acceptance Criteria

1. THE Peripheral_Controller for SPI SHALL simulate the three ESP32 SPI controllers (SPI1/HSPI/VSPI) in both master and slave roles; WHEN operating as a slave, THE Peripheral_Controller SHALL assert MISO data on the clock edge defined by the configured CPOL/CPHA mode and deliver received MOSI bytes to the connected Virtual_Component's receive buffer.
2. THE Peripheral_Controller for SPI SHALL support all four SPI clock polarity and phase modes (CPOL=0/CPHA=0, CPOL=0/CPHA=1, CPOL=1/CPHA=0, CPOL=1/CPHA=1).
3. WHEN firmware initiates an SPI master transfer, THE Peripheral_Controller SHALL assert CS low before the first clock edge, shift data out on MOSI MSB-first and capture data on MISO simultaneously (full-duplex) at the configured clock frequency, and deassert CS high after the last clock edge; the CS-to-first-clock setup time SHALL be at least one half clock period.
4. WHEN firmware initiates a DMA-linked list SPI transfer, THE Peripheral_Controller SHALL process each descriptor in the chain in order, transferring the data buffer described by each descriptor, and SHALL generate a transfer-complete interrupt after the final descriptor's data has been fully shifted out.
5. WHEN the Protocol_Decoder is enabled for an SPI channel, THE Protocol_Decoder SHALL decode each SPI transaction from the simulated MOSI, MISO, SCLK, and CS waveforms and display the transaction start time, byte count, MOSI bytes in hexadecimal, MISO bytes in hexadecimal, and CS assertion duration in a timestamped packet list.

---

### Requirement 7: UART Peripheral Simulation

**User Story:** As an embedded developer, I want accurate UART simulation, so that I can test firmware serial communication and view printf output in the Simulator console.

#### Acceptance Criteria

1. THE Peripheral_Controller for UART SHALL simulate the three ESP32 UART controllers (UART0, UART1, UART2) with configurable baud rate (300 bps to 5 Mbps), data bits (5–8), stop bits (1, 1.5, 2), and parity (none, even, odd).
2. WHEN firmware writes a byte to the UART TX FIFO, THE Peripheral_Controller SHALL serialize the byte at the configured baud rate and deliver it to the connected Virtual_Component or the Simulator console within 1 simulated bit-period of the last stop bit.
3. THE Peripheral_Controller for UART SHALL simulate TX and RX FIFO buffers of 128 bytes each and SHALL generate FIFO-full and FIFO-threshold interrupts only when the FIFO levels actually reach the firmware-configured threshold values.
4. WHEN UART0 transmits a byte, THE Simulator SHALL append the decoded ASCII character (or a hex escape for non-printable bytes) to the dedicated console panel within 16 milliseconds of simulation time after the stop bit.
5. WHEN the Protocol_Decoder is enabled for a UART channel, THE Protocol_Decoder SHALL decode each UART frame from the simulated TX/RX waveforms and display the start bit timestamp, data bits, parity bit value, stop bit, and decoded byte value in hexadecimal and ASCII in a timestamped packet list.
6. WHERE hardware flow control is enabled by firmware, THE Peripheral_Controller for UART SHALL pause TX serialization when CTS is de-asserted and resume TX serialization when CTS is re-asserted.
7. IF firmware writes a byte to the TX FIFO when the TX FIFO contains 128 bytes, THEN THE Peripheral_Controller SHALL discard the byte and set the UART_STATUS TXFIFO_CNT field to 128.
8. IF the Peripheral_Controller receives a byte with a parity error or framing error, THEN THE Peripheral_Controller SHALL set the corresponding error bits in UART_INT_RAW and deliver a UART interrupt to the Emulation_Engine.

---

### Requirement 8: Wi-Fi Connectivity Simulation

**User Story:** As an embedded developer, I want Wi-Fi connectivity simulation, so that I can test my firmware's network stack and HTTP/MQTT client behavior without a physical access point.

#### Acceptance Criteria

1. THE Simulator SHALL provide a virtual Wi-Fi access point that firmware can discover, associate with, and obtain an IP address from via DHCP; the DHCP server SHALL assign addresses in the range 192.168.4.10–192.168.4.250.
2. WHEN firmware calls `esp_wifi_connect()` with valid credentials matching the virtual AP configuration, THE Simulator SHALL complete the association handshake, deliver a `WIFI_EVENT_STA_CONNECTED` event to the firmware within 500 simulated milliseconds, and subsequently deliver an `IP_EVENT_STA_GOT_IP` event with the assigned IP address.
3. IF firmware calls `esp_wifi_connect()` with credentials that do not match the virtual AP configuration, THEN THE Simulator SHALL deliver a `WIFI_EVENT_STA_DISCONNECTED` event with reason code `WIFI_REASON_AUTH_FAIL` to the firmware; IF firmware subsequently calls `esp_wifi_disconnect()`, THEN THE Simulator SHALL also deliver a second `WIFI_EVENT_STA_DISCONNECTED` event for the user-initiated disconnect.
4. WHEN the host machine has internet access, THE Simulator SHALL route TCP/IP packets from the firmware's lwIP stack to the host operating system's network stack, enabling firmware HTTP clients to reach real internet endpoints; IF the host machine has no internet access, THEN THE Simulator SHALL return a connection-refused error to the firmware's TCP/IP stack.
5. THE Simulator SHALL simulate RSSI values between −30 dBm and −90 dBm, configurable by the user in the Simulator control panel, and report the configured value when firmware calls `esp_wifi_sta_get_ap_info()`.
6. THE Simulator SHALL simulate Wi-Fi packet loss by dropping a configurable percentage (0%–100%) of packets transmitted from the firmware toward the host network stack to test firmware retry and error-handling logic.
7. WHEN firmware calls `esp_wifi_disconnect()`, THE Simulator SHALL deliver a `WIFI_EVENT_STA_DISCONNECTED` event and an `IP_EVENT_STA_LOST_IP` event to the firmware within 100 simulated milliseconds.

---

### Requirement 9: Bluetooth Connectivity Simulation

**User Story:** As an embedded developer, I want Bluetooth Low Energy simulation, so that I can test my firmware's BLE GATT server and client behavior without physical hardware.

#### Acceptance Criteria

1. THE Simulator SHALL simulate a BLE radio that firmware can initialize using the standard ESP-IDF Bluetooth controller and Bluedroid stack APIs.
2. WHEN firmware starts BLE advertising, THE Simulator SHALL make the virtual device discoverable to a built-in virtual BLE scanner panel in the Simulator GUI.
3. WHEN a user initiates a connection from the virtual BLE scanner panel to the advertising firmware device, THE Simulator SHALL deliver an `ESP_GATTS_CONNECT_EVT` to the firmware's GATT server callback; THE Simulator SHALL deliver the connect event even if the underlying BLE connection establishment does not fully complete.
4. WHEN the virtual BLE scanner panel sends a GATT read request to a characteristic handle registered by the firmware, THE Simulator SHALL deliver an `ESP_GATTS_READ_EVT` to the firmware's GATT server callback with the correct connection ID and attribute handle.
5. WHEN the virtual BLE scanner panel sends a GATT write request to a characteristic handle registered by the firmware, THE Simulator SHALL deliver an `ESP_GATTS_WRITE_EVT` to the firmware's GATT server callback with the correct connection ID, attribute handle, and written data bytes.
6. THE Simulator SHALL simulate BLE RSSI values between −40 dBm and −80 dBm, configurable per virtual connection in the Simulator GUI.
7. IF firmware attempts to use Classic Bluetooth (BR/EDR) APIs, THEN THE Simulator SHALL log a warning to the Simulator console panel indicating that Classic Bluetooth simulation is not supported and return `ESP_ERR_NOT_SUPPORTED` from the relevant API calls.

---

### Requirement 10: GUI — Pinout Panel

**User Story:** As an embedded developer, I want an interactive visual ESP32 pinout diagram, so that I can see and configure pin states at a glance.

#### Acceptance Criteria

1. THE Pinout_Panel SHALL render a 2D diagram of the ESP32-WROOM-32 module showing all 38 physical pads (including power, GND, and EN) with their physical positions and labels; GPIO34–GPIO39 SHALL be visually marked as input-only with a dashed border.
2. THE Pinout_Panel SHALL color-code each GPIO pin according to its current configured function: green for digital output HIGH, dark gray for digital output LOW, blue for ADC input, orange for PWM output, yellow for I2C, purple for SPI, cyan for UART; unconfigured GPIO pins SHALL display green (HIGH) or dark gray (LOW) based on their current electrical state; floating unconfigured pins with no pull resistor SHALL display dark gray.
3. WHEN firmware changes a GPIO pin's mode or output level, THE Pinout_Panel SHALL update the corresponding pin's color and label within 16 milliseconds (one 60 Hz display frame).
4. WHEN a user clicks a GPIO pin in the Pinout_Panel, THE Simulator SHALL display a tooltip showing the pin number, current mode, current logic level or analog voltage, and either the name of the connected Virtual_Component or "unconnected" if no component is assigned.
5. WHEN a user right-clicks a GPIO pin configured as an input in the Pinout_Panel, THE Simulator SHALL display a context menu with options to manually drive the pin HIGH, manually drive the pin LOW, connect a Virtual_Component, or open the pin's waveform in the Logic_Analyzer; WHEN a user right-clicks a GPIO pin configured as an output, THE Simulator SHALL display a context menu with options to connect a Virtual_Component or open the pin's waveform in the Logic_Analyzer only.
6. THE Pinout_Panel SHALL visually distinguish input-only pins (GPIO34–GPIO39) from bidirectional pins using a dashed border style on the pin rectangle.

---

### Requirement 11: GUI — Workspace and Virtual Components

**User Story:** As an embedded developer, I want to place and connect virtual components on a workspace canvas, so that I can build a virtual circuit that interacts with my firmware.

#### Acceptance Criteria

1. THE Workspace SHALL allow users to drag Virtual_Components from a component library panel and drop them onto a 16×16 pixel grid-aligned canvas.
2. WHEN a Virtual_Component is dropped onto the Workspace, THE Simulator SHALL display a connection dialog prompting the user to assign the component's signal pins to ESP32 GPIO pins; IF the user closes the dialog without completing all pin assignments, THE Simulator SHALL leave unassigned pins unconnected.
3. THE Workspace SHALL render connection wires between Virtual_Component pins and ESP32 GPIO pins with orthogonal routing labeled with the GPIO pin number; incomplete connections (pins not yet assigned to a GPIO) SHALL render as dashed lines.
4. THE Simulator SHALL provide the following built-in Virtual_Components: single-color LED, RGB LED, 16×2 character LCD (HD44780 compatible), 128×64 OLED display (SSD1306 I2C), push button, toggle switch, NTC thermistor (temperature sensor), LDR (light sensor), HC-SR04 ultrasonic distance sensor, and DHT22 temperature/humidity sensor.
5. WHEN the GPIO_Model drives a GPIO pin connected to an LED Virtual_Component HIGH, THE LED Virtual_Component SHALL illuminate; WHEN the GPIO_Model drives the pin LOW, THE LED Virtual_Component SHALL extinguish; the illuminated color and brightness (1–100%) SHALL be configurable in the component's properties panel.
6. WHEN firmware sends HD44780-compatible commands over a GPIO bus connected to a 16×2 LCD Virtual_Component, THE LCD Virtual_Component SHALL render the correct characters in the correct cursor positions within 1 simulated millisecond of the enable pulse.
7. WHEN firmware sends SSD1306 I2C commands to a 128×64 OLED Virtual_Component, THE OLED Virtual_Component SHALL update its pixel buffer and re-render the display within 16 milliseconds.
8. WHEN a user clicks a push button Virtual_Component in the Workspace, THE Workspace SHALL drive the connected GPIO pin to the active logic level configured in the component's properties panel for the duration of the mouse button press and release it to the inactive level on mouse-up.
9. THE Workspace SHALL allow users to configure Virtual_Component parameters (LED color, LED brightness 1–100%, sensor output value within 0.0 to the sensor's defined maximum, button active level) via a properties panel that opens exclusively on double-click of the component.
10. WHEN the user saves the project, THE Workspace SHALL persist the component layout, wiring, and configuration to a JSON project file; IF the save operation fails, THE Simulator SHALL display an error message and leave the in-memory state unchanged.

---

### Requirement 12: Logic Analyzer

**User Story:** As an embedded developer, I want a real-time logic analyzer view, so that I can capture and inspect digital signal waveforms across GPIO pins and protocol buses.

#### Acceptance Criteria

1. THE Logic_Analyzer SHALL capture digital state transitions on up to 34 GPIO channels simultaneously at a minimum sample rate of 10 MHz (100 ns resolution).
2. THE Logic_Analyzer SHALL display captured waveforms as horizontal time-domain traces, one per channel, with a configurable time window of 100 µs to 10 s.
3. WHEN a user configures a trigger condition (rising edge, falling edge, or logic pattern) on a selected channel, THE Logic_Analyzer SHALL begin recording waveform data from the moment the trigger condition is met; the capture buffer SHALL hold up to 1,000,000 samples per channel; IF the trigger condition is not met within 5 seconds of wall-clock time after arming, THE Logic_Analyzer SHALL display a "trigger timeout" notification and disarm.
4. THE Logic_Analyzer SHALL allow users to place two vertical cursor markers on the waveform display and SHALL display the time difference between the cursors; IF the signal between the cursors contains at least 2 complete cycles, THE Logic_Analyzer SHALL display the measured frequency; otherwise THE Logic_Analyzer SHALL display 0 Hz.
5. WHERE the Protocol_Decoder is enabled for an I2C, SPI, or UART channel, WHEN a complete transaction is captured, THE Logic_Analyzer SHALL overlay the decoded packet annotation on the corresponding waveform trace.
6. WHEN the user exports waveform data, THE Logic_Analyzer SHALL write all active channels and the full captured buffer to a VCD file with nanosecond timestamp resolution.
7. WHEN the Simulator is paused, THE Logic_Analyzer SHALL freeze the waveform display at the current simulation time and allow the user to scroll and zoom the captured history with a zoom range of 1 µs/div to 1 s/div.

---

### Requirement 13: Oscilloscope View

**User Story:** As an embedded developer, I want an oscilloscope view for analog signals, so that I can visualize PWM waveforms and ADC input voltages.

#### Acceptance Criteria

1. THE Oscilloscope SHALL display analog voltage waveforms for GPIO pins configured as LEDC PWM output or ADC input, with voltage range 0 V to 3.3 V, at a minimum sample rate of 100,000 samples per second.
2. THE Oscilloscope SHALL provide a configurable timebase from 1 µs/div to 1 s/div and a voltage scale from 0.1 V/div to 3.3 V/div.
3. WHEN the user selects a channel in the Oscilloscope, THE Oscilloscope SHALL calculate and display the following measurements: frequency, period, minimum voltage, maximum voltage, and RMS voltage; for PWM channels, THE Oscilloscope SHALL also display duty cycle; for ADC channels, THE Oscilloscope SHALL display "N/A" for duty cycle.
4. WHEN the PWM duty cycle or frequency is changed by firmware, THE Oscilloscope SHALL update the displayed waveform within 100 simulated milliseconds.
5. THE Oscilloscope SHALL support simultaneous display of up to 4 analog channels with independent vertical scaling and color-coded traces.
6. WHEN the Simulator is paused, THE Oscilloscope SHALL freeze the waveform display at the current simulation time and allow the user to scroll and zoom the captured analog history.

---

### Requirement 14: Debugging Tools

**User Story:** As an embedded developer, I want comprehensive debugging tools, so that I can identify and fix firmware bugs before deploying to physical hardware.

#### Acceptance Criteria

1. THE Debug_Controller SHALL allow users to set software breakpoints at any instruction address; WHEN debug symbols are present in the ELF file, THE Debug_Controller SHALL also allow users to set breakpoints by source file name and line number.
2. WHEN execution reaches a breakpoint address, THE Emulation_Engine SHALL halt execution, notify the Debug_Controller, and transition the Simulator to a paused state within 1 simulated CPU cycle of the breakpoint instruction.
3. WHILE the Simulator is in a paused state, THE Debug_Controller SHALL display the current values of all 64 Xtensa general-purpose registers (AR0–AR63), the program counter, and the window base register.
4. WHILE the Simulator is in a paused state, THE Debug_Controller SHALL allow users to inspect and modify any memory address in the Memory_Model.
5. WHEN the user triggers single-step execution, THE Debug_Controller SHALL advance the Emulation_Engine by exactly one instruction and return to the paused state.
6. WHEN the user triggers step-over execution and the current instruction is a CALL, THE Debug_Controller SHALL advance the Emulation_Engine until the instruction immediately following the CALL is reached; IF the current instruction is not a CALL, THE Debug_Controller SHALL advance by one instruction.
7. WHEN the user triggers step-out execution, THE Debug_Controller SHALL advance the Emulation_Engine until the current function's RETW instruction executes; IF the Emulation_Engine is already at the top-level entry point with no caller frame, THE Debug_Controller SHALL display a notification that step-out is not available.
8. THE Simulator SHALL embed a GDB_Server implementing the GDB remote serial protocol on a configurable TCP port (default 3333); IF the configured port is already in use when the Simulator starts, THE Simulator SHALL display an error message identifying the port conflict and SHALL not start the GDB_Server.
9. WHEN a watched address range is written with a value that differs from its previous value, THE Debug_Controller SHALL pause execution and display a notification showing the address, the previous value, and the new value; THE Debug_Controller SHALL support up to 32 simultaneously active watch entries.
10. WHILE the Simulator is in a paused state and debug symbols are available, THE Debug_Controller SHALL display a call stack trace showing the function name, source file, and line number for each frame; IF debug symbols are not available, THE Debug_Controller SHALL display the raw return address for each frame.

---

### Requirement 15: Simulation Control and Timing

**User Story:** As an embedded developer, I want to control simulation speed and execution flow, so that I can run firmware at different speeds for debugging and testing.

#### Acceptance Criteria

1. THE Simulator SHALL provide a control bar with Run, Pause, Reset, and Step buttons; Run SHALL transition the Emulation_Engine from paused to executing; Pause SHALL transition from executing to paused; Reset SHALL reinitialize the simulation; Step SHALL advance by one instruction from the paused state.
2. THE Simulator SHALL support adjustable simulation speed from 0.1× to 10× real time, configurable via a speed slider in the control bar; the default speed on startup SHALL be 1×.
3. WHEN the Reset button is explicitly activated by the user, THE Simulator SHALL reset the Emulation_Engine program counter to the firmware reset vector, clear all peripheral state, and restore all GPIO pins to their power-on default states (all pins as inputs, pull-up/pull-down disabled, output level LOW) within 100 milliseconds of wall-clock time; IF the program counter reset fails, THE Simulator SHALL still complete the peripheral state clear and GPIO pin restore operations.
4. THE Simulator SHALL display the current simulation time (in microseconds), the simulated CPU frequency, and an estimated power consumption value in the range 0–1000 mW (derived from active peripheral count and CPU load) in the status bar.
5. WHILE the Simulator is running, THE Simulator SHALL update the status bar values at a minimum rate of 4 Hz.
6. THE Event_Scheduler SHALL process all pending timed events in Simulation_Clock order before advancing the Simulation_Clock; IF two events share the same Simulation_Clock timestamp, THE Event_Scheduler SHALL process them in FIFO order of their enqueue time.
7. IF the user clicks Run, Pause, Reset, or Step when no firmware is loaded, THE Simulator SHALL display a notification indicating that no firmware is loaded and SHALL not change the Emulation_Engine state.

---

### Requirement 16: Performance

**User Story:** As an embedded developer, I want the Simulator to run typical IoT firmware at near real-time speed, so that time-sensitive behavior such as Wi-Fi keep-alive and sensor polling is observable.

#### Acceptance Criteria

1. THE Simulator SHALL execute typical ESP32 IoT firmware (FreeRTOS task scheduler, Wi-Fi stack, one I2C sensor, one UART output) at a minimum of 0.5× real-time speed on a host machine with a 4-core CPU at 3.0 GHz or faster and 8 GB RAM; the speed ratio SHALL be measured as simulated time elapsed divided by wall-clock time elapsed over any 10-second wall-clock window.
2. WHILE the Emulation_Engine is running at 1× real-time speed, THE Simulator SHALL maintain a GUI frame rate of at least 30 frames per second sustained over any 5-second wall-clock window.
3. THE Logic_Analyzer SHALL buffer up to 10 seconds of waveform data for up to 16 simultaneously active channels in memory without causing the Simulator to drop below 0.5× real-time execution speed, measured over any 10-second wall-clock window.
4. THE Simulator SHALL use a multi-threaded architecture with at minimum three threads: one for the Emulation_Engine, one for the GUI rendering, and one for peripheral I/O event processing.
5. WHILE the Simulator is running, IF the execution speed drops below 0.5× real-time for more than 3 consecutive seconds, THE Simulator SHALL display a speed-warning indicator in the status bar.

---

### Requirement 17: Build System Integration

**User Story:** As an embedded developer, I want the Simulator to integrate with the existing ESP32 Small OS build system, so that I can build and launch the simulation from the same project.

#### Acceptance Criteria

1. THE Simulator SHALL be buildable as a separate CMake target (`esp32_simulator`) within the existing project's CMakeLists.txt; building the `esp32_simulator` target SHALL NOT modify, rebuild, or invalidate any existing firmware build targets.
2. WHEN the `SIMULATION` preprocessor macro is defined, THE HAL SHALL route all `gpio_hal_*` function calls to the GPIO_Model; IF the GPIO_Model is not initialized when a `gpio_hal_*` call is made, THE Simulator SHALL log an error message identifying the uninitialized GPIO_Model and terminate the simulation with a non-zero exit code.
3. THE Simulator build SHALL require Qt 6.x with the Widgets and Charts modules and SHALL produce a self-contained executable (statically linked or with bundled Qt libraries) for Windows (x64), macOS (arm64 and x86_64), and Linux (x86_64).
4. WHEN the Simulator starts, THE Simulator SHALL load `sim/sim_runner.c` stub implementations as a compatibility shim; as each Peripheral_Controller model is implemented, THE Simulator SHALL use the full model in place of the corresponding stub without requiring changes to the stub file.
5. WHEN the `SIMULATION` macro is defined, THE Simulator SHALL intercept all `ESP_LOGI`, `ESP_LOGE`, `ESP_LOGW`, and `ESP_LOGD` macro calls and append the formatted log message to the Simulator's console panel instead of writing to the host standard output.

---

### Requirement 18: Data Export and Import

**User Story:** As an embedded developer, I want to export simulation data and import test vectors, so that I can compare simulation results with hardware measurements and automate regression testing.

#### Acceptance Criteria

1. WHEN the user exports waveform data from the Logic_Analyzer, THE Logic_Analyzer SHALL write all active channels and the full captured buffer to a VCD file with nanosecond timestamp resolution.
2. WHEN the user imports a VCD file as stimulus input, THE Simulator SHALL parse the VCD file and drive GPIO pin states according to the value changes in the VCD file, replaying them at the timestamps specified in the file.
3. IF the imported VCD file contains syntax errors or references signal names that do not match any GPIO pin, THE Simulator SHALL display an error message identifying the first offending line and SHALL not begin stimulus replay.
4. FOR ALL VCD files exported by the Logic_Analyzer, importing the same VCD file as stimulus and re-running the simulation SHALL produce GPIO output waveforms that match the original capture within ±100 ns for all output pins that were not driven by the stimulus.
5. WHEN the user exports the simulation session, THE Simulator SHALL write the firmware binary path, component layout, wiring, and Virtual_Component configurations to a JSON project file.
6. WHEN the user imports a JSON project file, THE Simulator SHALL restore the component layout, wiring, and Virtual_Component configurations and reload the firmware binary from the path stored in the file; IF the firmware binary is not found at the stored path, THE Simulator SHALL display an error message identifying the missing file and SHALL restore all other session state.
