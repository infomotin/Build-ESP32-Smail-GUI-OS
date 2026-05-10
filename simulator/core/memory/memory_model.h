#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include <map>
#include <functional>

// ESP32 Memory Map Regions
#define ESP32_IRAM_BASE     0x40080000  // 192 KB
#define ESP32_IRAM_SIZE     0x30000
#define ESP32_DRAM_BASE     0x3FFB0000  // 296 KB
#define ESP32_DRAM_SIZE     0x4B000
#define ESP32_ROM_BASE      0x40000000  // 448 KB
#define ESP32_ROM_SIZE      0x70000
#define ESP32_IROM_BASE     0x400C2000  // External flash via instruction cache
#define ESP32_IROM_SIZE     0x1000000   // Up to 16 MB
#define ESP32_DROM_BASE     0x3FF80000  // External flash via data cache
#define ESP32_DROM_SIZE     0x1000000   // Up to 16 MB
#define ESP32_MMIO_BASE     0x3FF00000  // Memory-mapped I/O
#define ESP32_MMIO_SIZE     0x80000

// Memory access types
enum class MemoryAccessType {
    READ,
    WRITE,
    FETCH
};

// Memory region types
enum class MemoryRegionType {
    RAM,        // Read/write RAM
    ROM,        // Read-only ROM
    FLASH,      // External flash (read-only)
    MMIO,       // Memory-mapped I/O
    RESERVED    // Reserved/unmapped
};

// Memory region descriptor
struct MemoryRegion {
    uint32_t base;
    uint32_t size;
    MemoryRegionType type;
    std::vector<uint8_t> data;
    std::function<uint32_t(uint32_t)> read_handler;
    std::function<void(uint32_t, uint32_t)> write_handler;
    
    MemoryRegion(uint32_t base, uint32_t size, MemoryRegionType type)
        : base(base), size(size), type(type) {
        if (type != MemoryRegionType::MMIO) {
            data.resize(size, 0);
        }
    }
};

// Memory access exception
class MemoryAccessException : public std::exception {
private:
    std::string message;
    uint32_t address;
    MemoryAccessType access_type;
    
public:
    MemoryAccessException(uint32_t addr, MemoryAccessType type, const std::string& msg)
        : address(addr), access_type(type), message(msg) {}
    
    const char* what() const noexcept override {
        return message.c_str();
    }
    
    uint32_t get_address() const { return address; }
    MemoryAccessType get_access_type() const { return access_type; }
};

// Memory watch point
struct MemoryWatchPoint {
    uint32_t address;
    uint32_t size;
    bool read_watch;
    bool write_watch;
    std::function<void(uint32_t, uint32_t, MemoryAccessType)> callback;
    
    MemoryWatchPoint(uint32_t addr, uint32_t sz, bool read, bool write,
                    std::function<void(uint32_t, uint32_t, MemoryAccessType)> cb)
        : address(addr), size(sz), read_watch(read), write_watch(write), callback(cb) {}
};

// Memory Model class
class MemoryModel {
public:
    MemoryModel();
    ~MemoryModel();
    
    // Core memory access
    uint32_t read(uint32_t address);
    void write(uint32_t address, uint32_t value);
    uint8_t read_byte(uint32_t address);
    void write_byte(uint32_t address, uint8_t value);
    uint16_t read_halfword(uint32_t address);
    void write_halfword(uint32_t address, uint16_t value);
    uint32_t read_word(uint32_t address);
    void write_word(uint32_t address, uint32_t value);
    
    // Region management
    void add_region(uint32_t base, uint32_t size, MemoryRegionType type);
    void add_mmio_region(uint32_t base, uint32_t size,
                        std::function<uint32_t(uint32_t)> read_handler,
                        std::function<void(uint32_t, uint32_t)> write_handler);
    void remove_region(uint32_t base);
    
    // Flash operations
    bool load_flash(const std::string& filename, uint32_t base_address = ESP32_IROM_BASE);
    bool save_flash(const std::string& filename, uint32_t base_address = ESP32_IROM_BASE);
    void load_elf_segment(uint32_t load_address, const std::vector<uint8_t>& data);
    
    // Watch points
    void add_watch_point(uint32_t address, uint32_t size, bool read_watch, bool write_watch,
                        std::function<void(uint32_t, uint32_t, MemoryAccessType)> callback);
    void remove_watch_point(uint32_t address);
    void clear_watch_points();
    
    // Utility functions
    bool is_address_valid(uint32_t address) const;
    bool is_address_writable(uint32_t address) const;
    MemoryRegionType get_region_type(uint32_t address) const;
    uint32_t get_region_base(uint32_t address) const;
    uint32_t get_region_size(uint32_t address) const;
    
    // Statistics
    struct Statistics {
        uint64_t total_reads;
        uint64_t total_writes;
        uint64_t mmio_reads;
        uint64_t mmio_writes;
        uint64_t watch_triggers;
        uint64_t exceptions;
    };
    const Statistics& get_statistics() const { return stats; }
    void reset_statistics() { stats = {}; }
    
    // Debug functions
    void dump_memory(uint32_t address, uint32_t size) const;
    void dump_regions() const;
    void dump_watch_points() const;

private:
    // Memory regions
    std::vector<std::unique_ptr<MemoryRegion>> regions;
    std::map<uint32_t, MemoryRegion*> region_map; // For fast lookup
    
    // Watch points
    std::vector<std::unique_ptr<MemoryWatchPoint>> watch_points;
    
    // Statistics
    Statistics stats;
    
    // Internal helper functions
    MemoryRegion* find_region(uint32_t address);
    const MemoryRegion* find_region(uint32_t address) const;
    void check_watch_points(uint32_t address, uint32_t value, MemoryAccessType access_type);
    void update_region_map();
    
    // Default MMIO handlers
    uint32_t default_mmio_read(uint32_t address);
    void default_mmio_write(uint32_t address, uint32_t value);
    
    // Utility functions
    uint32_t align_address(uint32_t address, uint32_t alignment) const;
    bool is_aligned(uint32_t address, uint32_t alignment) const;
    uint32_t read_from_region(const MemoryRegion* region, uint32_t offset, uint32_t size);
    void write_to_region(MemoryRegion* region, uint32_t offset, uint32_t value, uint32_t size);
};
