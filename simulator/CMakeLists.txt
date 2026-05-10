cmake_minimum_required(VERSION 3.20)

# Project definition
project(esp32_simulator VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# Enable Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Compiler flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/iss
    ${CMAKE_CURRENT_SOURCE_DIR}/core/memory
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scheduler
    ${CMAKE_CURRENT_SOURCE_DIR}/core/elf_loader
    ${CMAKE_CURRENT_SOURCE_DIR}/peripherals
    ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/gpio
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
)

# Core library sources
set(CORE_SOURCES
    core/iss/xtensa_iss.cpp
    core/memory/memory_model.cpp
    core/scheduler/event_scheduler.cpp
    core/elf_loader/elf_loader.cpp
)

# Peripheral sources
set(PERIPHERAL_SOURCES
    peripherals/gpio/gpio_model.cpp
)

# GUI sources
set(GUI_SOURCES
    gui/main_window.cpp
    gui/pinout_panel.cpp
    gui/workspace_widget.cpp
    gui/simulation_control_widget.cpp
    gui/status_widget.cpp
    gui/logic_analyzer_widget.cpp
)

# Main application source
set(MAIN_SOURCES
    main.cpp
)

# All sources
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${PERIPHERAL_SOURCES}
    ${GUI_SOURCES}
    ${MAIN_SOURCES}
)

# Create executable
add_executable(esp32_simulator ${ALL_SOURCES})

# Link Qt6 libraries
target_link_libraries(esp32_simulator
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    set_target_properties(esp32_simulator PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Installation
install(TARGETS esp32_simulator
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Create a simple test firmware for demonstration
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_firmware.elf
    COMMAND ${CMAKE_COMMAND} -E echo "Creating test firmware placeholder"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../sim/simple_test.c
        ${CMAKE_CURRENT_BINARY_DIR}/test_firmware.c
    COMMENT "Creating test firmware"
    VERBATIM
)

# Add custom target for test firmware
add_custom_target(test_firmware
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/test_firmware.elf
    COMMENT "Test firmware available"
)

# Package configuration
include(CPack)
set(CPACK_PACKAGE_NAME "ESP32 Virtual Hardware Simulator")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ESP32 Virtual Hardware Simulation Environment")
set(CPACK_PACKAGE_VENDOR "ESP32 Small OS Project")
set(CPACK_PACKAGE_CONTACT "support@esp32-small-os.org")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "ESP32 Virtual Hardware Simulator")
    set(CPACK_NSIS_PACKAGE_NAME "ESP32Simulator")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ESP32 Small OS Project")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6widgets6, libqt6charts6")
endif()
