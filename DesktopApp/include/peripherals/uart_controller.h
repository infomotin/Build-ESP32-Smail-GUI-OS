/**
 * @file uart_controller.h
 * @brief UART peripheral simulation controller
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <array>
#include <queue>
#include <atomic>
#include <mutex>
#include <deque>
#include <QObject>

#include "simulator/core/memory/memory_model.h"

namespace esp32sim {

/**
 * @enum UARTParity
 * @brief UART parity modes
 */
enum class UARTParity {
    NONE = 0,
    EVEN = 1,
    ODD = 2
};

/**
 * @enum UARTStopBits
 * @brief UART stop bit configuration
 */
enum class UARTStopBits {
    STOP_BITS_1 = 1,
    STOP_BITS_1_5 = 2,
    STOP_BITS_2 = 3
};

/**
 * @struct UARTFrame
 * @brief Decoded UART frame
 */
struct UARTFrame {
    uint64_t timestamp;       // Simulation time when frame started
    bool start_bit;
    uint8_t data_bits[8];     // Up to 8 data bits
    uint8_t data_bits_count;
    bool parity_bit;
    bool parity_ok;
    bool stop_bit;
    uint8_t decoded_byte;     // Assembled byte
};

/**
 * @class UARTController
 * @brief Simulates ESP32 UART controller
 *
 * Features:
 * - Configurable baud rate (300 bps to 5 Mbps)
 * - 5-8 data bits, 1/1.5/2 stop bits
 * - Even/odd/no parity
 * - TX and RX FIFO buffers (128 bytes)
 * - Hardware flow control (RTS/CTS)
 * - Break detection
 */
class UARTController : public QObject {
    Q_OBJECT
public:
    explicit UARTController(uint8_t port, MemoryModel* memory, QObject* parent = nullptr);
    ~UARTController();

    /**
     * @brief Initialize the UART controller
     */
    bool initialize();

    /**
     * @brief Reset controller
     */
    void reset();

    /**
     * @brief Configure UART parameters
     * @param baudrate Baud rate in bits per second
     * @param data_bits Number of data bits (5-8)
     * @param stop_bits Number of stop bits
     * @param parity Parity mode
     * @param flow_control Enable hardware flow control
     */
    void configure(uint32_t baudrate, uint8_t data_bits = 8,
                   UARTStopBits stop_bits = UARTStopBits::STOP_BITS_1,
                   UARTParity parity = UARTParity::NONE,
                   bool flow_control = false);

    /**
     * @brief Write byte to TX FIFO
     * @param byte Byte to transmit
     * @return ESP_OK on success, ESP_FAIL if FIFO full
     */
    int writeByte(uint8_t byte);

    /**
     * @brief Read byte from RX FIFO
     * @param byte Pointer to receive byte
     * @return ESP_OK on success, ESP_ERR_NOT_FOUND if FIFO empty
     */
    int readByte(uint8_t* byte);

    /**
     * @brief Check if TX FIFO is empty
     */
    bool isTXEmpty() const { return tx_fifo_.empty(); }

    /**
     * @brief Check if TX FIFO is full
     */
    bool isTXFull() const { return tx_fifo_.size() >= tx_fifo_size_; }

    /**
     * @brief Check if RX FIFO is empty
     */
    bool isRXEmpty() const { return rx_fifo_.empty(); }

    /**
     * @brief Check if RX FIFO is full
     */
    bool isRXFull() const { return rx_fifo_.size() >= rx_fifo_size_; }

    /**
     * @brief Get number of bytes in TX FIFO
     */
    size_t txFIFOLevel() const { return tx_fifo_.size(); }

    /**
     * @brief Get number of bytes in RX FIFO
     */
    size_t rxFIFOLevel() const { return rx_fifo_.size(); }

    /**
     * @brief Set TX pin level (for loopback or external connection)
     */
    void setTXPin(bool level) { tx_pin_level_ = level; }

    /**
     * @brief Get RX pin level
     */
    bool getRXPin() const { return rx_pin_level_; }

    /**
     * @brief Simulate receiving a bit on RX line
     */
    void receiveBit(bool level, uint64_t timestamp);

    /**
     * @brief Simulate one tick (update bit timing)
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
     * @brief Get decoded frames for protocol analysis
     */
    const std::deque<UARTFrame>& decodedFrames() const { return decoded_frames_; }
    void clearDecodedFrames() { decoded_frames_.clear(); }

    /**
     * @brief Get statistics
     */
    struct Stats {
        uint64_t tx_bytes = 0;
        uint64_t rx_bytes = 0;
        uint64_t framing_errors = 0;
        uint64_t parity_errors = 0;
        uint64_t overrun_errors = 0;
        uint64_t break_events = 0;
    };
    const Stats& stats() const { return stats_; }

signals:
    /**
     * @brief Byte transmitted (TX complete)
     */
    void byteTransmitted(uint8_t byte);

    /**
     * @brief Byte received
     */
    void byteReceived(uint8_t byte);

    /**
     * @brief Break condition detected
     */
    void breakDetected();

private:
    uint8_t port_;
    MemoryModel* memory_ = nullptr;

    // Configuration
    uint32_t baudrate_ = 115200;
    uint8_t data_bits_ = 8;
    UARTStopBits stop_bits_ = UARTStopBits::STOP_BITS_1;
    UARTParity parity_ = UARTParity::NONE;
    bool flow_control_ = false;

    // Bit timing
    uint64_t bit_time_ns_ = 0;  // Nanoseconds per bit
    uint64_t half_period_ns_ = 0; // Half period in ns
    uint64_t bit_ticks_ = 0;    // Accumulated partial time
    uint64_t current_bit_time_ = 0;

    // Pin state
    std::atomic<bool> tx_pin_level_{true};      // TX output (idle high)
    std::atomic<bool> rx_pin_level_{true};      // RX input (idle high)

    // Transmit state
    std::queue<uint8_t> tx_fifo_;
    uint8_t tx_current_byte_ = 0;
    int tx_bit_index_ = 0;
    bool tx_start_bit_ = false;
    bool tx_active_ = false;
    size_t tx_fifo_size_ = 128;

    // Receive state
    std::queue<uint8_t> rx_fifo_;
    uint8_t rx_current_byte_ = 0;
    int rx_bit_index_ = 0;
    bool rx_start_bit_detected_ = false;
    bool rx_active_ = false;
    size_t rx_fifo_size_ = 128;

    // Protocol decode
    UARTFrame current_frame_;
    std::deque<UARTFrame> decoded_frames_;
    bool frame_started_ = false;

    // Flow control
    bool rts_ = false;
    bool cts_ = false;

    // Interrupt state
    bool tx_break_intr_ = false;

    // Statistics
    Stats stats_;

    // Mutex
    mutable std::mutex mutex_;

    // Internal methods
    void transmitBit(bool bit);
    void processTX();
    void processRX();
    void decodeFrame();
    void checkInterrupts();

    // Utility
    uint64_t bitDurationNs() const { return bit_time_ns_; }
    uint64_t computeBitTime() const;
};

} // namespace esp32sim
