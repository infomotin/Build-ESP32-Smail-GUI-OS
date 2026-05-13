/**
 * @file uart_controller.cpp
 * @brief UART peripheral simulation implementation
 */

#include "peripherals/uart_controller.h"
#include "utils/logger.h"
#include <cmath>

#ifndef ESP_OK
#define ESP_OK 0
#endif
#ifndef ESP_ERR_TIMEOUT
#define ESP_ERR_TIMEOUT -1
#endif
#ifndef ESP_ERR_NOT_FOUND
#define ESP_ERR_NOT_FOUND -2
#endif

namespace esp32sim {

UARTController::UARTController(uint8_t port, MemoryModel* memory, QObject* parent)
    : QObject(parent), port_(port), memory_(memory) {
}

UARTController::~UARTController() = default;

bool UARTController::initialize() {
    LOG_INFO("UART{} controller initialized", port_);
    reset();
    return true;
}

void UARTController::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    while (!tx_fifo_.empty()) tx_fifo_.pop();
    while (!rx_fifo_.empty()) rx_fifo_.pop();

    tx_active_ = false;
    rx_active_ = false;
    tx_current_byte_ = 0;
    rx_current_byte_ = 0;
    tx_bit_index_ = 0;
    rx_bit_index_ = 0;

    tx_pin_level_ = true;
    rx_pin_level_ = true;

    stats_ = {};
}

void UARTController::configure(uint32_t baudrate, uint8_t data_bits,
                               UARTStopBits stop_bits, UARTParity parity,
                               bool flow_control) {
    std::lock_guard<std::mutex> lock(mutex_);

    baudrate_ = baudrate;
    data_bits_ = std::min(data_bits, (uint8_t)8);
    stop_bits_ = stop_bits;
    parity_ = parity;
    flow_control_ = flow_control;

    // Calculate bit time in nanoseconds
    bit_time_ns_ = static_cast<uint64_t>(1e9 / baudrate_);
    half_period_ns_ = bit_time_ns_ / 2;

    LOG_DEBUG("UART{} configured: {} baud, {}N{}",
              port_, baudrate, data_bits);
}

int UARTController::writeByte(uint8_t byte) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (tx_fifo_.size() >= tx_fifo_size_) {
        LOG_WARN("UART{} TX FIFO full", port_);
        return ESP_ERR_TIMEOUT;  // Or appropriate error
    }

    tx_fifo_.push(byte);
    stats_.tx_bytes++;

    // Start transmission if not active
    if (!tx_active_) {
        processTX();
    }

    Q_EMIT byteTransmitted(byte);
    return ESP_OK;
}

int UARTController::readByte(uint8_t* byte) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (rx_fifo_.empty()) {
        return ESP_ERR_NOT_FOUND;
    }

    *byte = rx_fifo_.front();
    rx_fifo_.pop();
    return ESP_OK;
}

void UARTController::receiveBit(bool level, uint64_t /*timestamp*/) {
    std::lock_guard<std::mutex> lock(mutex_);

    rx_pin_level_ = level;
    // Bit processing handled in tick()
}

void UARTController::tick(uint64_t elapsed_ns) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Update bit timing accumulator
    bit_ticks_ += elapsed_ns;

    // Process transmit
    if (tx_active_) {
        processTX();
    }

    // Process receive
    processRX();
}

void UARTController::processTX() {
    if (tx_fifo_.empty() && !tx_active_) {
        return;
    }

    if (!tx_active_) {
        // Start bit
        tx_current_byte_ = tx_fifo_.front();
        tx_active_ = true;
        tx_bit_index_ = -1;  // Start bit
        tx_pin_level_ = false;  // Start bit = 0
        tx_start_bit_ = true;
        return;
    }

    // Check if it's time for next bit
    while (bit_ticks_ >= bit_time_ns_) {
        bit_ticks_ -= bit_time_ns_;

        tx_bit_index_++;

        if (tx_bit_index_ < 0) {
            // Still transmitting start bit
            tx_pin_level_ = false;
        } else if (tx_bit_index_ < data_bits_) {
            // Data bits
            tx_pin_level_ = (tx_current_byte_ >> tx_bit_index_) & 1;
        } else if (tx_bit_index_ == data_bits_) {
            // Parity bit (if enabled)
            if (parity_ != UARTParity::NONE) {
                bool parity_bit = false;
                for (int i = 0; i < data_bits_; i++) {
                    if ((tx_current_byte_ >> i) & 1) parity_bit = !parity_bit;
                }
                if (parity_ == UARTParity::ODD) {
                    parity_bit = !parity_bit;
                }
                tx_pin_level_ = parity_bit;
            } else {
                tx_bit_index_++;  // Skip parity
                goto stop_bits;
            }
        } else {
        stop_bits:
            // Stop bits
            int stop_bits_count = (stop_bits_ == UARTStopBits::STOP_BITS_1_5) ? 2 : (int)stop_bits_;
            if (tx_bit_index_ - data_bits_ - (parity_ != UARTParity::NONE ? 1 : 0) < stop_bits_count) {
                tx_pin_level_ = true;  // Stop bit = 1
            } else {
                // Frame complete
                tx_fifo_.pop();
                tx_active_ = false;
                tx_start_bit_ = false;

                // Start next byte if any
                if (!tx_fifo_.empty()) {
                    processTX();
                }
                break;
            }
        }
    }
}

void UARTController::processRX() {
    bool current_level = rx_pin_level_;

    // Detect start bit (falling edge)
    if (!rx_active_ && !current_level && !rx_start_bit_detected_) {
        // Possible start bit - wait half bit time to confirm
        // For simplicity, assume it's valid and start sampling
        rx_active_ = true;
        rx_start_bit_detected_ = true;
        rx_bit_index_ = -1;
        rx_current_byte_ = 0;
        // Reset counter would be handled in real implementation
        current_bit_time_ = 0;
    }

    if (rx_active_) {
        // Sample at middle of bit time
        if (current_bit_time_ >= half_period_ns_) {
            // Sample
            if (rx_bit_index_ < 0) {
                // Start bit (should be 0)
                if (current_level != false) {
                    // Start bit not low - framing error
                    stats_.framing_errors++;
                    rx_active_ = false;
                    rx_start_bit_detected_ = false;
                    return;
                }
            } else if (rx_bit_index_ < data_bits_) {
                // Data bit
                if (current_level) {
                    rx_current_byte_ |= (1 << rx_bit_index_);
                }
            } else if (rx_bit_index_ == data_bits_) {
                // Parity bit
                if (parity_ != UARTParity::NONE) {
                    bool expected_parity = false;
                    for (int i = 0; i < data_bits_; i++) {
                        if ((rx_current_byte_ >> i) & 1) expected_parity = !expected_parity;
                    }
                    if (parity_ == UARTParity::ODD) expected_parity = !expected_parity;

                    if (current_level != expected_parity) {
                        stats_.parity_errors++;
                    }
                } else {
                    // No parity, this is first stop bit
                }
            } else {
                // Stop bits
                if (!current_level) {
                    // Low = framing error
                    stats_.framing_errors++;
                }

                // Frame complete
                if (rx_fifo_.size() < rx_fifo_size_) {
                    rx_fifo_.push(rx_current_byte_);
                    Q_EMIT byteReceived(rx_current_byte_);
                } else {
                    stats_.overrun_errors++;
                }

                rx_active_ = false;
                rx_start_bit_detected_ = false;
                return;
            }

            // Move to next bit
            rx_bit_index_++;
            current_bit_time_ = 0;
        } else {
            current_bit_time_ += bit_time_ns_;  // This would accumulate elapsed time
        }
    }
}

uint32_t UARTController::readRegister(uint32_t address) {
    return 0;
}

void UARTController::writeRegister(uint32_t address, uint32_t value) {
    // Handle register writes from firmware
}

} // namespace esp32sim
