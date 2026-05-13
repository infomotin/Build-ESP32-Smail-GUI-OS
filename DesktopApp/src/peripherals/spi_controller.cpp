/**
 * @file spi_controller.cpp
 * @brief SPI controller stub implementation
 */

#include "peripherals/spi_controller.h"
#include "utils/logger.h"

namespace esp32sim {

SPIController::SPIController(uint8_t bus_id, MemoryModel* memory, QObject* parent)
    : QObject(parent), bus_id_(bus_id), memory_(memory) {
}

SPIController::~SPIController() = default;

bool SPIController::initialize() {
    LOG_DEBUG("SPIController {}: Initialized (stub)", bus_id_);
    return true;
}

void SPIController::reset() {
    LOG_DEBUG("SPIController {}: Reset", bus_id_);
}

void SPIController::configure(uint32_t baudrate_hz, SPIMode mode, uint8_t data_bits, SPITransferDirection direction) {
    baudrate_hz_ = baudrate_hz;
    mode_ = mode;
    data_bits_ = data_bits;
    direction_ = direction;
    LOG_DEBUG("SPIController {}: Configured {}Hz, Mode {}", bus_id_, baudrate_hz_, static_cast<int>(mode_));
}

std::vector<uint8_t> SPIController::transfer(const std::vector<uint8_t>& tx_data, uint32_t timeout_ms) {
    LOG_DEBUG("SPIController {}: transfer ({} bytes) - stub", bus_id_, tx_data.size());
    std::vector<uint8_t> rx_data(tx_data.size(), 0);
    return rx_data;
}

void SPIController::transferFullDuplex(const uint8_t* tx_data, uint8_t* rx_data, size_t len, uint32_t timeout_ms) {
    LOG_DEBUG("SPIController {}: transferFullDuplex ({} bytes) - stub", bus_id_, len);
}

void SPIController::setCSPin(uint8_t cs_pin) {
    cs_pin_ = cs_pin;
}

void SPIController::registerSlaveDevice(std::shared_ptr<SPISlaveDevice> device) {
    slave_devices_.push_back(device);
}

void SPIController::unregisterSlaveDevice(uint8_t cs_pin) {
    for (auto it = slave_devices_.begin(); it != slave_devices_.end(); ++it) {
        if ((*it)->cs_pin == cs_pin) {
            slave_devices_.erase(it);
            break;
        }
    }
}

void SPIController::tick(uint64_t elapsed_ns) {
    // stub
}

void SPIController::mapMemoryRegions() {
    LOG_DEBUG("SPIController {}: mapMemoryRegions (stub)", bus_id_);
}

uint32_t SPIController::readRegister(uint32_t address) {
    return 0;
}

void SPIController::writeRegister(uint32_t address, uint32_t value) {
    // stub
}

} // namespace esp32sim
