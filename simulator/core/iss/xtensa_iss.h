#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <vector>
#include <memory>

// Xtensa LX6 Register Definitions
#define XTENSA_NUM_REGISTERS 64
#define XTENSA_WINDOW_SIZE 8
#define XTENSA_NUM_WINDOWS 16

// Register indices
#define XTENSA_PC 0
#define XTENSA_PS 1
#define XTENSA_WB 2
#define XTENSA_WS 3
#define XTENSA_WOE 4
#define XTENSA_CALLINC 5
#define XTENSA_EXCCAUSE 6
#define XTENSA_EXCVADDR 7
#define XTENSA_EXCSAVE1 8
#define XTENSA_EXCSAVE2 9
#define XTENSA_EXCSAVE3 10
#define XTENSA_EXCSAVE4 11
#define XTENSA_EPC1 12
#define XTENSA_EPC2 13
#define XTENSA_EPC3 14
#define XTENSA_EPC4 15
#define XTENSA_DEPC 16
#define XTENSA_EPS2 17
#define XTENSA_EPS3 18
#define XTENSA_EPS4 19
#define XTENSA_SAR 20
#define XTENSA_ICOUNT 21
#define XTENSA_IBREAKENABLE 22
#define XTENSA_IBREAKA0 23
#define XTENSA_IBREAKA1 24
#define XTENSA_DBREAKA0 25
#define XTENSA_DBREAKA1 26
#define XTENSA_DBREAKC0 27
#define XTENSA_DBREAKC1 28
#define XTENAS_DBREAKD0 29
#define XTENAS_DBREAKD1 30
#define XTENAS_CONFIGID0 31
#define XTENAS_CONFIGID1 32
#define XTENAS_CPENABLE 33
#define XTENAS_INTENABLE 34
#define XTENAS_INTSET 35
#define XTENAS_INTCLEAR 36
#define XTENAS_INTLEVEL 37
#define XTENAS_EXCMASK 38
#define XTENSA_VECBASE 39
#define XTENAS_EXCVADDR 40

// Exception causes
#define EXCCAUSE_ILLEGAL_INSTRUCTION 0
#define EXCCAUSE_SYSCALL 1
#define EXCCAUSE_INSTRUCTION_FETCH_ERROR 2
#define EXCCAUSE_LOAD_STORE_ERROR 3
#define EXCCAUSE_LEVEL1_INTERRUPT 4
#define EXCCAUSE_ALLOCA 5
#define EXCCAUSE_INTEGER_DIVIDE_BY_ZERO 6
#define EXCCAUSE_PRIVILEGED_INSTRUCTION 7
#define EXCCAUSE_UNALIGNED_ACCESS 8
#define EXCCAUSE_INSTRUCTION_DATA_ERROR 9
#define EXCCAUSE_LOAD_STORE_DATA_ERROR 10
#define EXCCAUSE_INSTRUCTION_ADDRESS_ERROR 11
#define EXCCAUSE_LOAD_STORE_ADDRESS_ERROR 12
#define EXCCAUSE_NMI 13
#define EXCCAUSE_DEBUG_EXCEPTION 14

// Processor state
enum class ProcessorState {
    RUNNING,
    PAUSED,
    STEPPING,
    HALTED
};

// Memory access types
enum class MemoryAccessType {
    READ,
    WRITE,
    FETCH
};

// Forward declarations
class MemoryModel;
class EventScheduler;

// Xtensa ISS class
class XtensaISS {
public:
    XtensaISS(MemoryModel* memory, EventScheduler* scheduler);
    ~XtensaISS();

    // Core execution
    void reset();
    void step();
    void run(uint32_t cycles = 0);
    void pause() { state = ProcessorState::PAUSED; }
    void halt() { state = ProcessorState::HALTED; }
    bool is_running() const { return state == ProcessorState::RUNNING; }
    bool is_halted() const { return state == ProcessorState::HALTED; }

    // Register access
    uint32_t get_register(uint32_t index) const;
    void set_register(uint32_t index, uint32_t value);
    uint32_t get_pc() const { return registers[XTENSA_PC]; }
    void set_pc(uint32_t pc) { registers[XTENSA_PC] = pc; }
    
    // Windowed register operations
    uint32_t get_windowed_register(uint32_t window, uint32_t reg);
    void set_windowed_register(uint32_t window, uint32_t reg, uint32_t value);
    void rotate_window(int8_t rotation);
    
    // Interrupt handling
    void raise_interrupt(uint32_t level);
    void clear_interrupt(uint32_t level);
    uint32_t get_interrupt_level() const;
    
    // Breakpoints
    void set_breakpoint(uint32_t address);
    void clear_breakpoint(uint32_t address);
    bool has_breakpoint(uint32_t address) const;
    
    // Debugging
    void dump_registers() const;
    void dump_call_stack() const;
    uint64_t get_cycle_count() const { return cycle_count; }
    
    // Statistics
    struct Statistics {
        uint64_t instructions_executed;
        uint64_t memory_accesses;
        uint64_t interrupts_handled;
        uint64_t exceptions_taken;
        uint64_t cycles_stalled;
    };
    const Statistics& get_statistics() const { return stats; }
    void reset_statistics() { stats = {}; }

private:
    // Core components
    MemoryModel* memory;
    EventScheduler* scheduler;
    
    // Processor state
    ProcessorState state;
    uint64_t cycle_count;
    uint32_t registers[XTENSA_NUM_REGISTERS];
    uint32_t windowed_registers[XTENSA_NUM_WINDOWS][XTENSA_WINDOW_SIZE];
    uint8_t current_window;
    bool window_overflow;
    bool window_underflow;
    
    // Interrupt state
    uint32_t pending_interrupts;
    uint32_t interrupt_mask;
    uint32_t current_interrupt_level;
    
    // Breakpoints
    std::vector<uint32_t> breakpoints;
    bool step_mode;
    
    // Statistics
    Statistics stats;
    
    // Instruction execution
    uint32_t fetch_instruction(uint32_t address);
    void execute_instruction(uint32_t instruction);
    
    // Instruction implementations
    void execute_alu_instruction(uint32_t instruction);
    void execute_memory_instruction(uint32_t instruction);
    void execute_branch_instruction(uint32_t instruction);
    void execute_jump_instruction(uint32_t instruction);
    void execute_special_instruction(uint32_t instruction);
    
    // Memory access helpers
    uint32_t read_memory(uint32_t address, MemoryAccessType type);
    void write_memory(uint32_t address, uint32_t value, MemoryAccessType type);
    
    // Exception handling
    void raise_exception(uint32_t cause, uint32_t address = 0);
    void handle_exception(uint32_t cause, uint32_t address);
    
    // Window management
    void check_window_overflow();
    void check_window_underflow();
    void save_window_to_stack();
    void restore_window_from_stack();
    
    // Utility functions
    uint32_t sign_extend(uint32_t value, uint32_t bits);
    uint32_t arithmetic_shift_right(uint32_t value, uint32_t shift);
    bool condition_met(uint32_t condition);
    
    // Instruction decoding helpers
    uint32_t get_opcode(uint32_t instruction);
    uint32_t get_rd(uint32_t instruction);
    uint32_t get_rs1(uint32_t instruction);
    uint32_t get_rs2(uint32_t instruction);
    uint32_t get_imm8(uint32_t instruction);
    uint32_t get_imm12(uint32_t instruction);
    uint32_t get_imm16(uint32_t instruction);
    uint32_t get_imm24(uint32_t instruction);
};

// Instruction field extraction functions
inline uint32_t XtensaISS::get_opcode(uint32_t instruction) {
    return (instruction >> 24) & 0xFF;
}

inline uint32_t XtensaISS::get_rd(uint32_t instruction) {
    return (instruction >> 20) & 0xF;
}

inline uint32_t XtensaISS::get_rs1(uint32_t instruction) {
    return (instruction >> 16) & 0xF;
}

inline uint32_t XtensaISS::get_rs2(uint32_t instruction) {
    return (instruction >> 12) & 0xF;
}

inline uint32_t XtensaISS::get_imm8(uint32_t instruction) {
    return instruction & 0xFF;
}

inline uint32_t XtensaISS::get_imm12(uint32_t instruction) {
    return instruction & 0xFFF;
}

inline uint32_t XtensaISS::get_imm16(uint32_t instruction) {
    return instruction & 0xFFFF;
}

inline uint32_t XtensaISS::get_imm24(uint32_t instruction) {
    return instruction & 0xFFFFFF;
}
