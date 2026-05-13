/**
 * @file wifi_simulator.cpp
 * @brief Wi-Fi simulation implementation
 */

#include "network/wifi_simulator.h"
#include "utils/logger.h"

#include <random>

#include <random>

#ifndef ESP_OK
#define ESP_OK 0
#endif
#ifndef ESP_ERR_TIMEOUT
#define ESP_ERR_TIMEOUT -2
#endif

namespace esp32sim {

WiFiSimulator::WiFiSimulator(QObject* parent) : QObject(parent) {}

WiFiSimulator::~WiFiSimulator() = default;

bool WiFiSimulator::initialize() {
    LOG_INFO("WiFi Simulator initialized");
    return true;
}

void WiFiSimulator::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    station_connected_ = false;
    station_ip_.clear();
    stats_ = {};
}

void WiFiSimulator::setAPConfig(const std::string& ssid, const std::string& password,
                                 const std::string& ip_range,
                                 const std::string& gateway) {
    ap_ssid_ = ssid;
    ap_password_ = password;
    ap_ip_ = gateway;
}

int WiFiSimulator::startAP() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Starting virtual AP '{}'", ap_ssid_);
    ap_running_ = true;
    stats_.packets_sent = 0;
    return ESP_OK;
}

int WiFiSimulator::stopAP() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Stopping virtual AP");
    ap_running_ = false;

    // Disconnect any station
    if (station_connected_) {
        station_connected_ = false;
        Q_EMIT stationConnectionChanged(false, 0);
    }
    return ESP_OK;
}

int WiFiSimulator::connectStation(const std::string& ssid, const std::string& password) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.connection_attempts++;

    LOG_INFO("Station connecting to SSID '{}'", ssid);

    if (!ap_running_) {
        LOG_WARN("AP not running, rejecting connection");
        stats_.auth_failures++;
        Q_EMIT stationConnectionChanged(false, 1);  // Reason: no AP found
        return ESP_ERR_TIMEOUT;
    }

    // Check SSID
    if (ssid != ap_ssid_) {
        LOG_WARN("SSID mismatch");
        stats_.auth_failures++;
        Q_EMIT stationConnectionChanged(false, 1);
        return ESP_ERR_TIMEOUT;
    }

    // Simulate authentication delay
    authenticationSimulation(password);

    if (password != ap_password_ && !ap_password_.empty()) {
        LOG_WARN("Authentication failed: bad password");
        stats_.auth_failures++;
        Q_EMIT stationConnectionChanged(false, 1);
        return ESP_ERR_TIMEOUT;
    }

    // Success!
    station_connected_ = true;
    station_ssid_ = ssid;
    station_ip_ = "192.168.4.10";  // Simulated DHCP

    LOG_INFO("Station connected, IP: {}", station_ip_);
    Q_EMIT stationConnectionChanged(true, 0);

    return ESP_OK;
}

int WiFiSimulator::disconnectStation() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (station_connected_) {
        LOG_INFO("Station disconnected");
        station_connected_ = false;
        station_ip_.clear();
        Q_EMIT stationConnectionChanged(false, 0);
    }
    return ESP_OK;
}

bool WiFiSimulator::transmitPacket(const std::vector<uint8_t>& packet, bool to_station) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Simulate packet loss
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 99);

    if (dist(rng) < packet_loss_) {
        stats_.packets_dropped++;
        return false;
    }

    if (to_station) {
        // Deliver to station (would go to lwIP stack)
        stats_.packets_sent++;
    } else {
        stats_.packets_received++;
    }

    return true;
}

void WiFiSimulator::authenticationSimulation(const std::string& password) {
    // Stub
}

void WiFiSimulator::dhcpSimulation() {
    // Stub
}

bool WiFiSimulator::simulateWirelessConditions(bool to_station) {
    return true; // Stub
}

} // namespace esp32sim
