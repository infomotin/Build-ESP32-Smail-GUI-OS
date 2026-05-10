/**
 * @file i2c_controller.h
 * @brief I2C peripheral simulation controller
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>

#include "simulator/core/memory/memory_model.h"

namespace esp32sim {

/**
 * @enum I2CMode
 * @brief I2C controller modes
 */
enum class I2CMode {
    MASTER = 0,
    SLAVE = 1,
    MASTER_SLAVE = 2  // Simultaneous master and slave
};

/**
 * @enum I2CClockSpeed
 * @brief I2C clock speeds
 */
enum class I2CClockSpeed {
    SCL_100K = 100000,
    SCL_80K = 80000,
    SCL_400K = 400000,
    SCL_500K = 500000,
    SCL_1M = 1000000
};

/**
 * @struct I2CTransaction
 * @brief Represents an ongoing I2C transaction
 */
struct I2CTransaction {
    bool is_write = false;           // true = write, false = read
    bool start_condition = false;
    bool stop_condition = false;
    uint8_t slave_address = 0;
    std::vector<uint8_t> data;
    size_t data_index = 0;
    int ack_received = -1;          // -1 = none, 0 = NACK, 1 = ACK
};

/**
 * @struct I2CSlaveDevice
 * @brief Simulated I2C slave device
 */
struct I2CSlaveDevice {
    uint8_t address = 0;
    std::string name;
    std::vector<uint8_t> tx_buffer;  // Data to send to master (read)
    std::vector<uint8_t> rx_buffer;  // Data received from master (write)
    std::function<void(const std::vector<uint8_t>&)> on_data_received;
    std::function<std::vector<uint8_t>()> on_data_requested;
    bool transaction_active = false;
};

/**
 * @class I2CController
 * @brief Simulates ESP32 I2C controller (I2C0 or I2C1)
 *
 * Features:
 * - Master and slave mode simulation
 * - Accurate timing (100kHz-1MHz)
 * - Clock stretching support
 * - ACK/NACK handling
 * - Multiple slave device support
 * - Protocol decoding integration
 */
class I2CController {
public:
    I2CController(uint8_t bus_id, MemoryModel* memory);
    ~I2CController();

    /**
     * @brief Initialize the I2C controller
     */
    bool initialize();

    /**
     * @brief Reset controller state
     */
    void reset();

    /**
     * @brief Set I2C mode (master/slave)
     */
    void setMode(I2CMode mode);

    /**
     * @brief Configure clock speed
     */
    void setClockSpeed(I2CClockSpeed speed);

    /**
     * @brief Start I2C bus
     */
    void start();

    /**
     * @brief Stop I2C bus
     */
    void stop();

    /**
     * @brief Master write transaction
     * @param slave_addr 7-bit slave address
     * @param data Data bytes to send
     * @param timeout_ms Timeout in milliseconds
     * @return ESP_OK on success
     */
    int masterWrite(uint8_t slave_addr, const std::vector<uint8_t>& data, uint32_t timeout_ms = 1000);

    /**
     * @brief Master read transaction
     * @param slave_addr 7-bit slave address
     * @param data Buffer to receive data
     * @param num_bytes Number of bytes to read
     * @param timeout_ms Timeout
     * @return ESP_OK on success
     */
    int masterRead(uint8_t slave_addr, std::vector<uint8_t>& data, uint32_t timeout_ms = 1000);

    /**
     * @brief Master write read transaction
     */
    int masterWriteRead(uint8_t slave_addr,
                       const std::vector<uint8_t>& write_data,
                       std::vector<uint8_t>& read_data,
                       uint32_t timeout_ms = 1000);

    /**
     * @brief Register a slave device
     */
    void registerSlaveDevice(uint8_t address, std::shared_ptr<I2CSlaveDevice> device);

    /**
     * @brief Unregister a slave device
     */
    void unregisterSlaveDevice(uint8_t address);

    /**
     * @brief Get current bus state
     */
    bool isBusBusy() const { return transaction_in_progress_; }

    /**
     * @brief Simulate one tick (advance timing)
     */
    void tick(uint64_t elapsed_ns);

    /**
     * @brief Map memory-mapped registers
     */
    void mapMemoryRegions();

    /**
     * @brief Read from memory-mapped register
     */
    uint32_t readRegister(uint32_t address);

    /**
     * @brief Write to memory-mapped register
     */
    void writeRegister(uint32_t address, uint32_t value);

    /**
     * @brief Get transaction statistics
     */
    struct Stats {
        uint64_t master_transactions = 0;
        uint64_t bytes_written = 0;
        uint64_t bytes_read = 0;
        uint64_t nack_errors = 0;
        uint64_t timeout_errors = 0;
        uint64_t clock_stretch_events = 0;
    };
    const Stats& stats() const { return stats_; }

    /**
     * @brief Get decoded protocol data
     */
    struct ProtocolFrame {
        uint64_t timestamp;
        bool is_start;
        bool is_stop;
        bool is_ack;
        uint8_t address;
        bool is_write;
        uint8_t data_byte;
    };
    const std::vector<ProtocolFrame>& protocolFrames() const { return protocol_frames_; }
    void clearProtocolFrames() { protocol_frames_.clear(); }

signals:
    /**
     * @brief Transaction completed
     */
    void transactionComplete(bool success, const std::string& error = "");

private:
    uint8_t bus_id_;
    MemoryModel* memory_ = nullptr;
    I2CMode mode_ = I2CMode::MASTER;
    I2CClockSpeed clock_speed_ = I2CClockSpeed::SCL_100K;

    // Bus state
    std::atomic<bool> sda_level_{true};   // SDA line (idle high)
    std::atomic<bool> scl_level_{true};   // SCL line (idle high)
    bool transaction_in_progress_ = false;
    I2CTransaction current_transaction_;

    // Timing
    uint64_t bit_period_ns_ = 10000;  // 100kHz default = 10us per bit
    uint64_t half_period_ns_ = 5000;
    uint64_t next_tick_time_ = 0;
    uint64_t elapsed_ = 0;

    // Clock stretching
    bool clock_stretching_ = false;
    uint64_t stretch_until_ = 0;

    // Devices
    std::unordered_map<uint8_t, std::shared_ptr<I2CSlaveDevice>> slave_devices_;

    // Statistics
    Stats stats_;
    std::vector<ProtocolFrame> protocol_frames_;

    // Internal methods
    void startTransaction(bool write, uint8_t slave_addr);
    void sendBit(bool bit);
    bool receiveBit();
    void sendByte(uint8_t byte, bool& ack);
    uint8_t receiveByte(bool& ack);
    void generateStartCondition();
    void generateStopCondition();
    void handleSlaveResponse();
    void processClockStretching();

    // Utility
    uint64_t nanosecondsPerBit() const { return bit_period_ns_; }
};

/**
 * @class I2CSlaveDeviceBase
 * @brief Base class for implementing I2C slave devices
 */
class I2CSlaveDeviceBase : public I2CSlaveDevice {
public:
    I2CSlaveDeviceBase(uint8_t address, const std::string& name)
        : address(address), name(name) {}

    virtual ~I2CSlaveDeviceBase() = default;

    /**
     * @brief Called when master writes data
     */
    virtual void onWrite(const std::vector<uint8_t>& data) = 0;

    /**
     * @brief Called when master reads data
     * @return Data to send
     */
    virtual std::vector<uint8_t> onRead() = 0;

    /**
     * @brief Process received command
     */
    void processWrite(const std::vector<uint8_t>& data) {
        rx_buffer_.insert(rx_buffer_.end(), data.begin(), data.end());
        onWrite(data);
    }

    std::vector<uint8_t> processRead() {
        tx_buffer_ = onRead();
        return tx_buffer_;
    }

protected:
    std::vector<uint8_t> rx_buffer_;
    std::vector<uint8_t> tx_buffer_;
};

} // namespace esp32sim
