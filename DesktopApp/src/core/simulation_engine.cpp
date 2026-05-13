/**
 * @file simulation_engine.cpp
 * @brief Central simulation engine implementation
 */

#include "simulation_engine.h"
#include "application.h"
#include "peripherals/gpio_controller.h"
#include "peripherals/i2c_controller.h"
#include "peripherals/spi_controller.h"
#include "peripherals/uart_controller.h"
#include "peripherals/adc_controller.h"
#include "peripherals/dac_controller.h"
#include "peripherals/ledc_controller.h"
#include "network/wifi_simulator.h"
#include "network/ble_simulator.h"
#include "debug/debug_controller.h"
#include "utils/logger.h"

#include <QTimer>
#include <QThread>
#include <QElapsedTimer>

namespace esp32sim {

SimulationEngine::SimulationEngine() {
    LOG_DEBUG("SimulationEngine: Creating engine instance");
}

SimulationEngine::~SimulationEngine() {
    shutdown();
}

bool SimulationEngine::initialize() {
    LOG_INFO("Initializing SimulationEngine");

    // Create core components
    memory_ = std::make_unique<MemoryModel>();
    scheduler_ = std::make_unique<EventScheduler>();
    elf_loader_ = std::make_unique<ElfLoader>();
    iss_ = std::make_unique<XtensaISS>(memory_.get(), scheduler_.get());

    // Initialize core
    if (!memory_->initialize()) {
        LOG_ERROR("Failed to initialize MemoryModel");
        return false;
    }

    if (!scheduler_->initialize()) {
        LOG_ERROR("Failed to initialize EventScheduler");
        return false;
    }

    // Initialize peripherals
    initializePeripherals();

    // Initialize ISS
    iss_->reset();

    // Set up statistics timer
    stats_timer_ = new QTimer();
    connect(stats_timer_, &QTimer::timeout, this, &SimulationEngine::updateStats);
    stats_timer_->start(1000);  // Update every second

    LOG_INFO("SimulationEngine initialized successfully");
    return true;
}

void SimulationEngine::shutdown() {
    LOG_INFO("Shutting down SimulationEngine");

    // Stop simulation if running
    if (isRunning()) {
        stop();
    }

    // Clean up
    debug_controller_.reset();
    wifi_sim_.reset();
    ble_sim_.reset();

    for (auto& ctrl : i2c_controller_) {
        ctrl.reset();
    }
    for (auto& ctrl : spi_controller_) {
        ctrl.reset();
    }
    for (auto& ctrl : uart_controller_) {
        ctrl.reset();
    }

    adc_controller_.reset();
    dac_controller_.reset();
    ledc_controller_.reset();
    gpio_controller_.reset();

    if (stats_timer_) {
        stats_timer_->stop();
        delete stats_timer_;
        stats_timer_ = nullptr;
    }

    iss_.reset();
    scheduler_.reset();
    memory_.reset();
    elf_loader_.reset();

    LOG_INFO("SimulationEngine shutdown complete");
}

void SimulationEngine::initializePeripherals() {
    LOG_DEBUG("Initializing peripherals");

    // GPIO first (other peripherals depend on it)
    gpio_controller_ = std::make_unique<GPIOController>(memory_.get());
    gpio_controller_->initialize();
    gpio_controller_->mapMemoryRegions();

    // I2C controllers
    i2c_controller_[0] = std::make_unique<I2CController>(0, memory_.get());
    i2c_controller_[1] = std::make_unique<I2CController>(1, memory_.get());
    i2c_controller_[0]->initialize();
    i2c_controller_[1]->initialize();
    i2c_controller_[0]->mapMemoryRegions();
    i2c_controller_[1]->mapMemoryRegions();

    // SPI controllers
    spi_controller_[0] = std::make_unique<SPIController>(0, memory_.get());
    spi_controller_[1] = std::make_unique<SPIController>(1, memory_.get());
    spi_controller_[2] = std::make_unique<SPIController>(2, memory_.get());
    for (auto& ctrl : spi_controller_) {
        ctrl->initialize();
        ctrl->mapMemoryRegions();
    }

    // UART controllers
    uart_controller_[0] = std::make_unique<UARTController>(0, memory_.get());
    uart_controller_[1] = std::make_unique<UARTController>(1, memory_.get());
    uart_controller_[2] = std::make_unique<UARTController>(2, memory_.get());
    for (auto& ctrl : uart_controller_) {
        ctrl->initialize();
        ctrl->mapMemoryRegions();
    }

    // ADC and DAC
    adc_controller_ = std::make_unique<ADCController>(memory_.get());
    adc_controller_->initialize();
    adc_controller_->mapMemoryRegions();

    dac_controller_ = std::make_unique<DACController>(memory_.get());
    dac_controller_->initialize();
    dac_controller_->mapMemoryRegions();

    // LEDC (PWM)
    ledc_controller_ = std::make_unique<LEDCController>(memory_.get());
    ledc_controller_->initialize();
    ledc_controller_->mapMemoryRegions();

    // Network
    wifi_sim_ = std::make_unique<WiFiSimulator>();
    wifi_sim_->initialize();

    ble_sim_ = std::make_unique<BLESimulator>();
    ble_sim_->initialize();

    // Debug
    debug_controller_ = std::make_unique<DebugController>(iss_.get(), memory_.get());
    debug_controller_->initialize();

    LOG_DEBUG("All peripherals initialized");
}

bool SimulationEngine::loadFirmware(const std::string& filename) {
    LOG_INFO("Loading firmware: {}", filename);

    // Stop if running
    if (isRunning()) {
        stop();
    }

    // Load ELF file
    if (!elf_loader_->load_file(filename)) {
        LOG_ERROR("Failed to load ELF file: {}", filename);
        firmware_loaded_ = false;
        emit firmwareLoaded({}, false);
        return false;
    }

    // Get entry point
    uint32_t entry = elf_loader_->get_entry_point();

    // Load segments into memory
    elf_loader_->load_segments_to_memory([this](uint32_t addr, const std::vector<uint8_t>& data) {
        for (size_t i = 0; i < data.size(); i++) {
            memory_->write_byte(addr + i, data[i]);
        }
    });

    // Populate firmware info
    firmware_info_ = {};
    firmware_info_.filename = filename;
    firmware_info_.entry_point = entry;

    // Get symbol table for debug
    const auto& symbols = elf_loader_->get_symbols();
    for (const auto& sym : symbols) {
        if (sym.type == 2) {  // STT_FUNC
            firmware_info_.symbols.push_back(sym.name);
        }
    }

    // Check for debug info
    firmware_info_.has_debug_info = !elf_loader_->get_symbols().empty();

    // Set ISS PC to entry point
    iss_->set_pc(entry);

    firmware_loaded_ = true;
    LOG_INFO("Firmware loaded successfully. Entry: 0x{:08X}, {} symbols",
             entry, symbols.size());

    emit firmwareLoaded(firmware_info_, true);
    return true;
}

bool SimulationEngine::loadArduinoSketch(const std::string& ino_file) {
    LOG_INFO("Loading Arduino sketch: {}", ino_file);

    // TODO: Implement Arduino compilation pipeline
    LOG_WARN("Arduino sketch loading not yet implemented - use precompiled ELF");
    return false;
}

void SimulationEngine::start() {
    if (state_ == SimulationState::RUNNING) return;

    LOG_INFO("Starting simulation");
    state_ = SimulationState::RUNNING;
    emitStateChange(state_, SimulationState::RUNNING);

    // Start engine thread if not running
    if (!sim_thread_running_) {
        sim_thread_running_ = true;
        sim_thread_ = std::thread(&SimulationEngine::simulationLoop, this);
    }

    scheduler_->resume();
}

void SimulationEngine::stop() {
    if (state_ == SimulationState::STOPPED) return;

    LOG_INFO("Stopping simulation");
    state_ = SimulationState::STOPPED;
    emitStateChange(state_, SimulationState::STOPPED);

    // Stop scheduler
    scheduler_->pause();

    // Signal thread to stop
    sim_thread_running_ = false;
    if (sim_thread_.joinable()) {
        sim_thread_.join();
    }
}

void SimulationEngine::pause() {
    if (state_ != SimulationState::RUNNING) return;

    LOG_INFO("Pausing simulation");
    state_ = SimulationState::PAUSED;
    emitStateChange(state_, SimulationState::PAUSED);
    scheduler_->pause();
}

void SimulationEngine::resume() {
    if (state_ != SimulationState::PAUSED) return;

    LOG_INFO("Resuming simulation");
    state_ = SimulationState::RUNNING;
    emitStateChange(state_, SimulationState::RUNNING);
    scheduler_->resume();
}

void SimulationEngine::step() {
    if (state_ != SimulationState::PAUSED && state_ != SimulationState::STOPPED) {
        pause();
    }

    LOG_DEBUG("Stepping one instruction");
    state_ = SimulationState::STEPPING;
    iss_->step();

    // Update statistics
    stats_.instructions_executed = iss_->get_statistics().instructions_executed;
    stats_.total_cycles = iss_->get_cycle_count();

    emit statisticsUpdated(stats_);
}

void SimulationEngine::reset() {
    LOG_INFO("Resetting simulation");

    // Stop first
    if (isRunning()) {
        stop();
    }

    // Reset core components (stubbed - only minimal reset)
    // Note: full peripheral reset not yet implemented

    state_ = SimulationState::STOPPED;
    stats_ = {};
    last_update_time_ = 0;

    emitStateChange(state_, SimulationState::STOPPED);
}

void SimulationEngine::setSpeedMultiplier(double multiplier) {
    speed_multiplier_ = std::max(0.1, std::min(10.0, multiplier));
    scheduler_->set_time_scale(speed_multiplier_);
    LOG_DEBUG("Simulation speed set to {:.1f}x", speed_multiplier_.load());
}

void SimulationEngine::tick() {
    // Process scheduler events
    scheduler_->process_events();

    // Update peripherals
    uint64_t current_time = scheduler_->get_current_time();

    // Update PWM
    ledc_controller_->update(current_time / 1000);  // Convert ns to us

    // Update UART bit machines
    for (auto& uart : uart_controller_) {
        uart->tick(current_time);
    }

    // Update I2C
    for (auto& i2c : i2c_controller_) {
        i2c->tick(current_time);
    }

    // Update SPI
    for (auto& spi : spi_controller_) {
        spi->tick(current_time);
    }

    // Update GPIO PWM state
    gpio_controller_->updatePWM(current_time / 1000);

    // Step ISS if running
    if (state_ == SimulationState::RUNNING) {
        iss_->step();
        stats_.instructions_executed = iss_->get_statistics().instructions_executed;
        stats_.total_cycles = iss_->get_cycle_count();
    }

    // Check for exceptions
    if (iss_->is_halted()) {
        LOG_ERROR("ISS halted");
        state_ = SimulationState::ERROR;
        emitStateChange(state_, SimulationState::ERROR);
    }

    // Emit stats periodically
    if (current_time - last_stats_time_ > 1000000000ULL) {  // 1 second
        updateStats();
        last_stats_time_ = current_time;
    }
}

void SimulationEngine::simulationLoop() {
    LOG_DEBUG("Simulation thread started");

    QElapsedTimer timer;
    timer.start();

    while (sim_thread_running_) {
        uint64_t frame_start = scheduler_->get_current_time();

        // Process one tick
        tick();

        // Rate limiting to prevent runaway
        uint64_t elapsed = timer.nsecsElapsed();
        if (elapsed < 1000000) {  // Cap at 1ms per iteration
            QThread::usleep(1);
        }
    }

    LOG_DEBUG("Simulation thread exited");
}

void SimulationEngine::updateStats() {
    if (isRunning()) {
        stats_.runtime_ms = elapsedTimeMs();
        stats_.ips = (stats_.instructions_executed * 1000.0) / (stats_.runtime_ms + 1);
        const auto& mem_stats = memory_->get_statistics();
        stats_.memory_usage = mem_stats.total_reads + mem_stats.total_writes;

        emit statisticsUpdated(stats_);
    }
}

void SimulationEngine::emitStateChange(SimulationState /*old_state*/, SimulationState new_state) {
    Q_EMIT stateChanged(new_state);
}

void SimulationEngine::postEvent(std::function<void()> callback, uint64_t delay_ns) {
    uint64_t event_id = next_event_id_++;
    uint64_t execute_at = scheduler_->get_current_time() + delay_ns;

    scheduler_->schedule_absolute_event(execute_at, EventType::CUSTOM_EVENT,
                                       EventPriority::NORMAL, [this, event_id, callback]() {
        callback();
        pending_events_.erase(event_id);
    });

    pending_events_[event_id] = callback;
}

void SimulationEngine::cancelEvent(uint64_t event_id) {
    auto it = pending_events_.find(event_id);
    if (it != pending_events_.end()) {
        pending_events_.erase(it);
    }
}

void SimulationEngine::handleException(const std::exception& e) {
    LOG_ERROR("Simulation exception: {}", e.what());
    state_ = SimulationState::ERROR;
    emit exceptionOccurred(e.what());
    emitStateChange(state_, SimulationState::ERROR);
}

// I2C accessors
I2CController* SimulationEngine::i2c(int bus) const {
    if (bus >= 0 && bus < 2 && i2c_controller_[bus]) {
        return i2c_controller_[bus].get();
    }
    return nullptr;
}

SPIController* SimulationEngine::spi(int bus) const {
    if (bus >= 0 && bus < 3 && spi_controller_[bus]) {
        return spi_controller_[bus].get();
    }
    return nullptr;
}

UARTController* SimulationEngine::uart(int port) const {
    if (port >= 0 && port < 3 && uart_controller_[port]) {
        return uart_controller_[port].get();
    }
    return nullptr;
}

void SimulationEngine::resetStatistics() {
    stats_ = {};
    iss_->reset_statistics();
    memory_->reset_statistics();
}

} // namespace esp32sim
