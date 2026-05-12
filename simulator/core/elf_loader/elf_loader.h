#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <functional>

// ELF constants
#define ELF_MAGIC 0x464C457F  // "\x7FELF"
#define EM_XTENSA 94          // Xtensa architecture
#define ET_EXEC 2              // Executable file
#define ET_DYN 3               // Shared object
#define SHT_SYMTAB 2           // Symbol table
#define SHT_STRTAB 3           // String table
#define SHT_PROGBITS 1         // Program data
#define SHT_NOBITS 8           // BSS section

// ELF file header (32-bit)
 struct Elf32Header {
     uint32_t e_magic;
     uint8_t  e_class;
     uint8_t  e_data;
     uint8_t  e_ident_version;  // EI_VERSION byte from e_ident
     uint8_t  e_osabi;
     uint8_t  e_abiversion;
     uint8_t  e_pad[7];
     uint16_t e_type;
     uint16_t e_machine;
     uint32_t e_version;  // Object file version (must be 1)
     uint32_t e_entry;
     uint32_t e_phoff;
     uint32_t e_shoff;
     uint32_t e_flags;
     uint16_t e_ehsize;
     uint16_t e_phentsize;
     uint16_t e_phnum;
     uint16_t e_shentsize;
     uint16_t e_shnum;
     uint16_t e_shstrndx;
 };

// ELF program header (32-bit)
struct Elf32ProgramHeader {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

// ELF section header (32-bit)
struct Elf32SectionHeader {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
};

// ELF symbol (32-bit)
struct Elf32Symbol {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
};

// Symbol information
struct SymbolInfo {
    std::string name;
    uint32_t address;
    uint32_t size;
    uint8_t type;
    uint8_t binding;
    uint16_t section;
};

// Section information
struct SectionInfo {
    std::string name;
    uint32_t address;
    uint32_t size;
    uint32_t offset;
    uint32_t type;
    uint32_t flags;
    std::vector<uint8_t> data;
};

// ELF loading exception
class ElfLoadException : public std::exception {
private:
    std::string message;
    
public:
    ElfLoadException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

// ELF Loader class
class ElfLoader {
public:
    ElfLoader();
    ~ElfLoader();
    
    // Loading operations
    bool load_file(const std::string& filename);
    bool load_memory(const std::vector<uint8_t>& data);
    
    // Validation
    bool is_valid_elf() const;
    bool is_xtensa_elf() const;
    bool is_executable() const;
    
    // Header access
    const Elf32Header& get_header() const { return header; }
    uint32_t get_entry_point() const { return header.e_entry; }
    
    // Program headers
    const std::vector<Elf32ProgramHeader>& get_program_headers() const { return program_headers; }
    uint32_t get_program_header_count() const { return program_headers.size(); }
    
    // Section headers
    const std::vector<Elf32SectionHeader>& get_section_headers() const { return section_headers; }
    uint32_t get_section_header_count() const { return section_headers.size(); }
    
    // Symbol table
    const std::vector<SymbolInfo>& get_symbols() const { return symbols; }
    const SymbolInfo* find_symbol(const std::string& name) const;
    const SymbolInfo* find_symbol_by_address(uint32_t address) const;
    
    // Section data
    const std::vector<SectionInfo>& get_sections() const { return sections; }
    const SectionInfo* find_section(const std::string& name) const;
    const SectionInfo* find_section_by_address(uint32_t address) const;
    
    // Loading to memory
    void load_segments_to_memory(std::function<void(uint32_t, const std::vector<uint8_t>&)> write_memory);
    void load_section_to_memory(const std::string& section_name, 
                               std::function<void(uint32_t, const std::vector<uint8_t>&)> write_memory);
    
    // Utility functions
    void print_header() const;
    void print_program_headers() const;
    void print_section_headers() const;
    void print_symbols() const;
    void print_sections() const;
    
    // Statistics
    struct Statistics {
        uint32_t total_segments;
        uint32_t loadable_segments;
        uint32_t total_sections;
        uint32_t symbol_count;
        uint32_t text_size;
        uint32_t data_size;
        uint32_t bss_size;
    };
    const Statistics& get_statistics() const { return stats; }

private:
    // ELF data
    std::vector<uint8_t> elf_data;
    Elf32Header header;
    std::vector<Elf32ProgramHeader> program_headers;
    std::vector<Elf32SectionHeader> section_headers;
    std::vector<SymbolInfo> symbols;
    std::vector<SectionInfo> sections;
    
    // String tables
    std::string shstrtab;  // Section header string table
    std::string strtab;    // Symbol string table
    
    // Statistics
    Statistics stats;
    
    // Internal parsing functions
    void parse_header();
    void parse_program_headers();
    void parse_section_headers();
    void parse_symbol_table();
    void parse_string_tables();
    void parse_section_data();
    
    // Utility functions
    uint32_t read_uint32(size_t offset) const;
    uint16_t read_uint16(size_t offset) const;
    uint8_t read_uint8(size_t offset) const;
    std::string read_string(size_t offset) const;
    std::string read_string_from_table(uint32_t offset, const std::string& table) const;
    
    // Validation functions
    void validate_magic();
    void validate_architecture();
    void validate_type();
    void validate_consistency();
    
    // Symbol processing
    void process_symbols();
    uint8_t get_symbol_type(uint8_t info) const;
    uint8_t get_symbol_binding(uint8_t info) const;
    
    // Section processing
    void process_sections();
    std::string get_section_name(uint32_t name_index) const;
    
    // Reset functions
    void reset();
};
