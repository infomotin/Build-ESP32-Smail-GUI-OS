#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <string>
#include <vector>

// Include core simulator components
#include "core/iss/xtensa_iss.h"
#include "core/memory/memory_model.h"
#include "core/scheduler/event_scheduler.h"
#include "core/elf_loader/elf_loader.h"
#include "peripherals/gpio/gpio_model.h"

class ConsoleSimulator {
private:
    std::unique_ptr<XtensaISS> iss;
    std::unique_ptr<MemoryModel> memory;
    std::unique_ptr<EventScheduler> scheduler;
    std::unique_ptr<ElfLoader> elfLoader;
    std::unique_ptr<GpioModel> gpioModel;
    
    bool running;
    bool paused;
    uint64_t simulationTime;
    
public:
    ConsoleSimulator() : running(false), paused(false), simulationTime(0) {
        // Initialize core components
        memory = std::make_unique<MemoryModel>();
        scheduler = std::make_unique<EventScheduler>();
        elfLoader = std::make_unique<ElfLoader>();
        gpioModel = std::make_unique<GpioModel>();
        
        // Initialize ISS with memory and scheduler
        iss = std::make_unique<XtensaISS>(memory.get(), scheduler.get());
        
        std::cout << "ESP32 Virtual Hardware Simulation Environment - Console Version\n";
        std::cout << "========================================================\n\n";
    }
    
    bool loadFirmware(const std::string& filename) {
        std::cout << "Loading firmware: " << filename << "\n";
        
        if (!elfLoader->load_file(filename)) {
            std::cerr << "ERROR: Failed to load firmware file: " << filename << "\n";
            return false;
        }
        
        if (!elfLoader->is_xtensa_elf()) {
            std::cerr << "ERROR: Not a valid ESP32 Xtensa ELF binary\n";
            return false;
        }
        
        // Load segments into memory
        elfLoader->load_segments_to_memory([this](uint32_t address, const std::vector<uint8_t>& data) {
            for (size_t i = 0; i < data.size(); i++) {
                memory->write_byte(address + i, data[i]);
            }
        });
        
        // Set entry point
        iss->set_pc(elfLoader->get_entry_point());
        
        std::cout << "Firmware loaded successfully!\n";
        std::cout << "Entry point: 0x" << std::hex << elfLoader->get_entry_point() << "\n";
        std::cout << "Text size: " << std::dec << elfLoader->get_statistics().text_size << " bytes\n";
        std::cout << "Data size: " << elfLoader->get_statistics().data_size << " bytes\n";
        std::cout << "BSS size: " << elfLoader->get_statistics().bss_size << " bytes\n";
        std::cout << "\n";
        
        return true;
    }
    
    void startSimulation() {
        if (!running) {
            running = true;
            paused = false;
            std::cout << "Simulation started\n";
            
            // Run simulation in a separate thread
            std::thread simThread([this]() {
                while (running) {
                    if (!paused) {
                        // Execute one instruction
                        iss->step();
                        simulationTime++;
                        
                        // Update scheduler
                        scheduler->set_time(simulationTime);
                        scheduler->process_events();
                        
                        // Update GPIO model
                        gpioModel->update(simulationTime);
                        
                        // Print status every 1000 instructions
                        if (simulationTime % 1000 == 0) {
                            printStatus();
                        }
                    }
                    
                    // Small delay to control simulation speed
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            });
            
            simThread.detach();
        }
    }
    
    void pauseSimulation() {
        if (running && !paused) {
            paused = true;
            std::cout << "Simulation paused\n";
        }
    }
    
    void stopSimulation() {
        if (running) {
            running = false;
            paused = false;
            std::cout << "Simulation stopped\n";
        }
    }
    
    void resetSimulation() {
        stopSimulation();
        iss->reset();
        memory->reset();
        scheduler->reset();
        gpioModel->reset();
        simulationTime = 0;
        std::cout << "Simulation reset\n";
    }
    
    void printStatus() {
        std::cout << "\r";
        std::cout << "Time: " << std::setw(8) << std::setfill(' ') << simulationTime;
        std::cout << " | PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << iss->get_pc();
        std::cout << " | Instructions: " << std::dec << iss->get_statistics().instructions_executed;
        std::cout << " | Memory accesses: " << iss->get_statistics().memory_accesses;
        std::cout << std::flush;
    }
    
    void printGpioStatus() {
        std::cout << "\nGPIO Status:\n";
        std::cout << "-----------\n";
        
        for (int pin = 0; pin < 10; pin++) { // Show first 10 pins
            GpioPinState state = gpioModel->get_pin_state(pin);
            std::cout << "GPIO" << std::setw(2) << pin << ": ";
            
            switch (state.mode) {
                case GpioMode::DISABLED:
                    std::cout << "DISABLED";
                    break;
                case GpioMode::INPUT:
                case GpioMode::INPUT_PULLUP:
                case GpioMode::INPUT_PULLDOWN:
                    std::cout << "INPUT (" << (state.input_level ? "HIGH" : "LOW") << ")";
                    break;
                case GpioMode::OUTPUT:
                case GpioMode::OUTPUT_OD:
                    std::cout << "OUTPUT (" << (state.output_level ? "HIGH" : "LOW") << ")";
                    break;
                case GpioMode::ADC:
                    std::cout << "ADC (" << state.adc_value << ")";
                    break;
                case GpioMode::DAC:
                    std::cout << "DAC (" << (int)state.dac_value << ")";
                    break;
                case GpioMode::PWM:
                    std::cout << "PWM (" << state.pwm_duty << "%)";
                    break;
                default:
                    std::cout << "UNKNOWN";
                    break;
            }
            
            std::cout << "\n";
        }
        std::cout << "\n";
    }
    
    void printHelp() {
        std::cout << "\nCommands:\n";
        std::cout << "  load <filename>     Load ELF firmware file\n";
        std::cout << "  start               Start simulation\n";
        std::cout << "  pause               Pause simulation\n";
        std::cout << "  stop                Stop simulation\n";
        std::cout << "  reset               Reset simulation\n";
        std::cout << "  step                Execute single instruction\n";
        std::cout << "  status              Show current status\n";
        std::cout << "  gpio                Show GPIO status\n";
        std::cout << "  memory <addr>       Show memory at address\n";
        std::cout << "  registers           Show CPU registers\n";
        std::cout << "  help                Show this help\n";
        std::cout << "  quit                Exit simulator\n";
        std::cout << "\n";
    }
    
    void showMemory(uint32_t address) {
        std::cout << "\nMemory at 0x" << std::hex << address << ":\n";
        for (int i = 0; i < 16; i++) {
            if (i % 8 == 0) {
                std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << (address + i) << ": ";
            }
            
            try {
                uint8_t value = memory->read_byte(address + i);
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)value << " ";
            } catch (...) {
                std::cout << "?? ";
            }
            
            if (i % 8 == 7) {
                std::cout << "\n";
            }
        }
        std::cout << "\n";
    }
    
    void showRegisters() {
        std::cout << "\nCPU Registers:\n";
        std::cout << "-------------\n";
        
        iss->dump_registers();
        std::cout << "\n";
    }
    
    void run() {
        std::cout << "ESP32 Virtual Hardware Simulator - Console Version\n";
        std::cout << "Type 'help' for available commands\n\n";
        
        std::string command;
        while (true) {
            std::cout << "esp32> ";
            std::getline(std::cin, command);
            
            // Parse command
            if (command.empty()) {
                continue;
            }
            
            if (command == "help") {
                printHelp();
            } else if (command == "quit" || command == "exit") {
                stopSimulation();
                break;
            } else if (command == "start") {
                startSimulation();
            } else if (command == "pause") {
                pauseSimulation();
            } else if (command == "stop") {
                stopSimulation();
            } else if (command == "reset") {
                resetSimulation();
            } else if (command == "step") {
                if (iss) {
                    iss->step();
                    simulationTime++;
                    printStatus();
                    std::cout << "\n";
                }
            } else if (command == "status") {
                printStatus();
                std::cout << "\n";
            } else if (command == "gpio") {
                printGpioStatus();
            } else if (command == "registers") {
                showRegisters();
            } else if (command.substr(0, 6) == "load ") {
                std::string filename = command.substr(6);
                loadFirmware(filename);
            } else if (command.substr(0, 7) == "memory ") {
                try {
                    uint32_t address = std::stoul(command.substr(7), nullptr, 16);
                    showMemory(address);
                } catch (...) {
                    std::cout << "Invalid address format. Use: memory <hex_address>\n";
                }
            } else {
                std::cout << "Unknown command: " << command << "\n";
                std::cout << "Type 'help' for available commands\n";
            }
        }
        
        std::cout << "Goodbye!\n";
    }
};

int main(int argc, char* argv[]) {
    ConsoleSimulator simulator;
    
    // If firmware file provided as argument, load it
    if (argc > 1) {
        simulator.loadFirmware(argv[1]);
    }
    
    // Run interactive console
    simulator.run();
    
    return 0;
}
