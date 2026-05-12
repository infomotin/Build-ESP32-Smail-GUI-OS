/**
 * @file simulation_engine.h
 * @brief Central simulation engine coordinating all components
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <QObject>
#include <QTimer>

#include "simulator/core/iss/xtensa_iss.h"
#include "simulator/core/memory/memory_model.h"
#include "simulator/core/scheduler/event_scheduler.h"
#include "simulator/core/elf_loader/elf_loader.h"

namespace esp32sim {

// Bring core types from global namespace into esp32sim
using ::MemoryModel;
using ::EventScheduler;
using ::ElfLoader;
using ::XtensaISS;

// Forward declarations for peripheral classes (defined in DesktopApp)
class GPIOController;
class I2CController;
class SPIController;
class UARTController;
class ADCController;
class DACController;
class LEDCController;
class WiFiSimulator;
class BLESimulator;
class DebugController;

/**
 * @enum SimulationState
 * @brief Current state of the simulation
 */
enum class SimulationState {
    STOPPED,      /**< Simulation is stopped */
    RUNNING,      /**< Simulation is running */
    PAUSED,       /**< Simulation is paused */
    STEPPING,     /**< Single stepping mode */
    ERROR         /**< Simulation encountered an error */
};

/**
 * @struct SimulationStatistics
 * @brief Statistics about simulation execution
 */
struct SimulationStatistics {
    uint64_t total_cycles = 0;
    uint64_t instructions_executed = 0;
    uint64_t runtime_ms = 0;
    double ips = 0.0;  // Instructions per second
    uint32_t cpu_usage = 0;
    uint32_t memory_usage = 0;
    std::map<std::string, uint64_t> peripheral_activity;
};

/**
 * @struct FirmwareInfo
 * @brief Information about loaded firmware
 */
struct FirmwareInfo {
    std::string filename;
    std::string build_timestamp;
    std::string version;
    uint32_t entry_point = 0;
    uint32_t text_size = 0;
    uint32_t data_size = 0;
    uint32_t bss_size = 0;
    std::vector<std::string> symbols;
    bool has_debug_info = false;
};

/**
 * @class SimulationEngine
 * @brief Central engine coordinating the entire simulation
 *
 * This class integrates the ISS, memory model, peripherals, and provides
 * the main simulation loop with timing control.
 */
class SimulationEngine : public QObject {
    Q_OBJECT

public:
    explicit SimulationEngine();
    ~SimulationEngine();

    /**
     * @brief Initialize the simulation engine
     * @return true on success
     */
    bool initialize();

    /**
     * @brief Shutdown the simulation engine
     */
    void shutdown();

    /**
     * @brief Load firmware from ELF file
     * @param filename Path to ELF file
     * @return true on success
     */
    bool loadFirmware(const std::string& filename);

    /**
     * @brief Load Arduino sketch (compile and load)
     * @param ino_file Path to .ino file
     * @return true on success
     */
    bool loadArduinoSketch(const std::string& ino_file);

    /**
     * @brief Get firmware info
     */
    const FirmwareInfo& firmwareInfo() const { return firmware_info_; }

    /**
     * @brief Check if firmware is loaded
     */
    bool isFirmwareLoaded() const { return firmware_loaded_; }

    // Simulation control
    void start();
    void stop();
    void pause();
    void resume();
    void step();
    void reset();
    void setSpeed(double multiplier);  // 0.1x to 10x

    SimulationState state() const { return state_; }
    bool isRunning() const { return state_ == SimulationState::RUNNING; }
    bool isPaused() const { return state_ == SimulationState::PAUSED; }

    // Speed control
    double speed() const { return speed_multiplier_; }
    void setSpeedMultiplier(double multiplier);

    /**
     * @brief Get simulation time in microseconds
     */
    uint64_t simulationTime() const { return scheduler_ ? scheduler_->get_current_time() : 0; }

    /**
     * @brief Get elapsed simulation time in ms
     */
    uint64_t elapsedTimeMs() const { return simulationTime() / 1000; }

    // Accessors for components
    XtensaISS* iss() const { return iss_.get(); }
    MemoryModel* memory() const { return memory_.get(); }
    EventScheduler* scheduler() const { return scheduler_.get(); }
    ElfLoader* elfLoader() const { return elf_loader_.get(); }

    // Peripheral access
    GPIOController* gpio() const { return gpio_controller_.get(); }
    I2CController* i2c(int bus = 0) const;
    SPIController* spi(int bus = 1) const;
    UARTController* uart(int port = 0) const;
    ADCController* adc() const { return adc_controller_.get(); }
    DACController* dac() const { return dac_controller_.get(); }
    LEDCController* ledc() const { return ledc_controller_.get(); }
    WiFiSimulator* wifi() const { return wifi_sim_.get(); }
    BLESimulator* ble() const { return ble_sim_.get(); }
    DebugController* debug() const { return debug_controller_.get(); }

    // Statistics
    const SimulationStatistics& statistics() const { return stats_; }
    void resetStatistics();

    // Event handling
    void postEvent(std::function<void()> callback, uint64_t delay_ns = 0);
    void cancelEvent(uint64_t event_id);

signals:
    /**
     * @brief Emitted when simulation state changes
     */
    void stateChanged(SimulationState new_state);

    /**
     * @brief Emitted when firmware is loaded
     */
    void firmwareLoaded(const FirmwareInfo& info, bool success);

    /**
     * @brief Emitted when an exception occurs
     */
    void exceptionOccurred(const std::string& message);

    /**
     * @brief Emitted periodically with statistics
     */
    void statisticsUpdated(const SimulationStatistics& stats);

private slots:
    /**
     * @brief Main simulation loop tick
     */
    void tick();

    /**
     * @brief Update statistics periodically
     */
    void updateStats();

private:
    // State
    std::atomic<SimulationState> state_{SimulationState::STOPPED};
    std::atomic<double> speed_multiplier_{1.0};
    bool firmware_loaded_ = false;
    FirmwareInfo firmware_info_;

    // Core components
    std::unique_ptr<MemoryModel> memory_;
    std::unique_ptr<EventScheduler> scheduler_;
    std::unique_ptr<ElfLoader> elf_loader_;
    std::unique_ptr<XtensaISS> iss_;
    std::unique_ptr<GPIOController> gpio_controller_;
    std::unique_ptr<I2CController> i2c_controller_[2];  // I2C0, I2C1
    std::unique_ptr<SPIController> spi_controller_[3];  // SPI0, SPI1, SPI2
    std::unique_ptr<UARTController> uart_controller_[3]; // UART0, UART1, UART2
    std::unique_ptr<ADCController> adc_controller_;
    std::unique_ptr<DACController> dac_controller_;
    std::unique_ptr<LEDCController> ledc_controller_;
    std::unique_ptr<WiFiSimulator> wifi_sim_;
    std::unique_ptr<BLESimulator> ble_sim_;
    std::unique_ptr<DebugController> debug_controller_;

    // Threading
    std::thread sim_thread_;
    std::atomic<bool> sim_thread_running_{false};
    mutable std::mutex sim_mutex_;
    std::condition_variable sim_cv_;

    // Timing
    uint64_t last_update_time_ = 0;
    uint64_t tick_interval_ns_ = 1000;  // 1 microsecond per tick

    // Statistics
    SimulationStatistics stats_;
    QTimer* stats_timer_ = nullptr;
    uint64_t last_stats_time_ = 0;
    uint64_t last_instruction_count_ = 0;

    // Event tracking
    uint64_t next_event_id_ = 1;
    std::map<uint64_t, std::function<void()>> pending_events_;

    // Internal methods
    void initializePeripherals();
    void cleanupPeripherals();
    void simulationLoop();
    void handleException(const std::exception& e);
    void updateStatistics();
    void emitStateChange(SimulationState old_state, SimulationState new_state);
};

} // namespace esp32sim
