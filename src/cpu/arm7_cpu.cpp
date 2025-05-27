// cpu/arm7_cpu.cpp
#include "arm7_cpu.h"
#include "../system.h"
#include <iostream>

void ARM7CPU::init() {
    reset();
}

void ARM7CPU::reset() {
    registers.fill(0);
    registers[15] = 0x08000000;  // PC starts at ROM
    cpsr = static_cast<uint32_t>(CpuMode::SYSTEM);
    spsr.fill(0);
    banked_r13.fill(0);
    banked_r14.fill(0);
    banked_r8_r12.fill(0);
    thumb_mode = false;
    cycles = 0;

    // Initialize stack pointers for different modes
    // These are typical GBA stack pointer values
    banked_r13[get_mode_index(CpuMode::USER)] = 0x03007F00;       // User/System mode SP
    banked_r13[get_mode_index(CpuMode::IRQ)] = 0x03007FA0;        // IRQ mode SP
    banked_r13[get_mode_index(CpuMode::FIQ)] = 0x03007FE0;        // FIQ mode SP
    banked_r13[get_mode_index(CpuMode::SUPERVISOR)] = 0x03007FE0; // Supervisor mode SP
    banked_r13[get_mode_index(CpuMode::ABORT)] = 0x03007FE0;      // Abort mode SP
    banked_r13[get_mode_index(CpuMode::UNDEFINED)] = 0x03007FE0;  // Undefined mode SP
}

void ARM7CPU::step(GBASystem& gba) {
    // Check if IRQs are enabled and if there are pending interrupts
    if (!(cpsr & FLAG_I) && gba.has_pending_interrupts()) {
        handle_irq(gba);
        return;
    }

    if (thumb_mode) {
        uint16_t instruction = gba.memory.read16(registers[15]);
        registers[15] += 2;
        execute_thumb(gba, instruction);
    } else {
        uint32_t instruction = gba.memory.read32(registers[15]);
        registers[15] += 4;
        execute_arm(gba, instruction);
    }
    cycles++;
}

void ARM7CPU::handle_irq(GBASystem& gba) {
    #ifdef DEBUG_INTERRUPTS
    std::cout << "CPU: Handling IRQ interrupt" << std::endl;
    #endif

    CpuMode prior_mode = get_current_mode();

    // Save current CPSR to SPSR_irq
    spsr[get_mode_index(CpuMode::IRQ)] = cpsr;

    // Save return address in LR_irq
    uint32_t return_address;
    if (thumb_mode) {
        return_address = registers[15] - 2;  // Thumb
    } else {
        return_address = registers[15] - 4;  // ARM
    }

    banked_r14[get_mode_index(CpuMode::IRQ)] = return_address;

    switch_mode(CpuMode::IRQ);

    // Disable IRQs and clear Thumb mode
    cpsr |= FLAG_I;  // Disable IRQs
    cpsr &= ~FLAG_T; // Clear Thumb mode
    thumb_mode = false;

    registers[15] = VECTOR_IRQ;

    flush_pipeline();

    cycles += 3; // IRQ handling takes 3 cycles
}

void ARM7CPU::handle_fiq(GBASystem& gba) {
    #ifdef DEBUG_INTERRUPTS
    std::cout << "CPU: Handling FIQ interrupt" << std::endl;
    #endif

    // Save current mode
    CpuMode prior_mode = get_current_mode();

    // Save current CPSR to SPSR_fiq
    spsr[get_mode_index(CpuMode::FIQ)] = cpsr;

    // Save return address in LR_fiq
    uint32_t return_address;
    if (thumb_mode) {
        return_address = registers[15] - 2;
    } else {
        return_address = registers[15] - 4;
    }

    banked_r14[get_mode_index(CpuMode::FIQ)] = return_address;

    switch_mode(CpuMode::FIQ);

    // Disable both FIQs and IRQs, clear Thumb mode
    cpsr |= (FLAG_F | FLAG_I);
    cpsr &= ~FLAG_T;
    thumb_mode = false;

    registers[15] = VECTOR_FIQ;

    flush_pipeline();
    cycles += 3;
}

void ARM7CPU::switch_mode(CpuMode new_mode) {
    const CpuMode prior_mode = get_current_mode();

    if (prior_mode == new_mode) return;

    save_banked_registers(prior_mode);

    // Update mode bits in CPSR
    cpsr = (cpsr & ~0x1F) | static_cast<uint32_t>(new_mode);

    // Restore banked registers for new mode
    restore_banked_registers(new_mode);
}

CpuMode ARM7CPU::get_current_mode() const {
    return static_cast<CpuMode>(cpsr & 0x1F);
}

int ARM7CPU::get_mode_index(CpuMode mode) const {
    switch (mode) {
        case CpuMode::USER:
        case CpuMode::SYSTEM:
            return 0;
        case CpuMode::FIQ:
            return 1;
        case CpuMode::IRQ:
            return 2;
        case CpuMode::SUPERVISOR:
            return 3;
        case CpuMode::ABORT:
            return 4;
        case CpuMode::UNDEFINED:
            return 5;
        default:
            return 0;
    }
}

void ARM7CPU::save_banked_registers(CpuMode prior_mode) {
    int mode_index = get_mode_index(prior_mode);

    // Save R13 (SP) and R14 (LR)
    banked_r13[mode_index] = registers[13];
    banked_r14[mode_index] = registers[14];

    // FIQ mode also banks R8-R12
    if (prior_mode == CpuMode::FIQ) {
        for (int i = 0; i < 5; i++) {
            banked_r8_r12[i] = registers[8 + i];
        }
    }
}

void ARM7CPU::restore_banked_registers(CpuMode new_mode) {
    int mode_index = get_mode_index(new_mode);

    // Restore R13 (SP) and R14 (LR)
    registers[13] = banked_r13[mode_index];
    registers[14] = banked_r14[mode_index];

    // FIQ mode also banks R8-R12
    if (new_mode == CpuMode::FIQ) {
        for (int i = 0; i < 5; i++) {
            registers[8 + i] = banked_r8_r12[i];
        }
    } else {
        // Restore normal R8-R12 registers
        for (int i = 0; i < 5; i++) {
            registers[8 + i] = banked_r8_r12[5 + i]; // Second half stores normal registers
        }
    }
}

void ARM7CPU::flush_pipeline() {
    // TODO: Actually flush the pipeline
}

void ARM7CPU::execute_arm(GBASystem& gba, uint32_t instruction) {
    uint32_t condition = (instruction >> 28) & 0xF;
    if (!check_condition(condition)) {
        return;
    }

    // TODO: Basic instruction decoding
    uint32_t opcode = (instruction >> 21) & 0xF;

    if ((instruction & 0x0FB00FF0) == 0x01000090) {
        // SWP instruction (atomic swap)
        // TODO: Implement
        std::cout << "SWP instruction: 0x" << std::hex << instruction << std::endl;
    } else if ((instruction & 0x0F000000) == 0x0F000000) {
        // Software Interrupt (SWI)
        std::cout << "SWI instruction: 0x" << std::hex << instruction << std::endl;
        // TODO: Handle SWI
    } else {
        // TODO: Implement full ARM instruction set
        std::cout << "ARM instruction: 0x" << std::hex << instruction << std::endl;
    }
}

void ARM7CPU::execute_thumb(GBASystem& gba, uint16_t instruction) {
    // TODO: Basic Thumb instruction decoding
    if ((instruction & 0xFF00) == 0xDF00) {
        // Software Interrupt in Thumb mode
        std::cout << "Thumb SWI instruction: 0x" << std::hex << instruction << std::endl;
        // TODO: Handle Thumb SWI
    } else {
        // TODO: Implement full Thumb instruction set
        std::cout << "Thumb instruction: 0x" << std::hex << instruction << std::endl;
    }
}

void ARM7CPU::set_flags(uint32_t result, bool carry, bool overflow) {
    cpsr &= ~(FLAG_N | FLAG_Z | FLAG_C | FLAG_V);

    if (result == 0) cpsr |= FLAG_Z;
    if (result & 0x80000000) cpsr |= FLAG_N;
    if (carry) cpsr |= FLAG_C;
    if (overflow) cpsr |= FLAG_V;
}

bool ARM7CPU::check_condition(uint32_t condition) {
    switch (condition) {
        case 0x0: return (cpsr & FLAG_Z) != 0;                // EQ - Equal
        case 0x1: return (cpsr & FLAG_Z) == 0;                // NE - Not Equal
        case 0x2: return (cpsr & FLAG_C) != 0;                // CS - Carry Set
        case 0x3: return (cpsr & FLAG_C) == 0;                // CC - Carry Clear
        case 0x4: return (cpsr & FLAG_N) != 0;                // MI - Minus
        case 0x5: return (cpsr & FLAG_N) == 0;                // PL - Plus
        case 0x6: return (cpsr & FLAG_V) != 0;                // VS - Overflow Set
        case 0x7: return (cpsr & FLAG_V) == 0;                // VC - Overflow Clear
        case 0x8: return (cpsr & FLAG_C) && !(cpsr & FLAG_Z); // HI - Higher
        case 0x9: return !(cpsr & FLAG_C) || (cpsr & FLAG_Z); // LS - Lower or Same
        case 0xA: return !!(cpsr & FLAG_N) == !!(cpsr & FLAG_V); // GE - Greater or Equal
        case 0xB: return !!(cpsr & FLAG_N) != !!(cpsr & FLAG_V); // LT - Less Than
        case 0xC: return !(cpsr & FLAG_Z) && (!!(cpsr & FLAG_N) == !!(cpsr & FLAG_V)); // GT - Greater Than
        case 0xD: return (cpsr & FLAG_Z) || (!!(cpsr & FLAG_N) != !!(cpsr & FLAG_V));  // LE - Less or Equal
        case 0xE: return true;                        // AL - Always
        case 0xF: return false;                       // NV - Never
        default: return false;
    }
}