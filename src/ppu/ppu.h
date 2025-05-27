// ppu/ppu.h
#pragma once

#include <array>
#include <cstdint>

// Forward declaration
class GBASystem;

// PPU Constants
constexpr int GBA_SCREEN_WIDTH = 240;
constexpr int GBA_SCREEN_HEIGHT = 160;
constexpr int DOTS_PER_SCANLINE = 308;
constexpr int TOTAL_SCANLINES = 228;

// Display Control Register bits
constexpr uint16_t DISPCNT_BG_MODE_MASK = 0x0007;
constexpr uint16_t DISPCNT_CGBMODE = 0x0008;
constexpr uint16_t DISPCNT_DISPLAY_FRAME = 0x0010;
constexpr uint16_t DISPCNT_HBLANK_INTERVAL_FREE = 0x0020;
constexpr uint16_t DISPCNT_OBJ_CHAR_VRAM_MAP = 0x0040;
constexpr uint16_t DISPCNT_FORCED_BLANK = 0x0080;
constexpr uint16_t DISPCNT_SCREEN_DISPLAY_BG0 = 0x0100;
constexpr uint16_t DISPCNT_SCREEN_DISPLAY_BG1 = 0x0200;
constexpr uint16_t DISPCNT_SCREEN_DISPLAY_BG2 = 0x0400;
constexpr uint16_t DISPCNT_SCREEN_DISPLAY_BG3 = 0x0800;
constexpr uint16_t DISPCNT_SCREEN_DISPLAY_OBJ = 0x1000;
constexpr uint16_t DISPCNT_WINDOW_0_DISPLAY = 0x2000;
constexpr uint16_t DISPCNT_WINDOW_1_DISPLAY = 0x4000;
constexpr uint16_t DISPCNT_OBJ_WINDOW_DISPLAY = 0x8000;

// PPU (Picture Processing Unit) Class
class GBAPPU {
public:
    uint16_t dispcnt = 0;          // Display Control
    uint16_t dispstat = 0;         // Display Status
    uint16_t vcount = 0;           // Vertical Counter
    std::array<uint16_t, 4> bg_control{};    // Background Control
    std::array<uint16_t, 4> bg_scroll_x{};   // Background X Scroll
    std::array<uint16_t, 4> bg_scroll_y{};   // Background Y Scroll
    int scanline = 0;
    int dot = 0;
    std::array<uint32_t, GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT> framebuffer{};

    void init();
    void step(GBASystem& gba);
    void render_scanline(GBASystem& gba);

private:
    // Rendering helper functions
    void render_background_mode0(GBASystem& gba, int line);
    void render_background_mode1(GBASystem& gba, int line);
    void render_background_mode2(GBASystem& gba, int line);
    void render_background_mode3(GBASystem& gba, int line);
    void render_background_mode4(GBASystem& gba, int line);
    void render_background_mode5(GBASystem& gba, int line);
    void render_sprites(GBASystem& gba, int line);

    // Color conversion
    uint32_t convert_color(uint16_t gba_color) const;

    // Background rendering helpers
    void render_text_background(GBASystem& gba, int bg_num, int line);
    void render_affine_background(GBASystem& gba, int bg_num, int line);
};