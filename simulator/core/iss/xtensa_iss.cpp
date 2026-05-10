#include "xtensa_iss.h"
#include "memory/memory_model.h"
#include "scheduler/event_scheduler.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>

XtensaISS::XtensaISS(MemoryModel* memory, EventScheduler* scheduler)
    : memory(memory), scheduler(scheduler), state(ProcessorState::HALTED),
      cycle_count(0), current_window(0), window_overflow(false), window_underflow(false),
      pending_interrupts(0), interrupt_mask(0), current_interrupt_level(0),
      step_mode(false) {
    
    // Initialize registers to power-on state
    memset(registers, 0, sizeof(registers));
    memset(windowed_registers, 0, sizeof(windowed_registers));
    
    // Set initial PC to reset vector
    registers[XTENSA_PC] = 0x40000000; // ROM reset vector
    registers[XTENSA_PS] = 0x00040000; // Interrupt level 0, WOE disabled
    registers[XTENSA_WS] = 0; // Window start
    registers[XTENSA_WB] = 0; // Window base
    
    // Initialize statistics
    stats = {};
}

XtensaISS::~XtensaISS() {
    // Cleanup
}

void XtensaISS::reset() {
    // Reset all registers to power-on state
    memset(registers, 0, sizeof(registers));
    memset(windowed_registers, 0, sizeof(windowed_registers));
    
    // Reset PC to ROM reset vector
    registers[XTENSA_PC] = 0x40000000;
    registers[XTENSA_PS] = 0x00040000;
    registers[XTENSA_WS] = 0;
    registers[XTENSA_WB] = 0;
    
    // Reset window state
    current_window = 0;
    window_overflow = false;
    window_underflow = false;
    
    // Reset interrupt state
    pending_interrupts = 0;
    current_interrupt_level = 0;
    
    // Reset statistics
    cycle_count = 0;
    stats = {};
    
    // Reset state
    state = ProcessorState::HALTED;
}

void XtensaISS::step() {
    if (state == ProcessorState::HALTED) {
        return;
    }
    
    // Check for breakpoints
    if (has_breakpoint(registers[XTENSA_PC]) && !step_mode) {
        state = ProcessorState::PAUSED;
        return;
    }
    
    // Check for interrupts
    if (pending_interrupts && current_interrupt_level < 15) {
        uint32_t highest_level = 0;
        for (int level = 15; level >= 0; level--) {
            if (pending_interrupts & (1 << level)) {
                highest_level = level;
                break;
            }
        }
        
        if (highest_level > current_interrupt_level) {
            raise_exception(EXCCAUSE_LEVEL1_INTERRUPT + highest_level);
            current_interrupt_level = highest_level;
        }
    }
    
    // Fetch and execute instruction
    uint32_t instruction = fetch_instruction(registers[XTENSA_PC]);
    
    if (state == ProcessorState::RUNNING || state == ProcessorState::STEPPING) {
        execute_instruction(instruction);
        stats.instructions_executed++;
        cycle_count++;
        
        if (state == ProcessorState::STEPPING) {
            state = ProcessorState::PAUSED;
        }
    }
}

void XtensaISS::run(uint32_t cycles) {
    state = ProcessorState::RUNNING;
    
    uint32_t target_cycles = cycles ? cycles + cycle_count : UINT32_MAX;
    
    while (state == ProcessorState::RUNNING && cycle_count < target_cycles) {
        step();
    }
}

uint32_t XtensaISS::get_register(uint32_t index) const {
    if (index < XTENSA_NUM_REGISTERS) {
        return registers[index];
    }
    throw std::out_of_range("Invalid register index");
}

void XtensaISS::set_register(uint32_t index, uint32_t value) {
    if (index < XTENSA_NUM_REGISTERS) {
        registers[index] = value;
    } else {
        throw std::out_of_range("Invalid register index");
    }
}

uint32_t XtensaISS::get_windowed_register(uint32_t window, uint32_t reg) {
    if (window < XTENSA_NUM_WINDOWS && reg < XTENSA_WINDOW_SIZE) {
        return windowed_registers[window][reg];
    }
    return 0;
}

void XtensaISS::set_windowed_register(uint32_t window, uint32_t reg, uint32_t value) {
    if (window < XTENSA_NUM_WINDOWS && reg < XTENSA_WINDOW_SIZE) {
        windowed_registers[window][reg] = value;
    }
}

void XtensaISS::rotate_window(int8_t rotation) {
    current_window = (current_window + rotation) & (XTENSA_NUM_WINDOWS - 1);
    registers[XTENSA_WS] = current_window;
}

void XtensaISS::raise_interrupt(uint32_t level) {
    if (level < 16) {
        pending_interrupts |= (1 << level);
        stats.interrupts_handled++;
    }
}

void XtensaISS::clear_interrupt(uint32_t level) {
    if (level < 16) {
        pending_interrupts &= ~(1 << level);
    }
}

uint32_t XtensaISS::get_interrupt_level() const {
    return current_interrupt_level;
}

void XtensaISS::set_breakpoint(uint32_t address) {
    breakpoints.push_back(address);
}

void XtensaISS::clear_breakpoint(uint32_t address) {
    auto it = std::find(breakpoints.begin(), breakpoints.end(), address);
    if (it != breakpoints.end()) {
        breakpoints.erase(it);
    }
}

bool XtensaISS::has_breakpoint(uint32_t address) const {
    return std::find(breakpoints.begin(), breakpoints.end(), address) != breakpoints.end();
}

void XtensaISS::dump_registers() const {
    std::cout << "Register Dump:\n";
    std::cout << "PC = 0x" << std::hex << std::setw(8) << std::setfill('0') << registers[XTENSA_PC] << "\n";
    std::cout << "PS = 0x" << std::hex << std::setw(8) << std::setfill('0') << registers[XTENSA_PS] << "\n";
    std::cout << "WS = 0x" << std::hex << std::setw(8) << std::setfill('0') << registers[XTENSA_WS] << "\n";
    std::cout << "WB = 0x" << std::hex << std::setw(8) << std::setfill('0') << registers[XTENSA_WB] << "\n";
    
    // Dump windowed registers
    std::cout << "Window " << (int)current_window << ":\n";
    for (int i = 0; i < XTENSA_WINDOW_SIZE; i++) {
        std::cout << "  AR" << i << " = 0x" << std::hex << std::setw(8) << std::setfill('0') 
                  << windowed_registers[current_window][i] << "\n";
    }
}

void XtensaISS::dump_call_stack() const {
    std::cout << "Call Stack:\n";
    // TODO: Implement call stack traversal
    std::cout << "  [PC: 0x" << std::hex << std::setw(8) << std::setfill('0') 
              << registers[XTENSA_PC] << "]\n";
}

uint32_t XtensaISS::fetch_instruction(uint32_t address) {
    return read_memory(address, MemoryAccessType::FETCH);
}

void XtensaISS::execute_instruction(uint32_t instruction) {
    uint32_t opcode = get_opcode(instruction);
    
    switch (opcode) {
        case 0x01: // L8UI
        case 0x02: // L16UI
        case 0x03: // L32I
        case 0x04: // S8I
        case 0x05: // S16I
        case 0x06: // S32I
            execute_memory_instruction(instruction);
            break;
            
        case 0x07: // L32AI
        case 0x08: // S32AI
        case 0x09: // ADDI
        case 0x0A: // ADDMI
        case 0x0B: // SUBI
            execute_alu_instruction(instruction);
            break;
            
        case 0x0C: // J
        case 0x0D: // JX
        case 0x0E: // CALL0
        case 0x0F: // CALL4
        case 0x10: // CALL8
        case 0x11: // CALL12
            execute_jump_instruction(instruction);
            break;
            
        case 0x12: // BEQ
        case 0x13: // BNE
        case 0x14: // BLT
        case 0x15: // BGE
        case 0x16: // BLTU
        case 0x17: // BGEU
            execute_branch_instruction(instruction);
            break;
            
        case 0x18: // BEQI
        case 0x19: // BNEI
        case 0x1A: // BLTI
        case 0x1B: // BGEI
        case 0x1C: // BLTUI
        case 0x1D: // BGEUI
            execute_branch_instruction(instruction);
            break;
            
        case 0x1E: // ENTRY
        case 0x1F: // RETW
        case 0x20: // ROTW
            execute_special_instruction(instruction);
            break;
            
        default:
            // Handle other opcodes
            execute_alu_instruction(instruction);
            break;
    }
}

void XtensaISS::execute_alu_instruction(uint32_t instruction) {
    uint32_t rd = get_rd(instruction);
    uint32_t rs1 = get_rs1(instruction);
    uint32_t rs2 = get_rs2(instruction);
    uint32_t opcode = get_opcode(instruction);
    
    uint32_t rs1_val = get_windowed_register(current_window, rs1);
    uint32_t rs2_val = get_windowed_register(current_window, rs2);
    uint32_t result = 0;
    
    switch (opcode) {
        case 0x09: // ADDI
            result = rs1_val + get_imm12(instruction);
            break;
            
        case 0x0A: // ADDMI
            result = rs1_val + (get_imm8(instruction) << 8);
            break;
            
        case 0x0B: // SUBI
            result = rs1_val - get_imm12(instruction);
            break;
            
        default:
            // Default case - implement as ADD
            result = rs1_val + rs2_val;
            break;
    }
    
    set_windowed_register(current_window, rd, result);
}

void XtensaISS::execute_memory_instruction(uint32_t instruction) {
    uint32_t opcode = get_opcode(instruction);
    uint32_t rs1 = get_rs1(instruction);
    uint32_t rs2 = get_rs2(instruction);
    
    uint32_t rs1_val = get_windowed_register(current_window, rs1);
    uint32_t rs2_val = get_windowed_register(current_window, rs2);
    uint32_t address = rs1_val + get_imm12(instruction);
    
    switch (opcode) {
        case 0x01: // L8UI
            {
                uint8_t value = read_memory(address, MemoryAccessType::READ);
                set_windowed_register(current_window, rs2, value);
            }
            break;
            
        case 0x02: // L16UI
            {
                uint16_t value = read_memory(address, MemoryAccessType::READ);
                set_windowed_register(current_window, rs2, value);
            }
            break;
            
        case 0x03: // L32I
            {
                uint32_t value = read_memory(address, MemoryAccessType::READ);
                set_windowed_register(current_window, rs2, value);
            }
            break;
            
        case 0x04: // S8I
            write_memory(address, rs2_val, MemoryAccessType::WRITE);
            break;
            
        case 0x05: // S16I
            write_memory(address, rs2_val, MemoryAccessType::WRITE);
            break;
            
        case 0x06: // S32I
            write_memory(address, rs2_val, MemoryAccessType::WRITE);
            break;
            
        default:
            raise_exception(EXCCAUSE_ILLEGAL_INSTRUCTION, registers[XTENSA_PC]);
            break;
    }
    
    stats.memory_accesses++;
}

void XtensaISS::execute_branch_instruction(uint32_t instruction) {
    uint32_t opcode = get_opcode(instruction);
    uint32_t rs1 = get_rs1(instruction);
    uint32_t rs2 = get_rs2(instruction);
    
    uint32_t rs1_val = get_windowed_register(current_window, rs1);
    uint32_t rs2_val = get_windowed_register(current_window, rs2);
    
    bool condition = false;
    
    switch (opcode) {
        case 0x12: // BEQ
        case 0x18: // BEQI
            condition = (rs1_val == rs2_val);
            break;
            
        case 0x13: // BNE
        case 0x19: // BNEI
            condition = (rs1_val != rs2_val);
            break;
            
        case 0x14: // BLT
        case 0x1A: // BLTI
            condition = ((int32_t)rs1_val < (int32_t)rs2_val);
            break;
            
        case 0x15: // BGE
        case 0x1B: // BGEI
            condition = ((int32_t)rs1_val >= (int32_t)rs2_val);
            break;
            
        case 0x16: // BLTU
        case 0x1C: // BLTUI
            condition = (rs1_val < rs2_val);
            break;
            
        case 0x17: // BGEU
        case 0x1D: // BGEUI
            condition = (rs1_val >= rs2_val);
            break;
    }
    
    if (condition) {
        int32_t offset = (int32_t)(int16_t)get_imm16(instruction);
        registers[XTENSA_PC] += offset * 4; // Branches are word-aligned
    } else {
        registers[XTENSA_PC] += 4; // Next instruction
    }
}

void XtensaISS::execute_jump_instruction(uint32_t instruction) {
    uint32_t opcode = get_opcode(instruction);
    
    switch (opcode) {
        case 0x0C: // J
            {
                int32_t offset = (int32_t)(int24_t)get_imm24(instruction);
                registers[XTENSA_PC] += offset * 4;
            }
            break;
            
        case 0x0D: // JX
            {
                uint32_t rs1 = get_rs1(instruction);
                uint32_t target = get_windowed_register(current_window, rs1);
                registers[XTENSA_PC] = target;
            }
            break;
            
        case 0x0E: // CALL0
            {
                // Save return address
                set_windowed_register(current_window, get_rd(instruction), registers[XTENSA_PC] + 4);
                
                // Jump to target
                int32_t offset = (int32_t)(int24_t)get_imm24(instruction);
                registers[XTENSA_PC] += offset * 4;
            }
            break;
            
        default:
            raise_exception(EXCCAUSE_ILLEGAL_INSTRUCTION, registers[XTENSA_PC]);
            break;
    }
}

void XtensaISS::execute_special_instruction(uint32_t instruction) {
    uint32_t opcode = get_opcode(instruction);
    
    switch (opcode) {
        case 0x1E: // ENTRY
            {
                uint32_t size = get_imm8(instruction);
                // Save window to stack
                save_window_to_stack();
                // Rotate window
                rotate_window(1);
                // Adjust stack pointer
                uint32_t sp = get_windowed_register(current_window, 1); // A1 is SP
                sp -= size * 16;
                set_windowed_register(current_window, 1, sp);
            }
            break;
            
        case 0x1F: // RETW
            {
                // Restore window from stack
                restore_window_from_stack();
                // Rotate window back
                rotate_window(-1);
                // Load return address
                uint32_t ret_addr = get_windowed_register(current_window, 0); // A0 holds return
                registers[XTENSA_PC] = ret_addr;
            }
            break;
            
        case 0x20: // ROTW
            {
                int8_t rotation = (int8_t)get_imm8(instruction);
                rotate_window(rotation);
                registers[XTENSA_PC] += 4;
            }
            break;
            
        default:
            raise_exception(EXCCAUSE_ILLEGAL_INSTRUCTION, registers[XTENSA_PC]);
            break;
    }
}

uint32_t XtensaISS::read_memory(uint32_t address, MemoryAccessType type) {
    if (memory) {
        return memory->read(address);
    }
    return 0;
}

void XtensaISS::write_memory(uint32_t address, uint32_t value, MemoryAccessType type) {
    if (memory) {
        memory->write(address, value);
    }
}

void XtensaISS::raise_exception(uint32_t cause, uint32_t address) {
    stats.exceptions_taken++;
    handle_exception(cause, address);
}

void XtensaISS::handle_exception(uint32_t cause, uint32_t address) {
    // Save current PC
    registers[XTENSA_EPC1] = registers[XTENSA_PC];
    
    // Set exception cause
    registers[XTENSA_EXCCAUSE] = cause;
    
    // Set exception address
    if (address) {
        registers[XTENSA_EXCVADDR] = address;
    }
    
    // Jump to exception handler
    uint32_t vecbase = registers[XTENSA_VECBASE];
    uint32_t handler_addr = vecbase + (cause * 16);
    registers[XTENSA_PC] = handler_addr;
    
    // Update PS register
    registers[XTENSA_PS] &= ~0x0000000F; // Clear current level
    registers[XTENSA_PS] |= 0x00000001; // Set to level 1
}

void XtensaISS::save_window_to_stack() {
    // TODO: Implement window save to stack
}

void XtensaISS::restore_window_from_stack() {
    // TODO: Implement window restore from stack
}

uint32_t XtensaISS::sign_extend(uint32_t value, uint32_t bits) {
    if (bits < 32) {
        uint32_t mask = 1 << (bits - 1);
        return (value ^ mask) - mask;
    }
    return value;
}

uint32_t XtensaISS::arithmetic_shift_right(uint32_t value, uint32_t shift) {
    return (int32_t)value >> shift;
}

bool XtensaISS::condition_met(uint32_t condition) {
    // TODO: Implement condition evaluation
    return false;
}
