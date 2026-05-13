/**
 * @file debug_controller.h
 * @brief Debug controller for breakpoints, watchpoints, and inspection
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <set>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <QObject>
#include "simulator/core/iss/xtensa_iss.h"

namespace esp32sim {

struct SymbolInfo;

/**
 * @class DebugController
 * @brief Central debugging system
 *
 * Features:
 * - Software breakpoints
 * - Hardware breakpoint simulation
 * - Memory watchpoints
 * - Register inspection/modification
 * - Memory viewer
 * - Call stack unwinding
 * - GDB remote protocol server integration
 */
class DebugController : public QObject {
    Q_OBJECT
public:
    explicit DebugController(XtensaISS* iss, MemoryModel* memory, QObject* parent = nullptr);
    ~DebugController() override;

    /**
     * @brief Initialize debug controller
     */
    bool initialize();

    /**
     * @brief Reset debug state
     */
    void reset();

    /**
     * @brief Set software breakpoint at address
     */
    int setBreakpoint(uint32_t address, const std::string& condition = "");

    /**
     * @brief Remove breakpoint
     */
    int clearBreakpoint(uint32_t address);

    /**
     * @brief Enable/disable breakpoint
     */
    int enableBreakpoint(uint32_t address, bool enable);

    /**
     * @brief Check if breakpoint exists at address
     */
    bool hasBreakpoint(uint32_t address) const;

    /**
     * @brief Get breakpoint info
     */
    struct Breakpoint {
        uint32_t address;
        std::string condition;
        bool enabled = true;
        int hit_count = 0;
        uint32_t original_instruction = 0;  // For software BPs
    };
    const std::vector<Breakpoint>& breakpoints() const { return breakpoints_; }

    /**
     * @brief Set watchpoint
     */
    int setWatchpoint(uint32_t address, uint32_t size, bool read, bool write);

    /**
     * @brief Remove watchpoint
     */
    int clearWatchpoint(uint32_t address);

    /**
     * @brief Check if watchpoint hit
     */
    bool checkWatchpoint(uint32_t address, uint32_t value, bool is_write);

    /**
     * @brief Get all watchpoints
     */
    struct Watchpoint {
        uint32_t address;
        uint32_t size;
        bool watch_read;
        bool watch_write;
        uint64_t hit_count = 0;
    };
    const std::vector<Watchpoint>& watchpoints() const { return watchpoints_; }

    /**
     * @brief Pause execution
     */
    void pause();

    /**
     * @brief Resume execution
     */
    void resume();

    /**
     * @brief Step one instruction
     */
    void step();

    /**
     * @brief Step over (until PC changes)
     */
    void stepOver();

    /**
     * @brief Step out (until return)
     */
    void stepOut();

    /**
     * @brief Check if paused
     */
    bool isPaused() const { return paused_; }

    /**
     * @brief Get current PC
     */
    uint32_t getPC() const;

    /**
     * @brief Read register
     */
    uint32_t getRegister(uint32_t index) const;

    /**
     * @brief Write register
     */
    void setRegister(uint32_t index, uint32_t value);

    /**
     * @brief Get all registers
     */
    void getAllRegisters(uint32_t* regs) const;

    /**
     * @brief Read memory
     */
    uint32_t readMemory(uint32_t address, uint32_t size = 4) const;

    /**
     * @brief Write memory
     */
    void writeMemory(uint32_t address, uint32_t value, uint32_t size = 4);

    /**
     * @brief Get call stack
     */
    struct StackFrame {
        uint32_t return_address;
        uint32_t frame_pointer;
        std::string function_name;
        std::string source_file;
        int line_number = 0;
    };
    std::vector<StackFrame> getCallStack() const;

    /**
     * @brief Find symbol by address
     */
    const SymbolInfo* findSymbol(uint32_t address) const;

    /**
     * @brief Get stats
     */
    struct Stats {
        uint64_t breakpoints_hit = 0;
        uint64_t watchpoints_hit = 0;
        uint64_t steps_performed = 0;
        uint64_t continue_count = 0;
    };
    const Stats& stats() const { return stats_; }

signals:
    /**
     * @brief Breakpoint hit
     */
    void breakpointHit(uint32_t address, const std::string& symbol_name);

    /**
     * @brief Watchpoint hit
     */
    void watchpointHit(uint32_t address, uint32_t old_value, uint32_t new_value);

    /**
     * @brief State changed
     */
    void stateChanged(bool paused);

private:
    XtensaISS* iss_ = nullptr;
    MemoryModel* memory_ = nullptr;
    mutable std::mutex mutex_;

    // Breakpoints
    std::vector<Breakpoint> breakpoints_;
    std::set<uint32_t> breakpoint_addresses_;

    // Watchpoints
    std::vector<Watchpoint> watchpoints_;

    // State
    std::atomic<bool> paused_{true};
    std::atomic<bool> stepping_{false};

    // Call stack cache
    mutable std::vector<StackFrame> call_stack_cache_;
    mutable bool call_stack_valid_ = false;

    // Statistics
    Stats stats_;

    // Internal methods
    void checkBreakpoints();
    void invalidateCallStack() { call_stack_valid_ = false; }
    void buildCallStack() const;
    bool unwindFrame(uint32_t fp, uint32_t* ret_addr, uint32_t* next_fp) const;
    const char* demangleName(const char* mangled) const;

    // GDB server integration
    // std::unique_ptr<class GDBServer> gdb_server_;
    // bool gdb_server_running_ = false;
    // void startGDBServer(int port = 3333);
    // void stopGDBServer();
};

} // namespace esp32sim
