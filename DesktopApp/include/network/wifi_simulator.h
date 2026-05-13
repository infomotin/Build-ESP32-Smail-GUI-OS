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
#include <QObject>

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
class WiFiSimulator : public QObject {
    Q_OBJECT
public:
    explicit WiFiSimulator(QObject* parent = nullptr);
    ~WiFiSimulator() override;

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

} // namespace esp32sim

