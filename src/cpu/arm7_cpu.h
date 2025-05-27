// cpu/arm7_cpu.h
#pragma once

#include <array>
#include <cstdint>

// Forward declaration
class GBASystem;

// CPU Modes
enum class CpuMode : uint32_t {
    USER = 0x10,
    FIQ = 0x11,
    IRQ = 0x12,
    SUPERVISOR = 0x13,
    ABORT = 0x17,
    UNDEFINED = 0x1B,
    SYSTEM = 0x1F
};

// CPU State Flags
constexpr uint32_t FLAG_N = (1 << 31);  // Negative
constexpr uint32_t FLAG_Z = (1 << 30);  // Zero
constexpr uint32_t FLAG_C = (1 << 29);  // Carry
constexpr uint32_t FLAG_V = (1 << 28);  // Overflow
constexpr uint32_t FLAG_I = (1 << 7);   // IRQ disable
constexpr uint32_t FLAG_F = (1 << 6);   // FIQ disable
constexpr uint32_t FLAG_T = (1 << 5);   // Thumb mode

// Exception Vector Addresses
constexpr uint32_t VECTOR_RESET = 0x00000000;
constexpr uint32_t VECTOR_UNDEFINED = 0x00000004;
constexpr uint32_t VECTOR_SWI = 0x00000008;
constexpr uint32_t VECTOR_PREFETCH_ABORT = 0x0000000C;
constexpr uint32_t VECTOR_DATA_ABORT = 0x00000010;
constexpr uint32_t VECTOR_IRQ = 0x00000018;
constexpr uint32_t VECTOR_FIQ = 0x0000001C;

// ARM7TDMI CPU Class
class ARM7CPU {
public:
    std::array<uint32_t, 16> registers{};     // R0-R15 (R15 is PC)
    uint32_t cpsr = 0;                        // Current Program Status Register
    std::array<uint32_t, 5> spsr{};           // Saved Program Status Registers
    std::array<uint32_t, 6> banked_r13{};     // Banked R13 (SP) registers
    std::array<uint32_t, 6> banked_r14{};     // Banked R14 (LR) registers
    std::array<uint32_t, 10> banked_r8_r12{}; // Banked R8-R12 for FIQ mode
    bool thumb_mode = false;
    int cycles = 0;

    void init();
    void reset();
    void step(GBASystem& gba);
    void execute_arm(GBASystem& gba, uint32_t instruction);
    void execute_thumb(GBASystem& gba, uint16_t instruction);

    // Interrupt handling
    void handle_irq(GBASystem& gba);
    void handle_fiq(GBASystem& gba);

private:
    // Mode and register management
    void switch_mode(CpuMode new_mode);
    CpuMode get_current_mode() const;
    int get_mode_index(CpuMode mode) const;
    void save_banked_registers(CpuMode prior_mode);
    void restore_banked_registers(CpuMode new_mode);

    // Helper functions for instruction decoding
    void set_flags(uint32_t result, bool carry = false, bool overflow = false);
    bool check_condition(uint32_t condition);

    // Pipeline management
    void flush_pipeline();
};