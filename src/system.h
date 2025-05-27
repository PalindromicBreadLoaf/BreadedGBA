// system.h
#pragma once

#include "cpu/arm7_cpu.h"
#include "memory/memory.h"
#include "ppu/ppu.h"
#include <cstdint>

enum GBAInterrupt {
    IRQ_VBLANK = 0,     // V-Blank
    IRQ_HBLANK = 1,     // H-Blank
    IRQ_VCOUNT = 2,     // V-Counter Match
    IRQ_TIMER0 = 3,     // Timer 0 Overflow
    IRQ_TIMER1 = 4,     // Timer 1 Overflow
    IRQ_TIMER2 = 5,     // Timer 2 Overflow
    IRQ_TIMER3 = 6,     // Timer 3 Overflow
    IRQ_SERIAL = 7,     // Serial Communication
    IRQ_DMA0 = 8,       // DMA 0
    IRQ_DMA1 = 9,       // DMA 1
    IRQ_DMA2 = 10,      // DMA 2
    IRQ_DMA3 = 11,      // DMA 3
    IRQ_KEYPAD = 12,    // Keypad
    IRQ_GAMEPAK = 13    // Game Pak (external IRQ)
};

// I/O Register addresses for interrupts
constexpr uint32_t REG_IE = 0x04000200;    // Interrupt Enable
constexpr uint32_t REG_IF = 0x04000202;    // Interrupt Request Flags
constexpr uint32_t REG_IME = 0x04000208;   // Interrupt Master Enable

class GBASystem {
public:
    ARM7CPU cpu;
    GBAMemory memory;
    GBAPPU ppu;

    // System State
    bool running = false;
    uint64_t cycles;

    // Interrupt system
    uint16_t interrupt_enable;      // IE register
    uint16_t interrupt_flags;       // IF register
    uint32_t interrupt_master;      // IME register

    GBASystem();

    void init();
    void reset();
    bool load_rom(const std::string& filename);
    void run_frame();

    void request_interrupt(int interrupt_type);
    void check_interrupts();
    [[nodiscard]] bool has_pending_interrupts() const;

    // I/O register access
    uint8_t read_io_register(uint32_t address);
    uint16_t read_io_register16(uint32_t address);
    uint32_t read_io_register32(uint32_t address);
    void write_io_register(uint32_t address, uint8_t value);
    void write_io_register16(uint32_t address, uint16_t value);
    void write_io_register32(uint32_t address, uint32_t value);

private:
    void handle_interrupt();
};