/**
 * @file elf_loader_wrapper.h
 * @brief Wrapper for ELF loader with error handling and symbol table management
 */

#ifndef CORE_ELF_LOADER_WRAPPER_H
#define CORE_ELF_LOADER_WRAPPER_H

#include <string>
#include <vector>
#include <memory>
#include <map>

// Include the full ElfLoader definition (global namespace)
#include "simulator/core/elf_loader/elf_loader.h"

namespace esp32sim {

/**
 * @class ElfLoaderWrapper
 * @brief High-level wrapper around the low-level ELF loader
 */
class ElfLoaderWrapper {
public:
    ElfLoaderWrapper();
    ~ElfLoaderWrapper();

    /**
     * @brief Load and parse an ELF file
     * @param filename Path to ELF file
     * @return true on success, false on failure (check getLastError())
     */
    bool load(const std::string& filename);

    /**
     * @brief Find a symbol by name
     * @return Pointer to symbol info or nullptr if not found
     */
    const SymbolInfo* findSymbol(const std::string& name) const;

    /**
     * @brief Get address of a symbol by name
     * @return Address or 0 if not found
     */
    uint32_t symbolAddress(const std::string& name) const;

    /**
     * @brief Get all symbol names
     * @return Vector of symbol names
     */
    std::vector<std::string> getAllSymbolNames() const;

    /**
     * @brief Print summary to log
     */
    void printSummary() const;

    /**
     * @brief Get last error message
     */
    const std::string& getLastError() const { return last_error_; }

private:
    std::unique_ptr<::ElfLoader> loader_;
    std::map<std::string, SymbolInfo> symbol_table_;
    std::string last_error_;
};

} // namespace esp32sim

#endif // CORE_ELF_LOADER_WRAPPER_H
