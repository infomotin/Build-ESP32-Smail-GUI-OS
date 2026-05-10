#include "memory_model.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

MemoryModel::MemoryModel() {
    stats = {};
    
    // Initialize ESP32 memory regions
    add_region(ESP32_IRAM_BASE, ESP32_IRAM_SIZE, MemoryRegionType::RAM);
    add_region(ESP32_DRAM_BASE, ESP32_DRAM_SIZE, MemoryRegionType::RAM);
    add_region(ESP32_ROM_BASE, ESP32_ROM_SIZE, MemoryRegionType::ROM);
    add_region(ESP32_IROM_BASE, ESP32_IROM_SIZE, MemoryRegionType::FLASH);
    add_region(ESP32_DROM_BASE, ESP32_DROM_SIZE, MemoryRegionType::FLASH);
    add_region(ESP32_MMIO_BASE, ESP32_MMIO_SIZE, MemoryRegionType::MMIO);
}

MemoryModel::~MemoryModel() {
    // Cleanup handled by smart pointers
}

void MemoryModel::add_region(uint32_t base, uint32_t size, MemoryRegionType type) {
    auto region = std::make_unique<MemoryRegion>(base, size, type);
    region_map[base] = region.get();
    regions.push_back(std::move(region));
    update_region_map();
}

void MemoryModel::add_mmio_region(uint32_t base, uint32_t size,
                                 std::function<uint32_t(uint32_t)> read_handler,
                                 std::function<void(uint32_t, uint32_t)> write_handler) {
    auto region = std::make_unique<MemoryRegion>(base, size, MemoryRegionType::MMIO);
    region->read_handler = read_handler;
    region->write_handler = write_handler;
    region_map[base] = region.get();
    regions.push_back(std::move(region));
    update_region_map();
}

void MemoryModel::remove_region(uint32_t base) {
    auto it = region_map.find(base);
    if (it != region_map.end()) {
        region_map.erase(it);
        regions.erase(std::remove_if(regions.begin(), regions.end(),
            [base](const std::unique_ptr<MemoryRegion>& region) {
                return region->base == base;
            }), regions.end());
        update_region_map();
    }
}

uint32_t MemoryModel::read(uint32_t address) {
    return read_word(address);
}

void MemoryModel::write(uint32_t address, uint32_t value) {
    write_word(address, value);
}

uint8_t MemoryModel::read_byte(uint32_t address) {
    MemoryRegion* region = find_region(address);
    if (!region) {
        stats.exceptions++;
        throw MemoryAccessException(address, MemoryAccessType::READ, "Invalid address");
    }
    
    if (region->type == MemoryRegionType::MMIO) {
        stats.mmio_reads++;
        if (region->read_handler) {
            return region->read_handler(address) & 0xFF;
        }
        return default_mmio_read(address) & 0xFF;
    }
    
    if (region->type == MemoryRegionType::ROM || region->type == MemoryRegionType::FLASH) {
        stats.total_reads++;
        uint32_t offset = address - region->base;
        check_watch_points(address, region->data[offset], MemoryAccessType::READ);
        return region->data[offset];
    }
    
    if (region->type == MemoryRegionType::RAM) {
        stats.total_reads++;
        uint32_t offset = address - region->base;
        check_watch_points(address, region->data[offset], MemoryAccessType::READ);
        return region->data[offset];
    }
    
    stats.exceptions++;
    throw MemoryAccessException(address, MemoryAccessType::READ, "Read from reserved region");
}

void MemoryModel::write_byte(uint32_t address, uint8_t value) {
    MemoryRegion* region = find_region(address);
    if (!region) {
        stats.exceptions++;
        throw MemoryAccessException(address, MemoryAccessType::WRITE, "Invalid address");
    }
    
    if (region->type == MemoryRegionType::MMIO) {
        stats.mmio_writes++;
        if (region->write_handler) {
            region->write_handler(address, value);
        } else {
            default_mmio_write(address, value);
        }
        check_watch_points(address, value, MemoryAccessType::WRITE);
        return;
    }
    
    if (region->type == MemoryRegionType::ROM || region->type == MemoryRegionType::FLASH) {
        stats.exceptions++;
        throw MemoryAccessException(address, MemoryAccessType::WRITE, "Write to read-only region");
    }
    
    if (region->type == MemoryRegionType::RAM) {
        stats.total_writes++;
        uint32_t offset = address - region->base;
        check_watch_points(address, value, MemoryAccessType::WRITE);
        region->data[offset] = value;
        return;
    }
    
    stats.exceptions++;
    throw MemoryAccessException(address, MemoryAccessType::WRITE, "Write to reserved region");
}

uint16_t MemoryModel::read_halfword(uint32_t address) {
    if (!is_aligned(address, 2)) {
        stats.exceptions++;
        throw MemoryAccessException(address, MemoryAccessType::READ, "Unaligned halfword access");
    }
    
    uint8_t low = read_byte(address);
    uint8_t high = read_byte(address + 1);
    return low | (high << 8);
}

void MemoryModel::write_halfword(uint32_t address, uint16_t value) {
    if (!is_aligned(address, 2)) {
        stats.exceptions++;
        throw MemoryAccessException(address, MemoryAccessType::WRITE, "Unaligned halfword access");
    }
    
    write_byte(address, value & 0xFF);
    write_byte(address + 1, (value >> 8) & 0xFF);
}

uint32_t MemoryModel::read_word(uint32_t address) {
    if (!is_aligned(address, 4)) {
        stats.exceptions++;
        throw MemoryAccessException(address, MemoryAccessType::READ, "Unaligned word access");
    }
    
    uint8_t byte0 = read_byte(address);
    uint8_t byte1 = read_byte(address + 1);
    uint8_t byte2 = read_byte(address + 2);
    uint8_t byte3 = read_byte(address + 3);
    
    return byte0 | (byte1 << 8) | (byte2 << 16) | (byte3 << 24);
}

void MemoryModel::write_word(uint32_t address, uint32_t value) {
    if (!is_aligned(address, 4)) {
        stats.exceptions++;
        throw MemoryAccessException(address, MemoryAccessType::WRITE, "Unaligned word access");
    }
    
    write_byte(address, value & 0xFF);
    write_byte(address + 1, (value >> 8) & 0xFF);
    write_byte(address + 2, (value >> 16) & 0xFF);
    write_byte(address + 3, (value >> 24) & 0xFF);
}

bool MemoryModel::load_flash(const std::string& filename, uint32_t base_address) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Read file data
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(file_size);
    file.read(reinterpret_cast<char*>(data.data()), file_size);
    
    // Find appropriate region
    MemoryRegion* region = find_region(base_address);
    if (!region || (region->type != MemoryRegionType::FLASH && region->type != MemoryRegionType::RAM)) {
        return false;
    }
    
    // Copy data to memory
    uint32_t offset = base_address - region->base;
    size_t copy_size = std::min(file_size, static_cast<size_t>(region->size - offset));
    
    if (copy_size > 0) {
        std::copy(data.begin(), data.begin() + copy_size, region->data.begin() + offset);
    }
    
    return true;
}

bool MemoryModel::save_flash(const std::string& filename, uint32_t base_address) {
    MemoryRegion* region = find_region(base_address);
    if (!region || (region->type != MemoryRegionType::FLASH && region->type != MemoryRegionType::RAM)) {
        return false;
    }
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(region->data.data()), region->size);
    return true;
}

void MemoryModel::load_elf_segment(uint32_t load_address, const std::vector<uint8_t>& data) {
    MemoryRegion* region = find_region(load_address);
    if (!region || region->type != MemoryRegionType::RAM) {
        throw std::runtime_error("Invalid ELF segment load address");
    }
    
    uint32_t offset = load_address - region->base;
    if (offset + data.size() > region->size) {
        throw std::runtime_error("ELF segment exceeds region size");
    }
    
    std::copy(data.begin(), data.end(), region->data.begin() + offset);
}

void MemoryModel::add_watch_point(uint32_t address, uint32_t size, bool read_watch, bool write_watch,
                                 std::function<void(uint32_t, uint32_t, MemoryAccessType)> callback) {
    auto watch_point = std::make_unique<MemoryWatchPoint>(address, size, read_watch, write_watch, callback);
    watch_points.push_back(std::move(watch_point));
}

void MemoryModel::remove_watch_point(uint32_t address) {
    watch_points.erase(std::remove_if(watch_points.begin(), watch_points.end(),
        [address](const std::unique_ptr<MemoryWatchPoint>& wp) {
            return wp->address == address;
        }), watch_points.end());
}

void MemoryModel::clear_watch_points() {
    watch_points.clear();
}

bool MemoryModel::is_address_valid(uint32_t address) const {
    return find_region(address) != nullptr;
}

bool MemoryModel::is_address_writable(uint32_t address) const {
    const MemoryRegion* region = find_region(address);
    if (!region) return false;
    
    return (region->type == MemoryRegionType::RAM || region->type == MemoryRegionType::MMIO);
}

MemoryRegionType MemoryModel::get_region_type(uint32_t address) const {
    const MemoryRegion* region = find_region(address);
    return region ? region->type : MemoryRegionType::RESERVED;
}

uint32_t MemoryModel::get_region_base(uint32_t address) const {
    const MemoryRegion* region = find_region(address);
    return region ? region->base : 0;
}

uint32_t MemoryModel::get_region_size(uint32_t address) const {
    const MemoryRegion* region = find_region(address);
    return region ? region->size : 0;
}

void MemoryModel::dump_memory(uint32_t address, uint32_t size) const {
    std::cout << "Memory dump at 0x" << std::hex << address << " (0x" << size << " bytes):\n";
    
    for (uint32_t i = 0; i < size; i += 16) {
        std::cout << "0x" << std::setw(8) << std::setfill('0') << (address + i) << ": ";
        
        for (uint32_t j = 0; j < 16 && (i + j) < size; j++) {
            try {
                uint8_t byte = read_byte(address + i + j);
                std::cout << std::setw(2) << std::setfill('0') << (int)byte << " ";
            } catch (const MemoryAccessException&) {
                std::cout << "?? ";
            }
        }
        
        std::cout << "  ";
        
        for (uint32_t j = 0; j < 16 && (i + j) < size; j++) {
            try {
                uint8_t byte = read_byte(address + i + j);
                char c = (byte >= 32 && byte <= 126) ? byte : '.';
                std::cout << c;
            } catch (const MemoryAccessException&) {
                std::cout << '.';
            }
        }
        
        std::cout << "\n";
    }
}

void MemoryModel::dump_regions() const {
    std::cout << "Memory regions:\n";
    for (const auto& region : regions) {
        std::cout << "  0x" << std::hex << std::setw(8) << std::setfill('0') << region->base 
                  << " - 0x" << std::setw(8) << std::setfill('0') << (region->base + region->size - 1)
                  << " (" << std::dec << region->size << " bytes) - ";
        
        switch (region->type) {
            case MemoryRegionType::RAM:   std::cout << "RAM"; break;
            case MemoryRegionType::ROM:   std::cout << "ROM"; break;
            case MemoryRegionType::FLASH: std::cout << "FLASH"; break;
            case MemoryRegionType::MMIO:  std::cout << "MMIO"; break;
            case MemoryRegionType::RESERVED: std::cout << "RESERVED"; break;
        }
        
        std::cout << "\n";
    }
}

void MemoryModel::dump_watch_points() const {
    std::cout << "Watch points:\n";
    for (const auto& wp : watch_points) {
        std::cout << "  0x" << std::hex << std::setw(8) << std::setfill('0') << wp->address
                  << " (0x" << std::setw(8) << std::setfill('0') << wp->size << " bytes) - ";
        
        if (wp->read_watch) std::cout << "READ ";
        if (wp->write_watch) std::cout << "WRITE ";
        
        std::cout << "\n";
    }
}

MemoryRegion* MemoryModel::find_region(uint32_t address) {
    auto it = region_map.upper_bound(address);
    if (it != region_map.begin()) {
        --it;
        MemoryRegion* region = it->second;
        if (address >= region->base && address < region->base + region->size) {
            return region;
        }
    }
    return nullptr;
}

const MemoryRegion* MemoryModel::find_region(uint32_t address) const {
    auto it = region_map.upper_bound(address);
    if (it != region_map.begin()) {
        --it;
        const MemoryRegion* region = it->second;
        if (address >= region->base && address < region->base + region->size) {
            return region;
        }
    }
    return nullptr;
}

void MemoryModel::check_watch_points(uint32_t address, uint32_t value, MemoryAccessType access_type) {
    for (const auto& wp : watch_points) {
        if (address >= wp->address && address < wp->address + wp->size) {
            if ((access_type == MemoryAccessType::READ && wp->read_watch) ||
                (access_type == MemoryAccessType::WRITE && wp->write_watch)) {
                if (wp->callback) {
                    wp->callback(address, value, access_type);
                }
                stats.watch_triggers++;
            }
        }
    }
}

void MemoryModel::update_region_map() {
    region_map.clear();
    for (const auto& region : regions) {
        region_map[region->base] = region.get();
    }
}

uint32_t MemoryModel::default_mmio_read(uint32_t address) {
    // Return 0 for unmapped MMIO reads
    return 0;
}

void MemoryModel::default_mmio_write(uint32_t address, uint32_t value) {
    // Ignore unmapped MMIO writes
}

uint32_t MemoryModel::align_address(uint32_t address, uint32_t alignment) const {
    return address & ~(alignment - 1);
}

bool MemoryModel::is_aligned(uint32_t address, uint32_t alignment) const {
    return (address & (alignment - 1)) == 0;
}
