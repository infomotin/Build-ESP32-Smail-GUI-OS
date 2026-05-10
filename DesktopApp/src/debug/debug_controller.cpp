/**
 * @file debug_controller.cpp
 * @brief Debug controller implementation
 */

#include "debug/debug_controller.h"
#include "utils/logger.h"
#include <cstring>

namespace esp32sim {

DebugController::DebugController(XtensaISS* iss, MemoryModel* memory)
    : iss_(iss), memory_(memory) {
}

DebugController::~DebugController() = default;

bool DebugController::initialize() {
    LOG_INFO("Debug Controller initialized");
    reset();
    return true;
}

void DebugController::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    breakpoints_.clear();
    breakpoint_addresses_.clear();
    watchpoints_.clear();
    paused_ = false;
    stepping_ = false;
    call_stack_valid_ = false;

    LOG_DEBUG("Debug Controller reset");
}

int DebugController::setBreakpoint(uint32_t address, const std::string& condition) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if already exists
    for (auto& bp : breakpoints_) {
        if (bp.address == address) {
            bp.enabled = true;
            LOG_DEBUG("Breakpoint at 0x{:08X} re-enabled", address);
            return 0;
        }
    }

    // Create new breakpoint (software)
    Breakpoint bp;
    bp.address = address;
    bp.condition = condition;
    bp.enabled = true;
    bp.hit_count = 0;

    // Save original instruction (for software breakpoint)
    // In real implementation, we'd read the instruction from memory and replace with BREAK
    // For now, just track it
    bp.original_instruction = 0;  // memory_->read_word(address);

    breakpoints_.push_back(bp);
    breakpoint_addresses_.insert(address);

    // Set breakpoint in ISS
    iss_->set_breakpoint(address);

    LOG_DEBUG("Breakpoint set at 0x{:08X}", address);
    return 0;
}

int DebugController::clearBreakpoint(uint32_t address) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::remove_if(breakpoints_.begin(), breakpoints_.end(),
                             [address](const Breakpoint& bp) { return bp.address == address; });

    if (it != breakpoints_.end()) {
        breakpoints_.erase(it, breakpoints_.end());
        breakpoint_addresses_.erase(address);
        iss_->clear_breakpoint(address);
        LOG_DEBUG("Breakpoint cleared at 0x{:08X}", address);
        return 0;
    }

    return -1;
}

int DebugController::enableBreakpoint(uint32_t address, bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& bp : breakpoints_) {
        if (bp.address == address) {
            bp.enabled = enable;
            LOG_DEBUG("Breakpoint at 0x{:08X} {}", address, enable ? "enabled" : "disabled");
            return 0;
        }
    }
    return -1;
}

bool DebugController::hasBreakpoint(uint32_t address) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return breakpoint_addresses_.find(address) != breakpoint_addresses_.end();
}

int DebugController::setWatchpoint(uint32_t address, uint32_t size, bool read, bool write) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Remove existing watchpoint at same address
    clearWatchpoint(address);

    Watchpoint wp;
    wp.address = address;
    wp.size = size;
    wp.watch_read = read;
    wp.watch_write = write;
    wp.hit_count = 0;

    watchpoints_.push_back(wp);

    // Register with memory model
    memory_->add_watch_point(address, size, read, write,
        [this](uint32_t addr, uint32_t value, MemoryAccessType type) {
            this->checkWatchpoints(addr, value, type == MemoryAccessType::WRITE);
        });

    LOG_DEBUG("Watchpoint set at 0x{:08X} size={} bytes", address, size);
    return 0;
}

int DebugController::clearWatchpoint(uint32_t address) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::remove_if(watchpoints_.begin(), watchpoints_.end(),
                             [address](const Watchpoint& wp) { return wp.address == address; });

    if (it != watchpoints_.end()) {
        watchpoints_.erase(it, watchpoints_.end());
        memory_->remove_watch_point(address);
        return 0;
    }

    return -1;
}

bool DebugController::checkWatchpoint(uint32_t address, uint32_t value, bool is_write) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& wp : watchpoints_) {
        if (address >= wp.address && address < wp.address + wp.size) {
            if ((is_write && wp.watch_write) || (!is_write && wp.watch_read)) {
                wp.hit_count++;
                stats_.watchpoints_hit++;

                LOG_DEBUG("Watchpoint hit at 0x{:08X}: value=0x{:08X}", address, value);
                Q_EMIT watchpointHit(address, 0, value);  // TODO: store old value

                if (paused_) return true;

                pause();
                return true;
            }
        }
    }
    return false;
}

void DebugController::pause() {
    if (paused_) return;

    LOG_DEBUG("Debug pause requested");
    paused_ = true;
    stepping_ = false;
    Q_EMIT stateChanged(true);
}

void DebugController::resume() {
    if (!paused_) return;

    LOG_DEBUG("Debug resume");
    paused_ = false;
    Q_EMIT stateChanged(false);
}

void DebugController::step() {
    if (!paused_) pause();

    LOG_DEBUG("Single step");
    stepping_ = true;
    stats_.steps_performed++;
    // ISS would step once
    // issuer->step();
    stepping_ = false;
}

void DebugController::stepOver() {
    // Simplified - just step once for now
    // Proper implementation would need to detect call instruction and step until return
    step();
}

void DebugController::stepOut() {
    // Simplified
    step();
}

uint32_t DebugController::getPC() const {
    return iss_->get_pc();
}

uint32_t DebugController::getRegister(uint32_t index) const {
    return iss_->get_register(index);
}

void DebugController::setRegister(uint32_t index, uint32_t value) {
    iss_->set_register(index, value);
}

void DebugController::getAllRegisters(uint32_t* regs) const {
    for (int i = 0; i < 64; i++) {
        regs[i] = iss_->get_register(i);
    }
}

uint32_t DebugController::readMemory(uint32_t address, uint32_t size) const {
    switch (size) {
        case 1: return memory_->read_byte(address);
        case 2: return memory_->read_halfword(address);
        case 4: return memory_->read_word(address);
        default: return 0;
    }
}

void DebugController::writeMemory(uint32_t address, uint32_t value, uint32_t size) {
    switch (size) {
        case 1: memory_->write_byte(address, value); break;
        case 2: memory_->write_halfword(address, value); break;
        case 4: memory_->write_word(address, value); break;
    }
}

std::vector<DebugController::StackFrame> DebugController::getCallStack() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!call_stack_valid_) {
        buildCallStack();
    }

    return call_stack_cache_;
}

const SymbolInfo* DebugController::findSymbol(uint32_t address) const {
    // Get symbol table from elf loader
    // For now, return nullptr
    return nullptr;
}

void DebugController::buildCallStack() const {
    call_stack_cache_.clear();

    // Simplified: Walk the stack using frame pointer (A9=A0 for windowed ABI)
    // Real implementation would need proper stack frame unwinding
    uint32_t pc = getPC();
    uint32_t fp = getRegister(9);  // A9 is typically frame pointer

    StackFrame frame;
    frame.return_address = pc;
    frame.frame_pointer = fp;
    frame.function_name = "main";  // Placeholder
    frame.source_file = "";
    frame.line_number = 0;

    call_stack_cache_.push_back(frame);
    call_stack_valid_ = true;
}

} // namespace esp32sim
