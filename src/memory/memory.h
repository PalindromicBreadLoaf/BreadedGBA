// memory/memory.h
#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <string>

// Memory Map Constants
constexpr size_t BIOS_SIZE = 0x4000;      // 16KB BIOS
constexpr size_t EWRAM_SIZE = 0x40000;    // 256KB External Work RAM
constexpr size_t IWRAM_SIZE = 0x8000;     // 32KB Internal Work RAM
constexpr size_t IO_SIZE = 0x400;         // 1KB I/O Registers
constexpr size_t PALETTE_SIZE = 0x400;    // 1KB Palette RAM
constexpr size_t VRAM_SIZE = 0x18000;     // 96KB Video RAM
constexpr size_t OAM_SIZE = 0x400;        // 1KB Object Attribute Memory
constexpr size_t ROM_SIZE = 0x2000000;    // 32MB ROM space

// Memory regions
constexpr uint32_t BIOS_START = 0x00000000;
constexpr uint32_t EWRAM_START = 0x02000000;
constexpr uint32_t IWRAM_START = 0x03000000;
constexpr uint32_t IO_START = 0x04000000;
constexpr uint32_t PALETTE_START = 0x05000000;
constexpr uint32_t VRAM_START = 0x06000000;
constexpr uint32_t OAM_START = 0x07000000;
constexpr uint32_t ROM_START = 0x08000000;

// Memory Management Unit
class GBAMemory {
public:
    std::array<uint8_t, BIOS_SIZE> bios{};
    std::array<uint8_t, EWRAM_SIZE> ewram{};
    std::array<uint8_t, IWRAM_SIZE> iwram{};
    std::array<uint8_t, IO_SIZE> io_registers{};
    std::array<uint8_t, PALETTE_SIZE> palette{};
    std::array<uint8_t, VRAM_SIZE> vram{};
    std::array<uint8_t, OAM_SIZE> oam{};
    std::vector<uint8_t> rom;

    uint32_t read32(uint32_t address) const;
    uint16_t read16(uint32_t address) const;
    uint8_t read8(uint32_t address) const;
    void write32(uint32_t address, uint32_t value);
    void write16(uint32_t address, uint16_t value);
    void write8(uint32_t address, uint8_t value);
    bool load_rom(const std::string& filename);
    void reset();

private:
    // Helper functions for memory region detection
    bool is_readable(uint32_t address) const;
    bool is_writable(uint32_t address) const;
};