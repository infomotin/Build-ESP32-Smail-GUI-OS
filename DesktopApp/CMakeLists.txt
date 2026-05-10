cmake_minimum_required(VERSION 3.20)

# ESP32 Virtual Hardware Simulator Desktop Application
project(esp32_virtual_simulator VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts OpenGLWidgets)
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../simulator/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../simulator/core/iss
    ${CMAKE_CURRENT_SOURCE_DIR}/../simulator/core/memory
    ${CMAKE_CURRENT_SOURCE_DIR}/../simulator/core/scheduler
    ${CMAKE_CURRENT_SOURCE_DIR}/../simulator/core/elf_loader
)

# Source files - Core
set(CORE_SOURCES
    src/core/main.cpp
    src/core/application.cpp
    src/core/simulation_engine.cpp
    src/core/elf_loader_wrapper.cpp
    src/core/config_manager.cpp
    src/core/utils.cpp
)

# Source files - GUI
set(GUI_SOURCES
    src/gui/main_window.cpp
    src/gui/pinout_panel.cpp
    src/gui/workspace.cpp
    src/gui/component_library.cpp
    src/gui/properties_panel.cpp
    src/gui/console_panel.cpp
    src/gui/logic_analyzer.cpp
    src/gui/oscilloscope.cpp
    src/gui/debug_panel.cpp
    src/gui/register_view.cpp
    src/gui/memory_view.cpp
    src/gui/breakpoint_panel.cpp
    src/gui/waveform_view.cpp
    src/gui/protocol_decoder.cpp
    src/gui/graphics_utils.cpp
)

# Source files - Peripherals
set(PERIPHERAL_SOURCES
    src/peripherals/peripheral_manager.cpp
    src/peripherals/gpio_controller.cpp
    src/peripherals/i2c_controller.cpp
    src/peripherals/spi_controller.cpp
    src/peripherals/uart_controller.cpp
    src/peripherals/adc_controller.cpp
    src/peripherals/dac_controller.cpp
    src/peripherals/ledc_controller.cpp
    src/peripherals/timer_controller.cpp
)

# Source files - Virtual Components
set(VC_SOURCES
    src/virtual_components/component_base.cpp
    src/virtual_components/led_component.cpp
    src/virtual_components/rgb_led_component.cpp
    src/virtual_components/lcd_component.cpp
    src/virtual_components/oled_component.cpp
    src/virtual_components/button_component.cpp
    src/virtual_components/toggle_switch.cpp
    src/virtual_components/ntc_thermistor.cpp
    src/virtual_components/ldr_sensor.cpp
    src/virtual_components/hc_sr04.cpp
    src/virtual_components/dht22.cpp
    src/virtual_components/sensor_base.cpp
)

# Source files - Debug
set(DEBUG_SOURCES
    src/debug/debug_controller.cpp
    src/debug/gdb_server.cpp
    src/debug/breakpoint_manager.cpp
    src/debug/watchpoint_manager.cpp
    src/debug/call_stack.cpp
)

# Source files - Network
set(NETWORK_SOURCES
    src/network/wifi_simulator.cpp
    src/network/ble_simulator.cpp
    src/network/tcpip_stack.cpp
    src/network/virtual_ap.cpp
)

# Header files
set(HEADERS
    include/application.h
    include/simulation_engine.h
    include/elf_loader_wrapper.h
    include/config_manager.h
    include/utils.h
    include/gui/main_window.h
    include/gui/pinout_panel.h
    include/gui/workspace.h
    include/gui/component_library.h
    include/gui/properties_panel.h
    include/gui/console_panel.h
    include/gui/logic_analyzer.h
    include/gui/oscilloscope.h
    include/gui/debug_panel.h
    include/gui/register_view.h
    include/gui/memory_view.h
    include/gui/breakpoint_panel.h
    include/gui/waveform_view.h
    include/gui/protocol_decoder.h
    include/gui/graphics_utils.h
    include/peripherals/peripheral_manager.h
    include/peripherals/gpio_controller.h
    include/peripherals/i2c_controller.h
    include/peripherals/spi_controller.h
    include/peripherals/uart_controller.h
    include/peripherals/adc_controller.h
    include/peripherals/dac_controller.h
    include/peripherals/ledc_controller.h
    include/peripherals/timer_controller.h
    include/virtual_components/component_base.h
    include/virtual_components/led_component.h
    include/virtual_components/rgb_led_component.h
    include/virtual_components/lcd_component.h
    include/virtual_components/oled_component.h
    include/virtual_components/button_component.h
    include/virtual_components/toggle_switch.h
    include/virtual_components/ntc_thermistor.h
    include/virtual_components/ldr_sensor.h
    include/virtual_components/hc_sr04.h
    include/virtual_components/dht22.h
    include/virtual_components/sensor_base.h
    include/debug/debug_controller.h
    include/debug/gdb_server.h
    include/debug/breakpoint_manager.h
    include/debug/watchpoint_manager.h
    include/debug/call_stack.h
    include/network/wifi_simulator.h
    include/network/ble_simulator.h
    include/network/tcpip_stack.h
    include/network/virtual_ap.h
)

# UI files (Qt Designer forms)
set(UI_FILES
    resources/main_window.ui
    resources/component_properties.ui
    resources/debug_panel.ui
)

# Resource files
set(QRC_FILES
    resources/resources.qrc
)

# Process Qt files
qt6_wrap_ui(UI_HEADERS ${UI_FILES})
qt6_add_resources(QRC_SOURCES ${QRC_FILES})

# Create executable
add_executable(${PROJECT_NAME}
    ${CORE_SOURCES}
    ${GUI_SOURCES}
    ${PERIPHERAL_SOURCES}
    ${VC_SOURCES}
    ${DEBUG_SOURCES}
    ${NETWORK_SOURCES}
    ${HEADERS}
    ${UI_HEADERS}
    ${QRC_SOURCES}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
    Qt6::OpenGLWidgets
    Threads::Threads
    ${CMAKE_SOURCE_DIR}/simulator/libsimulator.a
)

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Add compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    SIMULATION=1
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
)

# Installation
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# Create simulator library from existing sources
add_library(simulator STATIC
    ../simulator/core/iss/xtensa_iss.cpp
    ../simulator/core/memory/memory_model.cpp
    ../simulator/core/scheduler/event_scheduler.cpp
    ../simulator/core/elf_loader/elf_loader.cpp
)

target_include_directories(simulator PUBLIC
    ../simulator/core
    ../simulator/core/iss
    ../simulator/core/memory
    ../simulator/core/scheduler
    ../simulator/core/elf_loader
)

target_link_libraries(simulator
    Qt6::Core
    Threads::Threads
)

set_target_properties(simulator PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "simulator"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)
