// memory/memory.cpp
#include "memory.h"
#include <fstream>
#include <iostream>

uint32_t GBAMemory::read32(uint32_t address) const {
    address &= ~3; // Align to 4-byte boundary

    if (address >= BIOS_START && address < BIOS_START + BIOS_SIZE) {
        return *reinterpret_cast<const uint32_t*>(&bios[address]);
    } else if (address >= EWRAM_START && address < EWRAM_START + EWRAM_SIZE) {
        return *reinterpret_cast<const uint32_t*>(&ewram[address - EWRAM_START]);
    } else if (address >= IWRAM_START && address < IWRAM_START + IWRAM_SIZE) {
        return *reinterpret_cast<const uint32_t*>(&iwram[address - IWRAM_START]);
    } else if (address >= IO_START && address < IO_START + IO_SIZE) {
        return *reinterpret_cast<const uint32_t*>(&io_registers[address - IO_START]);
    } else if (address >= PALETTE_START && address < PALETTE_START + PALETTE_SIZE) {
        return *reinterpret_cast<const uint32_t*>(&palette[address - PALETTE_START]);
    } else if (address >= VRAM_START && address < VRAM_START + VRAM_SIZE) {
        return *reinterpret_cast<const uint32_t*>(&vram[address - VRAM_START]);
    } else if (address >= OAM_START && address < OAM_START + OAM_SIZE) {
        return *reinterpret_cast<const uint32_t*>(&oam[address - OAM_START]);
    } else if (address >= ROM_START && address < ROM_START + rom.size()) {
        return *reinterpret_cast<const uint32_t*>(&rom[address - ROM_START]);
    }

    return 0; // Unmapped memory
}

uint16_t GBAMemory::read16(uint32_t address) const {
    uint32_t word = read32(address & ~3);
    return (address & 2) ? (word >> 16) : (word & 0xFFFF);
}

uint8_t GBAMemory::read8(uint32_t address) const {
    uint32_t word = read32(address & ~3);
    return (word >> ((address & 3) * 8)) & 0xFF;
}

void GBAMemory::write32(uint32_t address, uint32_t value) {
    address &= ~3;

    if (address >= EWRAM_START && address < EWRAM_START + EWRAM_SIZE) {
        *reinterpret_cast<uint32_t*>(&ewram[address - EWRAM_START]) = value;
    } else if (address >= IWRAM_START && address < IWRAM_START + IWRAM_SIZE) {
        *reinterpret_cast<uint32_t*>(&iwram[address - IWRAM_START]) = value;
    } else if (address >= IO_START && address < IO_START + IO_SIZE) {
        *reinterpret_cast<uint32_t*>(&io_registers[address - IO_START]) = value;
        // TODO: Handle I/O register writes (some have special behavior)
    } else if (address >= PALETTE_START && address < PALETTE_START + PALETTE_SIZE) {
        *reinterpret_cast<uint32_t*>(&palette[address - PALETTE_START]) = value;
    } else if (address >= VRAM_START && address < VRAM_START + VRAM_SIZE) {
        *reinterpret_cast<uint32_t*>(&vram[address - VRAM_START]) = value;
    } else if (address >= OAM_START && address < OAM_START + OAM_SIZE) {
        *reinterpret_cast<uint32_t*>(&oam[address - OAM_START]) = value;
    }
}

void GBAMemory::write16(uint32_t address, uint16_t value) {
    if (address & 1) {
        // Unaligned write - handle carefully
        uint32_t aligned_addr = address & ~3;
        uint32_t old_value = read32(aligned_addr);
        uint32_t shift = ((address & 3) * 8);
        uint32_t mask = 0xFFFF << shift;
        uint32_t new_value = (old_value & ~mask) | (static_cast<uint32_t>(value) << shift);
        write32(aligned_addr, new_value);
    } else {
        uint32_t aligned_addr = address & ~3;
        uint32_t old_value = read32(aligned_addr);
        if (address & 2) {
            write32(aligned_addr, (old_value & 0xFFFF) | (static_cast<uint32_t>(value) << 16));
        } else {
            write32(aligned_addr, (old_value & 0xFFFF0000) | value);
        }
    }
}

void GBAMemory::write8(uint32_t address, uint8_t value) {
    uint32_t aligned_addr = address & ~3;
    uint32_t old_value = read32(aligned_addr);
    uint32_t shift = (address & 3) * 8;
    uint32_t mask = 0xFF << shift;
    uint32_t new_value = (old_value & ~mask) | (static_cast<uint32_t>(value) << shift);
    write32(aligned_addr, new_value);
}

bool GBAMemory::load_rom(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open ROM file " << filename << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    rom.resize(size);
    if (!file.read(reinterpret_cast<char*>(rom.data()), size)) {
        std::cerr << "Error: Could not read ROM file" << std::endl;
        return false;
    }

    std::cout << "Loaded ROM: " << size << " bytes" << std::endl;
    return true;
}

void GBAMemory::reset() {
    ewram.fill(0);
    iwram.fill(0);
    io_registers.fill(0);
    palette.fill(0);
    vram.fill(0);
    oam.fill(0);
}

bool GBAMemory::is_readable(uint32_t address) const {
    return (address >= BIOS_START && address < BIOS_START + BIOS_SIZE) ||
           (address >= EWRAM_START && address < EWRAM_START + EWRAM_SIZE) ||
           (address >= IWRAM_START && address < IWRAM_START + IWRAM_SIZE) ||
           (address >= IO_START && address < IO_START + IO_SIZE) ||
           (address >= PALETTE_START && address < PALETTE_START + PALETTE_SIZE) ||
           (address >= VRAM_START && address < VRAM_START + VRAM_SIZE) ||
           (address >= OAM_START && address < OAM_START + OAM_SIZE) ||
           (address >= ROM_START && address < ROM_START + rom.size());
}

bool GBAMemory::is_writable(uint32_t address) const {
    // BIOS and ROM are read-only
    return (address >= EWRAM_START && address < EWRAM_START + EWRAM_SIZE) ||
           (address >= IWRAM_START && address < IWRAM_START + IWRAM_SIZE) ||
           (address >= IO_START && address < IO_START + IO_SIZE) ||
           (address >= PALETTE_START && address < PALETTE_START + PALETTE_SIZE) ||
           (address >= VRAM_START && address < VRAM_START + VRAM_SIZE) ||
           (address >= OAM_START && address < OAM_START + OAM_SIZE);
}