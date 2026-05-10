/**
 * @file ble_simulator.cpp
 * @brief BLE simulation implementation
 */

#include "network/ble_simulator.h"
#include "utils/logger.h"

namespace esp32sim {

BLESimulator::BLESimulator() = default;

BLESimulator::~BLESimulator() = default;

bool BLESimulator::initialize() {
    LOG_INFO("BLE Simulator initialized");
    buildDefaultGATTDatabase();
    return true;
}

void BLESimulator::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    advertising_ = false;
    connected_ = false;
    stats_ = {};
    characteristics_.clear();
    services_.clear();
    buildDefaultGATTDatabase();
}

int BLESimulator::startAdvertising(const std::string& device_name,
                                   const std::string& service_uuid) {
    std::lock_guard<std::mutex> lock(mutex_);
    device_name_ = device_name;
    service_uuid_ = service_uuid;
    advertising_ = true;
    stats_.advertising_events++;

    LOG_INFO("BLE advertising started: '{}'", device_name);
    return ESP_OK;
}

int BLESimulator::stopAdvertising() {
    std::lock_guard<std::mutex> lock(mutex_);
    advertising_ = false;
    LOG_INFO("BLE advertising stopped");
    return ESP_OK;
}

int BLESimulator::connect(const std::string& peer_address) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!advertising_) {
        LOG_WARN("Cannot connect: not advertising");
        return ESP_ERR_INVALID_STATE;
    }

    peer_address_ = peer_address;
    connected_ = true;
    stats_.connections++;

    LOG_INFO("BLE connection established from {}", peer_address);
    Q_EMIT gattEvent(0, "connected");
    return ESP_OK;
}

int BLESimulator::disconnect() {
    if (!connected_) return ESP_ERR_INVALID_STATE;

    handleDisconnection(0x13);  // Connection timeout
    return ESP_OK;
}

std::vector<uint8_t> BLESimulator::handleRead(uint16_t attribute_handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.read_requests++;

    // Find characteristic
    for (const auto& ch : characteristics_) {
        if (ch.value_handle == attribute_handle) {
            LOG_DEBUG("BLE read from handle 0x{:04X}: {} bytes", attribute_handle, ch.value.size());
            return ch.value;
        }
    }

    LOG_WARN("BLE read from unknown handle 0x{:04X}", attribute_handle);
    return {};
}

int BLESimulator::handleWrite(uint16_t attribute_handle,
                              const std::vector<uint8_t>& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.write_requests++;

    for (auto& ch : characteristics_) {
        if (ch.value_handle == attribute_handle) {
            ch.value = value;

            // Call write callback if registered
            if (ch.write_callback) {
                ch.write_callback(value);
            }

            LOG_DEBUG("BLE write to handle 0x{:04X}: {} bytes", attribute_handle, value.size());
            Q_EMIT gattEvent(1, "write:" + std::to_string(attribute_handle));

            return ESP_OK;
        }
    }

    LOG_WARN("BLE write to unknown handle 0x{:04X}", attribute_handle);
    return ESP_ERR_NOT_FOUND;
}

int BLESimulator::notifyCharacteristic(uint16_t characteristic_handle,
                                        const std::vector<uint8_t>& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.notifications_sent++;

    LOG_DEBUG("BLE notification handle 0x{:04X}: {} bytes", characteristic_handle, value.size());
    Q_EMIT gattEvent(2, "notify:" + std::to_string(characteristic_handle));

    return ESP_OK;
}

void BLESimulator::buildDefaultGATTDatabase() {
    // Simple service with a few standard characteristics
    // This is a simplified BLE GATT structure

    // Device Information Service (0x180A)
    // Manufacturer Name String (0x2A29)
    GATTCharacteristic manuf;
    manuf.handle = 1;
    manuf.value_handle = 2;
    manuf.properties = 0x02;  // READ
    manuf.value = {0x45, 0x53, 0x50, 0x33, 0x32};  // "ESP32"
    characteristics_.push_back(manuf);

    // Battery Level (0x2A19)
    GATTCharacteristic battery;
    battery.handle = 3;
    battery.value_handle = 4;
    battery.properties = 0x12;  // READ | NOTIFY
    battery.value = {100};
    characteristics_.push_back(battery);
}

void BLESimulator::handleConnection(const std::string& peer_addr) {
    connected_ = true;
    peer_address_ = peer_addr;
    LOG_INFO("BLE connected from {}", peer_addr);
}

void BLESimulator::handleDisconnection(int reason) {
    connected_ = false;
    LOG_INFO("BLE disconnected, reason: 0x{:04X}", reason);
    Q_EMIT gattEvent(3, "disconnect:" + std::to_string(reason));
}

} // namespace esp32sim
