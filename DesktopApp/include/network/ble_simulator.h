/**
 * @file ble_simulator.h
 * @brief BLE (Bluetooth Low Energy) simulator stub
 */

#ifndef NETWORK_BLE_SIMULATOR_H
#define NETWORK_BLE_SIMULATOR_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace esp32sim {

/**
 * @class BLESimulator
 * @brief Simulates ESP32 BLE radio and GATT (stub)
 */
class BLESimulator {
public:
    BLESimulator();
    ~BLESimulator();

    bool initialize();
    void reset();

    // Advertising
    int startAdvertising(const std::string& device_name, const std::string& service_uuid);
    int stopAdvertising();

    // Connections
    int connect(const std::string& peer_address);
    int disconnect();

    // GATT
    std::vector<uint8_t> handleRead(uint16_t attribute_handle);
    int handleWrite(uint16_t attribute_handle, const std::vector<uint8_t>& value);

    // Notifications/Indications
    int sendNotification(uint16_t characteristic_handle, const std::vector<uint8_t>& value);
    int sendIndication(uint16_t characteristic_handle, const std::vector<uint8_t>& value);

    // State queries
    bool isAdvertising() const { return advertising_; }
    bool isConnected() const { return connected_; }
    std::string connectedPeer() const { return peer_address_; }

    // Statistics
    struct Stats {
        uint64_t advertising_events = 0;
        uint64_t connections = 0;
        uint64_t disconnections = 0;
        uint64_t read_requests = 0;
        uint64_t write_requests = 0;
        uint64_t notifications_sent = 0;
    };
    const Stats& stats() const { return stats_; }

signals:
    // Qt signals for UI updates
    void gattEvent(int type, const std::string& desc); // type: 0=connect, 1=write, etc.

private:
    void buildDefaultGATTDatabase();
    void handleDisconnection(uint16_t reason);

    mutable std::mutex mutex_;
    std::atomic<bool> advertising_{false};
    std::atomic<bool> connected_{false};
    std::string device_name_;
    std::string service_uuid_;
    std::string peer_address_;

    Stats stats_;

    // GATT database (stub)
    struct Characteristic {
        uint16_t handle;
        uint16_t value_handle;
        std::vector<uint8_t> value;
        std::function<void(const std::vector<uint8_t>&)> write_callback;
    };
    std::vector<Characteristic> characteristics_;
    std::vector<uint16_t> services_;
};

} // namespace esp32sim

#endif // NETWORK_BLE_SIMULATOR_H
