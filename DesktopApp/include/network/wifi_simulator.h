/**
 * @file wifi_simulator.h
 * @brief Wi-Fi connectivity simulation
 */

#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <map>

namespace esp32sim {

/**
 * @class WiFiSimulator
 * @brief Simulates ESP32 Wi-Fi station mode
 *
 * Features:
 * - Virtual access point emulation
 * - Association and authentication simulation
 * - DHCP server emulation
 * - RSSI simulation with user-configurable value
 * - Packet loss simulation
 * - Traffic routing between simulated firmware and host network
 */
class WiFiSimulator {
public:
    explicit WiFiSimulator();
    ~WiFiSimulator();

    /**
     * @brief Initialize Wi-Fi simulation
     */
    bool initialize();

    /**
     * @brief Reset to initial state
     */
    void reset();

    /**
     * @brief Set virtual AP configuration
     */
    void setAPConfig(const std::string& ssid, const std::string& password,
                      const std::string& ip_range = "192.168.4.0/24",
                      const std::string& gateway = "192.168.4.1");

    /**
     * @brief Start virtual AP
     */
    int startAP();

    /**
     * @brief Stop virtual AP
     */
    int stopAP();

    /**
     * @brief Connect station (firmware) to AP
     * @return ESP_OK on success
     */
    int connectStation(const std::string& ssid, const std::string& password);

    /**
     * @brief Disconnect station
     */
    int disconnectStation();

    /**
     * @brief Check if station is connected
     */
    bool isConnected() const { return station_connected_; }

    /**
     * @brief Get station IP address
     */
    std::string stationIP() const { return station_ip_; }

    /**
     * @brief Get RSSI (configurable by user)
     */
    int8_t getRSSI() const { return rssi_; }

    /**
     * @brief Set RSSI for simulation
     */
    void setRSSI(int8_t rssi) { rssi_ = rssi; }

    /**
     * @brief Get packet loss percentage (0-100)
     */
    uint8_t packetLossPercent() const { return packet_loss_; }

    /**
     * @brief Set packet loss simulation
     */
    void setPacketLossPercent(uint8_t percent) { packet_loss_ = std::min(percent, (uint8_t)100); }

    /**
     * @brief Simulate transmitting a packet
     */
    bool transmitPacket(const std::vector<uint8_t>& packet, bool to_station);

    /**
     * @brief Get statistics
     */
    struct Stats {
        uint64_t packets_sent = 0;
        uint64_t packets_received = 0;
        uint64_t packets_dropped = 0;
        uint64_t connection_attempts = 0;
        uint64_t auth_failures = 0;
    };
    const Stats& stats() const { return stats_; }

    /**
     * @brief Get AP SSID
     */
    std::string ssid() const { return ap_ssid_; }

    /**
     * @brief Check if AP is running
     */
    bool isAPRunning() const { return ap_running_; }

signals:
    /**
     * @brief Station connected/disconnected
     */
    void stationConnectionChanged(bool connected, int reason = 0);

private:
    // AP configuration
    std::string ap_ssid_ = "ESP32-Sim-AP";
    std::string ap_password_ = "12345678";
    std::string ap_ip_ = "192.168.4.1";
    std::string dhcp_range_start_ = "192.168.4.10";
    std::string dhcp_range_end_ = "192.168.4.250";
    uint8_t ap_channel_ = 6;

    // Station state
    bool station_connected_ = false;
    std::string station_ssid_;
    std::string station_ip_;
    int8_t rssi_ = -60;

    // Simulation state
    std::atomic<bool> ap_running_{false};
    std::mutex mutex_;
    Stats stats_;

    // Network simulation
    uint8_t packet_loss_ = 0;

    // Internal methods
    void authenticationSimulation(const std::string& password);
    void dhcpSimulation();
    bool simulateWirelessConditions(bool to_station);
};

/**
 * @class BLESimulator
 * @brief Bluetooth Low Energy simulation
 *
 * Features:
 * - GATT server simulation
 * - Advertising simulation
 * - Connection simulation
 * - Read/write operations simulation
 * - RSSI simulation
 */
class BLESimulator {
public:
    explicit BLESimulator();
    ~BLESimulator();

    /**
     * @brief Initialize BLE
     */
    bool initialize();

    /**
     * @brief Reset
     */
    void reset();

    /**
     * @brief Start advertising
     */
    int startAdvertising(const std::string& device_name = "ESP32-Device",
                         const std::string& service_uuid = "0000FFFF-0000-1000-8000-00805F9B34FB");

    /**
     * @brief Stop advertising
     */
    int stopAdvertising();

    /**
     * @brief Check if advertising
     */
    bool isAdvertising() const { return advertising_; }

    /**
     * @brief Simulate connection from scanner
     */
    int connect(const std::string& peer_address);

    /**
     * @brief Disconnect
     */
    int disconnect();

    /**
     * @brief Check if connected
     */
    bool isConnected() const { return connected_; }

    /**
     * @brief Handle GATT read request
     */
    std::vector<uint8_t> handleRead(uint16_t attribute_handle);

    /**
     * @brief Handle GATT write request
     */
    int handleWrite(uint16_t attribute_handle, const std::vector<uint8_t>& value);

    /**
     * @brief Notify characteristic change
     */
    int notifyCharacteristic(uint16_t characteristic_handle,
                             const std::vector<uint8_t>& value);

    /**
     * @brief Set RSSI for this connection
     */
    void setRSSI(int8_t rssi) { rssi_ = rssi; }

    /**
     * @brief Get RSSI
     */
    int8_t getRSSI() const { return rssi_; }

    /**
     * @brief Get statistics
     */
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
    /**
     * @brief GATT event
     */
    void gattEvent(int event_type, const std::string& data);

private:
    bool advertising_ = false;
    bool connected_ = false;
    std::string device_name_;
    std::string service_uuid_;
    std::string peer_address_;
    int8_t rssi_ = -70;

    // GATT database (simplified)
    struct GATTCharacteristic {
        uint16_t handle;
        uint16_t value_handle;
        uint16_t properties;
        std::vector<uint8_t> value;
        std::function<std::vector<uint8_t>()> read_callback;
        std::function<int(const std::vector<uint8_t>&)> write_callback;
    };
    std::vector<GATTCharacteristic> characteristics_;

    // Services (placeholder)
    struct GATTService {
        uint16_t start_handle;
        uint16_t end_handle;
    };
    std::vector<GATTService> services_;

    // Statistics
    Stats stats_;

    // Internal
    std::mutex mutex_;

    // Internal methods
    void buildDefaultGATTDatabase();
    void handleConnection(const std::string& peer_addr);
    void handleDisconnection(int reason);
};

} // namespace esp32sim
