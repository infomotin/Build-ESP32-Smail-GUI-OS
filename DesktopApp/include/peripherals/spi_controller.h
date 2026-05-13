/**
 * @file spi_controller.h
 * @brief SPI peripheral simulation controller
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <array>
#include <atomic>
#include <mutex>
#include <QObject>

#include "simulator/core/memory/memory_model.h"

namespace esp32sim {

/**
 * @enum SPIMode
 * @brief SPI mode (CPOL/CPHA)
 */
enum class SPIMode {
    MODE0 = 0,  // CPOL=0, CPHA=0
    MODE1 = 1,  // CPOL=0, CPHA=1
    MODE2 = 2,  // CPOL=1, CPHA=0
    MODE3 = 3   // CPOL=1, CPHA=1
};

/**
 * @enum SPITransferDirection
 * @brief Transfer direction
 */
enum class SPITransferDirection {
    TX_ONLY = 0,
    RX_ONLY = 1,
    TX_RX = 2
};

/**
 * @struct SPISlaveDevice
 * @brief Simulated SPI slave device info
 */
struct SPISlaveDevice {
    uint8_t cs_pin = 0;
    std::string name;
    std::vector<uint8_t> rx_buffer;
    std::vector<uint8_t> tx_buffer;
    std::function<void(const std::vector<uint8_t>&)> on_receive;
    std::function<std::vector<uint8_t>(size_t)> on_transmit;
};

/**
 * @class SPIController
 * @brief Simulates ESP32 SPI controller
 *
 * Supports SPI0, SPI1 (HSPI), SPI2 (VSPI)
 * - Full-duplex communication
 * - DMA simulation
 * - All 4 SPI modes
 * - Configurable bit rate
 * - CS line control
 */
class SPIController : public QObject {
    Q_OBJECT
public:
    explicit SPIController(uint8_t bus_id, MemoryModel* memory, QObject* parent = nullptr);
    ~SPIController();

    /**
     * @brief Initialize the SPI controller
     */
    bool initialize();

    /**
     * @brief Reset controller
     */
    void reset();

    /**
     * @brief Configure SPI parameters
     */
    void configure(uint32_t baudrate_hz, SPIMode mode, uint8_t data_bits = 8,
                   SPITransferDirection direction = SPITransferDirection::TX_RX);

    /**
     * @brief Perform a synchronous transfer (master mode)
     */
    std::vector<uint8_t> transfer(const std::vector<uint8_t>& tx_data, uint32_t timeout_ms = 1000);

    /**
     * @brief Transfer with separate TX and RX buffers
     */
    void transferFullDuplex(const uint8_t* tx_data, uint8_t* rx_data, size_t len, uint32_t timeout_ms = 1000);

    /**
     * @brief Set chip select pin
     */
    void setCSPin(uint8_t cs_pin);

    /**
     * @brief Get chip select pin
     */
    uint8_t csPin() const { return cs_pin_; }

    /**
     * @brief Register a slave device
     */
    void registerSlaveDevice(std::shared_ptr<SPISlaveDevice> device);

    /**
     * @brief Unregister a slave device
     */
    void unregisterSlaveDevice(uint8_t cs_pin);

    /**
     * @brief Simulate one tick (bit banging)
     */
    void tick(uint64_t elapsed_ns);

    /**
     * @brief Map memory-mapped registers
     */
    void mapMemoryRegions();

    /**
     * @brief Read register
     */
    uint32_t readRegister(uint32_t address);

    /**
     * @brief Write register
     */
    void writeRegister(uint32_t address, uint32_t value);

    /**
     * @brief Get statistics
     */
    struct Stats {
        uint64_t transfers = 0;
        uint64_t bytes_transferred = 0;
        uint64_t overrun_errors = 0;
        uint64_t mode_faults = 0;
    };
    const Stats& stats() const { return stats_; }

    /**
     * @brief Get current MISO line state
     */
    bool getMISO() const { return miso_level_; }

    /**
     * @brief Set MISO line state (for slave devices)
     */
    void setMISO(bool level) { miso_level_ = level; }

    /**
     * @brief Get MOSI line state
     */
    bool getMOSI() const { return mosi_level_; }

    /**
     * @brief Get SCLK line state
     */
    bool getSCLK() const { return sclk_level_; }

    /**
     * @brief Get CS line state
     */
    bool getCS() const { return cs_level_; }

signals:
    /**
     * @brief Transfer completed
     */
    void transferComplete(const std::vector<uint8_t>& data, bool success);

private:
    uint8_t bus_id_;
    MemoryModel* memory_ = nullptr;

    // Configuration
    uint32_t baudrate_hz_ = 1000000;  // 1 MHz default
    SPIMode mode_ = SPIMode::MODE0;
    uint8_t data_bits_ = 8;
    SPITransferDirection direction_ = SPITransferDirection::TX_RX;

    // Pin state
    uint8_t cs_pin_ = 0;
    std::atomic<bool> sclk_level_{false};
    std::atomic<bool> mosi_level_{false};
    std::atomic<bool> miso_level_{false};
    std::atomic<bool> cs_level_{true};  // CS active low

    // Transfer state
    bool transfer_active_ = false;
    std::vector<uint8_t> tx_buffer_;
    std::vector<uint8_t> rx_buffer_;
    size_t tx_index_ = 0;
    size_t rx_index_ = 0;
    size_t bit_count_ = 0;
    uint8_t current_tx_byte_ = 0;
    uint8_t current_rx_byte_ = 0;

    // Bit timing
    uint64_t bit_period_ns_ = 0;
    uint64_t half_period_ns_ = 0;
    uint64_t next_edge_time_ = 0;
    bool sclk_next_high_ = true;

    // Devices
    std::vector<std::shared_ptr<SPISlaveDevice>> slave_devices_;

    // DMA simulation
    struct DMATransfer {
        uint32_t dma_channel = 0;
        size_t remaining_bytes = 0;
        std::function<void()> callback;
    };
    std::unique_ptr<DMATransfer> dma_transfer_;

    // Statistics
    Stats stats_;

    // Internal methods
    void beginTransfer();
    void endTransfer();
    void clockEdge();
    void sampleMOSI();
    void updateMISO();
    void processDMA();

    // Utility
    uint64_t bitTime() const { return bit_period_ns_; }
};

} // namespace esp32sim
