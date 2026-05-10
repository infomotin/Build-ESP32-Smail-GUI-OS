# ESP32 Small OS Simulation Makefile
# Compiles the Small OS for PC simulation

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -DSIMULATION
LDFLAGS = -lpthread -lm

# Directories
SIM_DIR = sim
SRC_DIR = .
BUILD_DIR = build

# Source files
SIM_SOURCES = $(SIM_DIR)/sim_runner.c $(SIM_DIR)/sim_main.c
OS_SOURCES = \
	$(SRC_DIR)/scheduler/task_scheduler.c \
	$(SRC_DIR)/gpio/gpio_hal.c \
	$(SRC_DIR)/interrupt/interrupt_handler.c \
	$(SRC_DIR)/http/http_server.c \
	$(SRC_DIR)/memory/memory_pool.c \
	$(SRC_DIR)/network/wifi_manager.c \
	$(SRC_DIR)/storage/nvs_storage.c \
	$(SRC_DIR)/utils/logger.c \
	$(SRC_DIR)/security/auth.c

# Object files
SIM_OBJECTS = $(SIM_SOURCES:$(SIM_DIR)/%.c=$(BUILD_DIR)/%.o)
OS_OBJECTS = $(OS_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
ALL_OBJECTS = $(SIM_OBJECTS) $(OS_OBJECTS)

# Target executable
TARGET = esp32_small_os_sim

# Include directories
INCLUDES = -I$(SIM_DIR) -I$(SRC_DIR) -I$(SRC_DIR)/bootloader -I$(SRC_DIR)/scheduler \
          -I$(SRC_DIR)/gpio -I$(SRC_DIR)/interrupt -I$(SRC_DIR)/http -I$(SRC_DIR)/memory \
          -I$(SRC_DIR)/network -I$(SRC_DIR)/storage -I$(SRC_DIR)/utils -I$(SRC_DIR)/security

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/main
	mkdir -p $(BUILD_DIR)/scheduler
	mkdir -p $(BUILD_DIR)/gpio
	mkdir -p $(BUILD_DIR)/interrupt
	mkdir -p $(BUILD_DIR)/http
	mkdir -p $(BUILD_DIR)/memory
	mkdir -p $(BUILD_DIR)/network
	mkdir -p $(BUILD_DIR)/storage
	mkdir -p $(BUILD_DIR)/utils
	mkdir -p $(BUILD_DIR)/security

# Compile simulation runner
$(BUILD_DIR)/%.o: $(SIM_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile OS source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link executable
$(TARGET): $(ALL_OBJECTS)
	$(CC) $(ALL_OBJECTS) $(LDFLAGS) -o $@
	@echo "Simulation built successfully: $(TARGET)"

# Clean build
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	@echo "Build cleaned"

# Run simulation
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y build-essential libcjson-dev

# Help
help:
	@echo "Available targets:"
	@echo "  all          - Build the simulation"
	@echo "  clean        - Clean build files"
	@echo "  run          - Build and run simulation"
	@echo "  debug        - Build with debug symbols"
	@echo "  install-deps - Install required dependencies"
	@echo "  help         - Show this help message"

.PHONY: all clean run debug install-deps help
