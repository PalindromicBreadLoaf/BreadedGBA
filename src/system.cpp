// system.cpp
#include "system.h"
#include <iostream>

GBASystem::GBASystem()
    : running(false), cycles(0), interrupt_enable(0), interrupt_flags(0), interrupt_master(0) {
}

void GBASystem::init() {
    cpu.reset();
    ppu.init();
    memory.reset();
    running = false;
}

void GBASystem::reset() {
    cpu.reset();
    ppu.init();
    memory.reset();
}

bool GBASystem::load_rom(const std::string& filename) {
    return memory.load_rom(filename);
}

void GBASystem::run_frame() {
    // Run for approximately one frame (about 280,896 cycles)
    for (int i = 0; i < 280896 && running; i++) {
        cpu.step(*this);
        ppu.step(*this);
    }
}

void GBASystem::request_interrupt(int interrupt_type) {
    if (interrupt_type < 0 || interrupt_type > 13) {
        std::cerr << "Invalid interrupt type: " << interrupt_type << std::endl;
        return;
    }

    // Set the corresponding bit in the interrupt flags register
    interrupt_flags |= (1 << interrupt_type);

    #ifdef DEBUG_INTERRUPTS
    std::cout << "Interrupt requested: " << interrupt_type << " (IF: 0x"
              << std::hex << interrupt_flags << ")" << std::endl;
    #endif
}

void GBASystem::check_interrupts() {
    // Only process interrupts if master enable is set
    if (!(interrupt_master & 1)) return;

    // Check if any enabled interrupts are pending
    uint16_t pending = interrupt_flags & interrupt_enable;
    if (pending != 0) {
        handle_interrupt();
    }
}

bool GBASystem::has_pending_interrupts() const {
    if (!(interrupt_master & 1)) return false;
    return (interrupt_flags & interrupt_enable) != 0;
}

void GBASystem::handle_interrupt() {
    // Tell CPU to handle the interrupt
    cpu.handle_irq(*this);

    #ifdef DEBUG_INTERRUPTS
    std::cout << "Handling interrupt - IF: 0x" << std::hex << interrupt_flags
              << ", IE: 0x" << interrupt_enable << std::endl;
    #endif
}

// I/O Register access functions
uint8_t GBASystem::read_io_register(uint32_t address) {
    switch (address) {
        case REG_IE:
            return interrupt_enable & 0xFF;
        case REG_IE + 1:
            return (interrupt_enable >> 8) & 0xFF;
        case REG_IF:
            return interrupt_flags & 0xFF;
        case REG_IF + 1:
            return (interrupt_flags >> 8) & 0xFF;
        case REG_IME:
            return interrupt_master & 0xFF;
        case REG_IME + 1:
        case REG_IME + 2:
        case REG_IME + 3:
            return (interrupt_master >> ((address - REG_IME) * 8)) & 0xFF;

        // PPU registers
        case 0x04000000: // DISPCNT
            return ppu.dispcnt & 0xFF;
        case 0x04000001:
            return (ppu.dispcnt >> 8) & 0xFF;
        case 0x04000004: // DISPSTAT
            return ppu.dispstat & 0xFF;
        case 0x04000005:
            return (ppu.dispstat >> 8) & 0xFF;
        case 0x04000006: // VCOUNT
            return ppu.vcount & 0xFF;
        case 0x04000007:
            return (ppu.vcount >> 8) & 0xFF;

        default:
            #ifdef DEBUG_IO
            std::cout << "Unhandled I/O read from 0x" << std::hex << address << std::endl;
            #endif
            return 0;
    }
}

uint16_t GBASystem::read_io_register16(uint32_t address) {
    switch (address) {
        case REG_IE:
            return interrupt_enable;
        case REG_IF:
            return interrupt_flags;
        case REG_IME:
            return interrupt_master & 0xFFFF;

        // PPU registers
        case 0x04000000: // DISPCNT
            return ppu.dispcnt;
        case 0x04000004: // DISPSTAT
            return ppu.dispstat;
        case 0x04000006: // VCOUNT
            return ppu.vcount;

        // Background control registers
        case 0x04000008: return ppu.bg_control[0];
        case 0x0400000A: return ppu.bg_control[1];
        case 0x0400000C: return ppu.bg_control[2];
        case 0x0400000E: return ppu.bg_control[3];

        // Background scroll registers
        case 0x04000010: return ppu.bg_scroll_x[0];
        case 0x04000012: return ppu.bg_scroll_y[0];
        case 0x04000014: return ppu.bg_scroll_x[1];
        case 0x04000016: return ppu.bg_scroll_y[1];
        case 0x04000018: return ppu.bg_scroll_x[2];
        case 0x0400001A: return ppu.bg_scroll_y[2];
        case 0x0400001C: return ppu.bg_scroll_x[3];
        case 0x0400001E: return ppu.bg_scroll_y[3];

        default:
            // Try reading as two 8-bit reads
            return read_io_register(address) | (read_io_register(address + 1) << 8);
    }
}

uint32_t GBASystem::read_io_register32(uint32_t address) {
    switch (address) {
        case REG_IME:
            return interrupt_master;

        default:
            // Try reading as two 16-bit reads
            return read_io_register16(address) | (read_io_register16(address + 2) << 16);
    }
}

void GBASystem::write_io_register(uint32_t address, uint8_t value) {
    switch (address) {
        case REG_IE:
            interrupt_enable = (interrupt_enable & 0xFF00) | value;
            break;
        case REG_IE + 1:
            interrupt_enable = (interrupt_enable & 0x00FF) | (value << 8);
            break;
        case REG_IF:
            // Writing 1 to IF bits clears them (acknowledge interrupt)
            interrupt_flags &= ~value;
            break;
        case REG_IF + 1:
            interrupt_flags &= ~(value << 8);
            break;
        case REG_IME:
            interrupt_master = (interrupt_master & 0xFFFFFF00) | value;
            break;
        case REG_IME + 1:
        case REG_IME + 2:
        case REG_IME + 3: {
            int shift = (address - REG_IME) * 8;
            uint32_t mask = ~(0xFF << shift);
            interrupt_master = (interrupt_master & mask) | (value << shift);
            break;
        }

        // PPU registers
        case 0x04000000: // DISPCNT
            ppu.dispcnt = (ppu.dispcnt & 0xFF00) | value;
            break;
        case 0x04000001:
            ppu.dispcnt = (ppu.dispcnt & 0x00FF) | (value << 8);
            break;
        case 0x04000004: // DISPSTAT
            ppu.dispstat = (ppu.dispstat & 0xFF00) | (value & 0xF8); // Lower 3 bits are read-only
            break;
        case 0x04000005:
            ppu.dispstat = (ppu.dispstat & 0x00FF) | (value << 8);
            break;

        default:
            #ifdef DEBUG_IO
            std::cout << "Unhandled I/O write to 0x" << std::hex << address
                      << " = 0x" << (int)value << std::endl;
            #endif
            break;
    }
}

void GBASystem::write_io_register16(uint32_t address, uint16_t value) {
    switch (address) {
        case REG_IE:
            interrupt_enable = value;
            break;
        case REG_IF:
            // Writing 1 to IF bits clears them (acknowledge interrupt)
            interrupt_flags &= ~value;
            break;
        case REG_IME:
            interrupt_master = (interrupt_master & 0xFFFF0000) | value;
            break;

        // PPU registers
        case 0x04000000: // DISPCNT
            ppu.dispcnt = value;
            break;
        case 0x04000004: // DISPSTAT
            ppu.dispstat = (ppu.dispstat & 0x0007) | (value & 0xFFF8); // Lower 3 bits read-only
            break;

        // Background control registers
        case 0x04000008: ppu.bg_control[0] = value; break;
        case 0x0400000A: ppu.bg_control[1] = value; break;
        case 0x0400000C: ppu.bg_control[2] = value; break;
        case 0x0400000E: ppu.bg_control[3] = value; break;

        // Background scroll registers
        case 0x04000010: ppu.bg_scroll_x[0] = value; break;
        case 0x04000012: ppu.bg_scroll_y[0] = value; break;
        case 0x04000014: ppu.bg_scroll_x[1] = value; break;
        case 0x04000016: ppu.bg_scroll_y[1] = value; break;
        case 0x04000018: ppu.bg_scroll_x[2] = value; break;
        case 0x0400001A: ppu.bg_scroll_y[2] = value; break;
        case 0x0400001C: ppu.bg_scroll_x[3] = value; break;
        case 0x0400001E: ppu.bg_scroll_y[3] = value; break;

        default:
            // Try writing as two 8-bit writes
            write_io_register(address, value & 0xFF);
            write_io_register(address + 1, (value >> 8) & 0xFF);
            break;
    }
}

void GBASystem::write_io_register32(uint32_t address, uint32_t value) {
    switch (address) {
        case REG_IME:
            interrupt_master = value;
            break;

        default:
            // Try writing as two 16-bit writes
            write_io_register16(address, value & 0xFFFF);
            write_io_register16(address + 2, (value >> 16) & 0xFFFF);
            break;
    }
}