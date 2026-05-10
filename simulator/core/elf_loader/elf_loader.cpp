#include "elf_loader.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cstring>

ElfLoader::ElfLoader() {
    reset();
}

ElfLoader::~ElfLoader() {
    // Cleanup handled by STL containers
}

bool ElfLoader::load_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Read entire file
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    elf_data.resize(file_size);
    file.read(reinterpret_cast<char*>(elf_data.data()), file_size);
    
    if (!file.good()) {
        reset();
        return false;
    }
    
    try {
        parse_header();
        parse_program_headers();
        parse_section_headers();
        parse_string_tables();
        parse_symbol_table();
        parse_section_data();
        process_sections();
        process_symbols();
        validate_consistency();
        
        return true;
    } catch (const ElfLoadException& e) {
        std::cerr << "ELF load error: " << e.what() << std::endl;
        reset();
        return false;
    }
}

bool ElfLoader::load_memory(const std::vector<uint8_t>& data) {
    elf_data = data;
    
    try {
        parse_header();
        parse_program_headers();
        parse_section_headers();
        parse_string_tables();
        parse_symbol_table();
        parse_section_data();
        process_sections();
        process_symbols();
        validate_consistency();
        
        return true;
    } catch (const ElfLoadException& e) {
        std::cerr << "ELF load error: " << e.what() << std::endl;
        reset();
        return false;
    }
}

bool ElfLoader::is_valid_elf() const {
    return header.e_magic == ELF_MAGIC;
}

bool ElfLoader::is_xtensa_elf() const {
    return header.e_machine == EM_XTENSA;
}

bool ElfLoader::is_executable() const {
    return header.e_type == ET_EXEC || header.e_type == ET_DYN;
}

void ElfLoader::parse_header() {
    if (elf_data.size() < sizeof(Elf32Header)) {
        throw ElfLoadException("ELF file too small for header");
    }
    
    validate_magic();
    
    header.e_magic = read_uint32(0);
    header.e_class = read_uint8(4);
    header.e_data = read_uint8(5);
    header.e_version = read_uint8(6);
    header.e_osabi = read_uint8(7);
    header.e_abiversion = read_uint8(8);
    header.e_type = read_uint16(16);
    header.e_machine = read_uint16(18);
    header.e_version = read_uint32(20);
    header.e_entry = read_uint32(24);
    header.e_phoff = read_uint32(28);
    header.e_shoff = read_uint32(32);
    header.e_flags = read_uint32(36);
    header.e_ehsize = read_uint16(40);
    header.e_phentsize = read_uint16(42);
    header.e_phnum = read_uint16(44);
    header.e_shentsize = read_uint16(46);
    header.e_shnum = read_uint16(48);
    header.e_shstrndx = read_uint16(50);
    
    validate_architecture();
    validate_type();
}

void ElfLoader::parse_program_headers() {
    if (header.e_phoff == 0 || header.e_phnum == 0) {
        return;
    }
    
    if (header.e_phentsize != sizeof(Elf32ProgramHeader)) {
        throw ElfLoadException("Invalid program header size");
    }
    
    program_headers.resize(header.e_phnum);
    
    for (uint32_t i = 0; i < header.e_phnum; i++) {
        size_t offset = header.e_phoff + i * header.e_phentsize;
        
        if (offset + sizeof(Elf32ProgramHeader) > elf_data.size()) {
            throw ElfLoadException("Program header extends beyond file");
        }
        
        Elf32ProgramHeader& ph = program_headers[i];
        ph.p_type = read_uint32(offset);
        ph.p_offset = read_uint32(offset + 4);
        ph.p_vaddr = read_uint32(offset + 8);
        ph.p_paddr = read_uint32(offset + 12);
        ph.p_filesz = read_uint32(offset + 16);
        ph.p_memsz = read_uint32(offset + 20);
        ph.p_flags = read_uint32(offset + 24);
        ph.p_align = read_uint32(offset + 28);
    }
}

void ElfLoader::parse_section_headers() {
    if (header.e_shoff == 0 || header.e_shnum == 0) {
        return;
    }
    
    if (header.e_shentsize != sizeof(Elf32SectionHeader)) {
        throw ElfLoadException("Invalid section header size");
    }
    
    section_headers.resize(header.e_shnum);
    
    for (uint32_t i = 0; i < header.e_shnum; i++) {
        size_t offset = header.e_shoff + i * header.e_shentsize;
        
        if (offset + sizeof(Elf32SectionHeader) > elf_data.size()) {
            throw ElfLoadException("Section header extends beyond file");
        }
        
        Elf32SectionHeader& sh = section_headers[i];
        sh.sh_name = read_uint32(offset);
        sh.sh_type = read_uint32(offset + 4);
        sh.sh_flags = read_uint32(offset + 8);
        sh.sh_addr = read_uint32(offset + 12);
        sh.sh_offset = read_uint32(offset + 16);
        sh.sh_size = read_uint32(offset + 20);
        sh.sh_link = read_uint32(offset + 24);
        sh.sh_info = read_uint32(offset + 28);
        sh.sh_addralign = read_uint32(offset + 32);
        sh.sh_entsize = read_uint32(offset + 36);
    }
}

void ElfLoader::parse_string_tables() {
    // Parse section header string table
    if (header.e_shstrndx < section_headers.size()) {
        const Elf32SectionHeader& sh = section_headers[header.e_shstrndx];
        if (sh.sh_type == SHT_STRTAB && sh.sh_offset > 0 && sh.sh_size > 0) {
            if (sh.sh_offset + sh.sh_size <= elf_data.size()) {
                shstrtab.assign(reinterpret_cast<const char*>(elf_data.data() + sh.sh_offset), sh.sh_size);
            }
        }
    }
    
    // Parse symbol string table
    for (const auto& sh : section_headers) {
        if (sh.sh_type == SHT_STRTAB && sh.sh_link < section_headers.size()) {
            if (sh.sh_offset > 0 && sh.sh_size > 0 && sh.sh_offset + sh.sh_size <= elf_data.size()) {
                // Check if this is the .strtab section (not .shstrtab)
                if (sh.sh_link == header.e_shstrndx) {
                    strtab.assign(reinterpret_cast<const char*>(elf_data.data() + sh.sh_offset), sh.sh_size);
                    break;
                }
            }
        }
    }
}

void ElfLoader::parse_symbol_table() {
    for (const auto& sh : section_headers) {
        if (sh.sh_type == SHT_SYMTAB && sh.sh_entsize == sizeof(Elf32Symbol)) {
            if (sh.sh_offset + sh.sh_size <= elf_data.size()) {
                uint32_t symbol_count = sh.sh_size / sh.sh_entsize;
                
                for (uint32_t i = 0; i < symbol_count; i++) {
                    size_t offset = sh.sh_offset + i * sh.sh_entsize;
                    
                    Elf32Symbol sym;
                    sym.st_name = read_uint32(offset);
                    sym.st_value = read_uint32(offset + 4);
                    sym.st_size = read_uint32(offset + 8);
                    sym.st_info = read_uint8(offset + 12);
                    sym.st_other = read_uint8(offset + 13);
                    sym.st_shndx = read_uint16(offset + 14);
                    
                    SymbolInfo symbol_info;
                    symbol_info.name = read_string_from_table(sym.st_name, strtab);
                    symbol_info.address = sym.st_value;
                    symbol_info.size = sym.st_size;
                    symbol_info.type = get_symbol_type(sym.st_info);
                    symbol_info.binding = get_symbol_binding(sym.st_info);
                    symbol_info.section = sym.st_shndx;
                    
                    symbols.push_back(symbol_info);
                }
            }
        }
    }
}

void ElfLoader::parse_section_data() {
    sections.resize(section_headers.size());
    
    for (uint32_t i = 0; i < section_headers.size(); i++) {
        const Elf32SectionHeader& sh = section_headers[i];
        SectionInfo& section = sections[i];
        
        section.name = get_section_name(sh.sh_name);
        section.address = sh.sh_addr;
        section.size = sh.sh_size;
        section.offset = sh.sh_offset;
        section.type = sh.sh_type;
        section.flags = sh.sh_flags;
        
        // Load section data if it exists and has content
        if (sh.sh_offset > 0 && sh.sh_size > 0 && sh.sh_type != SHT_NOBITS) {
            if (sh.sh_offset + sh.sh_size <= elf_data.size()) {
                section.data.resize(sh.sh_size);
                std::copy(elf_data.begin() + sh.sh_offset, 
                         elf_data.begin() + sh.sh_offset + sh.sh_size,
                         section.data.begin());
            }
        }
    }
}

void ElfLoader::process_sections() {
    stats.total_sections = sections.size();
    stats.text_size = 0;
    stats.data_size = 0;
    stats.bss_size = 0;
    
    for (const auto& section : sections) {
        if (section.name == ".text") {
            stats.text_size = section.size;
        } else if (section.name == ".data") {
            stats.data_size = section.size;
        } else if (section.name == ".bss") {
            stats.bss_size = section.size;
        }
    }
}

void ElfLoader::process_symbols() {
    stats.symbol_count = symbols.size();
}

void ElfLoader::validate_magic() {
    if (read_uint32(0) != ELF_MAGIC) {
        throw ElfLoadException("Invalid ELF magic number");
    }
}

void ElfLoader::validate_architecture() {
    if (header.e_machine != EM_XTENSA) {
        throw ElfLoadException("Not an Xtensa ELF file");
    }
}

void ElfLoader::validate_type() {
    if (header.e_type != ET_EXEC && header.e_type != ET_DYN) {
        throw ElfLoadException("Not an executable or shared object");
    }
}

void ElfLoader::validate_consistency() {
    // Validate program headers
    for (const auto& ph : program_headers) {
        if (ph.p_offset + ph.p_filesz > elf_data.size()) {
            throw ElfLoadException("Program header extends beyond file");
        }
    }
    
    // Validate section headers
    for (const auto& sh : section_headers) {
        if (sh.sh_offset + sh.sh_size > elf_data.size()) {
            throw ElfLoadException("Section header extends beyond file");
        }
    }
    
    // Update statistics
    stats.total_segments = program_headers.size();
    stats.loadable_segments = 0;
    
    for (const auto& ph : program_headers) {
        if (ph.p_type == 1) { // PT_LOAD
            stats.loadable_segments++;
        }
    }
}

const SymbolInfo* ElfLoader::find_symbol(const std::string& name) const {
    for (const auto& symbol : symbols) {
        if (symbol.name == name) {
            return &symbol;
        }
    }
    return nullptr;
}

const SymbolInfo* ElfLoader::find_symbol_by_address(uint32_t address) const {
    const SymbolInfo* best_match = nullptr;
    
    for (const auto& symbol : symbols) {
        if (symbol.address <= address && 
            (symbol.address + symbol.size > address || symbol.size == 0)) {
            if (!best_match || symbol.size > best_match->size) {
                best_match = &symbol;
            }
        }
    }
    
    return best_match;
}

const SectionInfo* ElfLoader::find_section(const std::string& name) const {
    for (const auto& section : sections) {
        if (section.name == name) {
            return &section;
        }
    }
    return nullptr;
}

const SectionInfo* ElfLoader::find_section_by_address(uint32_t address) const {
    for (const auto& section : sections) {
        if (section.address <= address && section.address + section.size > address) {
            return &section;
        }
    }
    return nullptr;
}

void ElfLoader::load_segments_to_memory(std::function<void(uint32_t, const std::vector<uint8_t>&)> write_memory) {
    for (const auto& ph : program_headers) {
        if (ph.p_type == 1) { // PT_LOAD
            if (ph.p_filesz > 0) {
                if (ph.p_offset + ph.p_filesz <= elf_data.size()) {
                    std::vector<uint8_t> segment_data(elf_data.begin() + ph.p_offset,
                                                     elf_data.begin() + ph.p_offset + ph.p_filesz);
                    write_memory(ph.p_vaddr, segment_data);
                }
            }
            
            // Zero-fill BSS portion
            if (ph.p_memsz > ph.p_filesz) {
                std::vector<uint8_t> bss_data(ph.p_memsz - ph.p_filesz, 0);
                write_memory(ph.p_vaddr + ph.p_filesz, bss_data);
            }
        }
    }
}

void ElfLoader::load_section_to_memory(const std::string& section_name, 
                                      std::function<void(uint32_t, const std::vector<uint8_t>&)> write_memory) {
    const SectionInfo* section = find_section(section_name);
    if (section && !section->data.empty()) {
        write_memory(section->address, section->data);
    }
}

uint32_t ElfLoader::read_uint32(size_t offset) const {
    if (offset + 4 > elf_data.size()) {
        return 0;
    }
    return static_cast<uint32_t>(elf_data[offset]) |
           (static_cast<uint32_t>(elf_data[offset + 1]) << 8) |
           (static_cast<uint32_t>(elf_data[offset + 2]) << 16) |
           (static_cast<uint32_t>(elf_data[offset + 3]) << 24);
}

uint16_t ElfLoader::read_uint16(size_t offset) const {
    if (offset + 2 > elf_data.size()) {
        return 0;
    }
    return static_cast<uint16_t>(elf_data[offset]) |
           (static_cast<uint16_t>(elf_data[offset + 1]) << 8);
}

uint8_t ElfLoader::read_uint8(size_t offset) const {
    if (offset >= elf_data.size()) {
        return 0;
    }
    return elf_data[offset];
}

std::string ElfLoader::read_string(size_t offset) const {
    std::string result;
    while (offset < elf_data.size() && elf_data[offset] != 0) {
        result += static_cast<char>(elf_data[offset]);
        offset++;
    }
    return result;
}

std::string ElfLoader::read_string_from_table(uint32_t offset, const std::string& table) const {
    if (offset >= table.size()) {
        return "";
    }
    
    const char* str = table.c_str() + offset;
    return std::string(str);
}

uint8_t ElfLoader::get_symbol_type(uint8_t info) const {
    return info & 0x0F;
}

uint8_t ElfLoader::get_symbol_binding(uint8_t info) const {
    return (info >> 4) & 0x0F;
}

std::string ElfLoader::get_section_name(uint32_t name_index) const {
    return read_string_from_table(name_index, shstrtab);
}

void ElfLoader::reset() {
    elf_data.clear();
    memset(&header, 0, sizeof(header));
    program_headers.clear();
    section_headers.clear();
    symbols.clear();
    sections.clear();
    shstrtab.clear();
    strtab.clear();
    stats = {};
}

void ElfLoader::print_header() const {
    std::cout << "ELF Header:\n";
    std::cout << "  Magic: 0x" << std::hex << header.e_magic << "\n";
    std::cout << "  Type: " << std::dec << header.e_type << "\n";
    std::cout << "  Machine: " << header.e_machine << " (Xtensa)\n";
    std::cout << "  Entry: 0x" << std::hex << header.e_entry << "\n";
    std::cout << "  Program Headers: " << std::dec << header.e_phnum << "\n";
    std::cout << "  Section Headers: " << header.e_shnum << "\n";
}

void ElfLoader::print_program_headers() const {
    std::cout << "Program Headers:\n";
    for (const auto& ph : program_headers) {
        std::cout << "  Type: " << std::dec << ph.p_type;
        std::cout << ", VAddr: 0x" << std::hex << ph.p_vaddr;
        std::cout << ", Size: 0x" << ph.p_memsz << "\n";
    }
}

void ElfLoader::print_section_headers() const {
    std::cout << "Section Headers:\n";
    for (const auto& sh : section_headers) {
        std::cout << "  " << get_section_name(sh.sh_name);
        std::cout << ": Type: " << std::dec << sh.sh_type;
        std::cout << ", Addr: 0x" << std::hex << sh.sh_addr;
        std::cout << ", Size: 0x" << sh.sh_size << "\n";
    }
}

void ElfLoader::print_symbols() const {
    std::cout << "Symbols:\n";
    for (const auto& sym : symbols) {
        std::cout << "  " << sym.name;
        std::cout << ": 0x" << std::hex << sym.address;
        std::cout << " (size: " << std::dec << sym.size << ")\n";
    }
}

void ElfLoader::print_sections() const {
    std::cout << "Sections:\n";
    for (const auto& section : sections) {
        std::cout << "  " << section.name;
        std::cout << ": 0x" << std::hex << section.address;
        std::cout << "-0x" << (section.address + section.size);
        std::cout << " (size: " << std::dec << section.size << ")\n";
    }
}
