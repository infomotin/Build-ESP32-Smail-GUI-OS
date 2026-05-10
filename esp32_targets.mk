# ESP32 Small OS Target Configurations
# Defines build configurations for different ESP32 variants

# ESP32 (Original)
ESP32_CONFIG := \
	CONFIG_IDF_TARGET_ESP32=y \
	CONFIG_ESP32_DEFAULT_CPU_FREQ_240=y \
	CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ=240 \
	CONFIG_ESP32_TRACEMEM_RESERVE_DRAM=0x0 \
	CONFIG_ESP32_ULP_COPROC_RESERVE_MEM=0

# ESP32-S2
ESP32S2_CONFIG := \
	CONFIG_IDF_TARGET_ESP32S2=y \
	CONFIG_ESP32S2_DEFAULT_CPU_FREQ_240=y \
	CONFIG_ESP32S2_DEFAULT_CPU_FREQ_MHZ=240 \
	CONFIG_ESP32S2_ULP_COPROC_RESERVE_MEM=0

# ESP32-S3
ESP32S3_CONFIG := \
	CONFIG_IDF_TARGET_ESP32S3=y \
	CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240=y \
	CONFIG_ESP32S3_DEFAULT_CPU_FREQ_MHZ=240 \
	CONFIG_ESP32S3_ULP_COPROC_RESERVE_MEM=0

# ESP32-C3
ESP32C3_CONFIG := \
	CONFIG_IDF_TARGET_ESP32C3=y \
	CONFIG_ESP32C3_DEFAULT_CPU_FREQ_160=y \
	CONFIG_ESP32C3_DEFAULT_CPU_FREQ_MHZ=160

# Common configurations for all targets
COMMON_CONFIG := \
	CONFIG_PARTITION_TABLE_SINGLE_APP=y \
	CONFIG_PARTITION_TABLE_FILENAME="partitions.csv" \
	CONFIG_PARTITION_TABLE_OFFSET=0x8000 \
	CONFIG_BOOTLOADER_COMPILER_OPTIMIZATION_SIZE=y \
	CONFIG_BOOTLOADER_LOG_LEVEL_INFO=y \
	CONFIG_LOG_DEFAULT_LEVEL_INFO=y \
	CONFIG_LOG_MAXIMUM_LEVEL=3 \
	CONFIG_ESP_TIMER_IMPL_FRC2=y \
	CONFIG_ESP_WIFI_ENABLED=y \
	CONFIG_ESP_WIFI_SW_COEXIST_ENABLE=y \
	CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM=10 \
	CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM=32 \
	CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=32 \
	CONFIG_FREERTOS_HZ=1000 \
	CONFIG_FREERTOS_IDLE_TASK_STACKSIZE=4096 \
	CONFIG_SPI_FLASH_SIZE_4MB=y \
	CONFIG_SPIRAM=y \
	CONFIG_SPIRAM_USE_MALLOC=y

# Help function
define HELP_TEXT
ESP32 Small OS Target Configurations

Usage: make target[TARGET_NAME]

Available targets:
  esp32      - Original ESP32 (default)
  esp32s2    - ESP32-S2 with USB support
  esp32s3    - ESP32-S3 with dual core
  esp32c3    - ESP32-C3 RISC-V single core

Examples:
  make targetesp32      # Build for ESP32
  make targetesp32s3    # Build for ESP32-S3
  make clean targetesp32s2  # Clean and build for ESP32-S2

Note: After setting target, run 'idf.py build' to build the firmware.
endef

# Target configuration rules
.PHONY: help targetesp32 targetesp32s2 targetesp32s3 targetesp32c3

help:
	@echo "$$HELP_TEXT"

targetesp32:
	@echo "Configuring for ESP32..."
	@mkdir -p build
	@echo "$(ESP32_CONFIG)" > build/sdkconfig
	@echo "$(COMMON_CONFIG)" >> build/sdkconfig
	@echo "ESP32 configuration applied. Run 'idf.py build' to build."

targetesp32s2:
	@echo "Configuring for ESP32-S2..."
	@mkdir -p build
	@echo "$(ESP32S2_CONFIG)" > build/sdkconfig
	@echo "$(COMMON_CONFIG)" >> build/sdkconfig
	@echo "ESP32-S2 configuration applied. Run 'idf.py build' to build."

targetesp32s3:
	@echo "Configuring for ESP32-S3..."
	@mkdir -p build
	@echo "$(ESP32S3_CONFIG)" > build/sdkconfig
	@echo "$(COMMON_CONFIG)" >> build/sdkconfig
	@echo "ESP32-S3 configuration applied. Run 'idf.py build' to build."

targetesp32c3:
	@echo "Configuring for ESP32-C3..."
	@mkdir -p build
	@echo "$(ESP32C3_CONFIG)" > build/sdkconfig
	@echo "$(COMMON_CONFIG)" >> build/sdkconfig
	@echo "ESP32-C3 configuration applied. Run 'idf.py build' to build."
