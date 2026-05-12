/**
 * @file elf_loader_wrapper.cpp
 * @brief Wrapper for ELF loader with error handling
 */

#include "core/elf_loader_wrapper.h"
#include "simulator/core/elf_loader/elf_loader.h"
#include "utils/logger.h"

#include <sstream>

namespace esp32sim {

ElfLoaderWrapper::ElfLoaderWrapper() {
    loader_ = std::make_unique<::ElfLoader>();
}

ElfLoaderWrapper::~ElfLoaderWrapper() = default;

bool ElfLoaderWrapper::load(const std::string& filename) {
    try {
        if (!loader_->load_file(filename)) {
            last_error_ = "Failed to read ELF file";
            return false;
        }

        if (!loader_->is_valid_elf()) {
            last_error_ = "Invalid ELF magic number";
            return false;
        }

        if (!loader_->is_xtensa_elf()) {
            last_error_ = "ELF file is not Xtensa architecture (expected ESP32)";
            return false;
        }

        if (!loader_->is_executable()) {
            last_error_ = "ELF file is not an executable or shared object";
            return false;
        }

        // Extract symbols
        const auto& symbols = loader_->get_symbols();
        for (const auto& sym : symbols) {
            symbol_table_[sym.name] = sym;
        }

        LOG_INFO("ELF loaded: {} symbols, entry=0x{:08X}",
                 symbols.size(), loader_->get_entry_point());
        return true;
    } catch (const std::exception& e) {
        last_error_ = e.what();
        return false;
    }
}

const SymbolInfo* ElfLoaderWrapper::findSymbol(const std::string& name) const {
    auto it = symbol_table_.find(name);
    if (it != symbol_table_.end()) {
        return &it->second;
    }
    return nullptr;
}

uint32_t ElfLoaderWrapper::symbolAddress(const std::string& name) const {
    const SymbolInfo* sym = findSymbol(name);
    return sym ? sym->address : 0;
}

std::vector<std::string> ElfLoaderWrapper::getAllSymbolNames() const {
    std::vector<std::string> names;
    for (const auto& pair : symbol_table_) {
        names.push_back(pair.first);
    }
    return names;
}

void ElfLoaderWrapper::printSummary() const {
    if (!loader_) return;

    LOG_INFO("ELF Summary:");
    LOG_INFO("  Entry point: 0x{:08X}", loader_->get_entry_point());
    LOG_INFO("  Symbol count: {}", symbol_table_.size());
    LOG_INFO("  Text size: {} bytes", loader_->get_statistics().text_size);
    LOG_INFO("  Data size: {} bytes", loader_->get_statistics().data_size);
}

} // namespace esp32sim
