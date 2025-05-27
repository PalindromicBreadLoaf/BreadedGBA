// ppu/ppu.cpp
#include "ppu.h"
#include "../system.h"
#include "../memory/memory.h"

// PPU Status Register bits
constexpr uint16_t DISPSTAT_VBLANK = 0x0001;
constexpr uint16_t DISPSTAT_HBLANK = 0x0002;
constexpr uint16_t DISPSTAT_VCOUNT_MATCH = 0x0004;
constexpr uint16_t DISPSTAT_VBLANK_IRQ_ENABLE = 0x0008;
constexpr uint16_t DISPSTAT_HBLANK_IRQ_ENABLE = 0x0010;
constexpr uint16_t DISPSTAT_VCOUNT_IRQ_ENABLE = 0x0020;
constexpr uint16_t DISPSTAT_VCOUNT_SETTING_MASK = 0xFF00;

// VRAM and Palette addresses
constexpr uint32_t VRAM_BASE = 0x06000000;
constexpr uint32_t PALETTE_RAM_BASE = 0x05000000;
constexpr uint32_t OAM_BASE = 0x07000000;

void GBAPPU::init() {
    dispcnt = 0;
    dispstat = 0;
    vcount = 0;
    scanline = 0;
    dot = 0;

    // Clear background control registers
    bg_control.fill(0);
    bg_scroll_x.fill(0);
    bg_scroll_y.fill(0);

    // Clear framebuffer
    framebuffer.fill(0);
}

void GBAPPU::step(GBASystem& gba) {
    dot++;

    // Handle H-Blank
    if (dot == 240) {
        dispstat |= DISPSTAT_HBLANK;

        // Trigger H-Blank IRQ if enabled
        if (dispstat & DISPSTAT_HBLANK_IRQ_ENABLE) {
            // Request H-Blank interrupt
            gba.request_interrupt(1); // IRQ_HBLANK
        }
    }

    // End of scanline
    if (dot >= DOTS_PER_SCANLINE) {
        dot = 0;
        scanline++;
        vcount = scanline;

        // Clear H-Blank flag
        dispstat &= ~DISPSTAT_HBLANK;

        // Check for V-Count match
        uint8_t vcount_setting = (dispstat & DISPSTAT_VCOUNT_SETTING_MASK) >> 8;
        if (vcount == vcount_setting) {
            dispstat |= DISPSTAT_VCOUNT_MATCH;
            if (dispstat & DISPSTAT_VCOUNT_IRQ_ENABLE) {
                gba.request_interrupt(2); // IRQ_VCOUNT
            }
        } else {
            dispstat &= ~DISPSTAT_VCOUNT_MATCH;
        }

        // Handle V-Blank
        if (scanline == GBA_SCREEN_HEIGHT) {
            dispstat |= DISPSTAT_VBLANK;

            // Trigger V-Blank IRQ if enabled
            if (dispstat & DISPSTAT_VBLANK_IRQ_ENABLE) {
                gba.request_interrupt(0); // IRQ_VBLANK
            }
        }

        // End of frame
        if (scanline >= TOTAL_SCANLINES) {
            scanline = 0;
            vcount = 0;
            dispstat &= ~DISPSTAT_VBLANK;
        }

        // Render visible scanlines
        if (scanline < GBA_SCREEN_HEIGHT) {
            render_scanline(gba);
        }
    }
}

void GBAPPU::render_scanline(GBASystem& gba) {
    // Skip rendering if forced blank is enabled
    if (dispcnt & DISPCNT_FORCED_BLANK) {
        // Fill scanline with white
        for (int x = 0; x < GBA_SCREEN_WIDTH; x++) {
            framebuffer[scanline * GBA_SCREEN_WIDTH + x] = 0xFFFFFFFF;
        }
        return;
    }

    // Clear scanline to backdrop color
    uint16_t backdrop_color = gba.memory.read16(PALETTE_RAM_BASE);
    uint32_t backdrop_rgb = convert_color(backdrop_color);

    for (int x = 0; x < GBA_SCREEN_WIDTH; x++) {
        framebuffer[scanline * GBA_SCREEN_WIDTH + x] = backdrop_rgb;
    }

    // Get background mode
    int bg_mode = dispcnt & DISPCNT_BG_MODE_MASK;

    // Render backgrounds based on mode
    switch (bg_mode) {
        case 0: render_background_mode0(gba, scanline); break;
        case 1: render_background_mode1(gba, scanline); break;
        case 2: render_background_mode2(gba, scanline); break;
        case 3: render_background_mode3(gba, scanline); break;
        case 4: render_background_mode4(gba, scanline); break;
        case 5: render_background_mode5(gba, scanline); break;
    }

    // Render sprites if enabled
    if (dispcnt & DISPCNT_SCREEN_DISPLAY_OBJ) {
        render_sprites(gba, scanline);
    }
}

void GBAPPU::render_background_mode0(GBASystem& gba, int line) {
    // Mode 0: 4 text backgrounds (BG0-BG3)
    for (int bg = 3; bg >= 0; bg--) { // Render back to front
        if (dispcnt & (DISPCNT_SCREEN_DISPLAY_BG0 << bg)) {
            render_text_background(gba, bg, line);
        }
    }
}

void GBAPPU::render_background_mode1(GBASystem& gba, int line) {
    // Mode 1: BG0, BG1 as text, BG2 as affine
    if (dispcnt & DISPCNT_SCREEN_DISPLAY_BG2) {
        render_affine_background(gba, 2, line);
    }
    if (dispcnt & DISPCNT_SCREEN_DISPLAY_BG1) {
        render_text_background(gba, 1, line);
    }
    if (dispcnt & DISPCNT_SCREEN_DISPLAY_BG0) {
        render_text_background(gba, 0, line);
    }
}

void GBAPPU::render_background_mode2(GBASystem& gba, int line) {
    // Mode 2: BG2, BG3 as affine
    if (dispcnt & DISPCNT_SCREEN_DISPLAY_BG3) {
        render_affine_background(gba, 3, line);
    }
    if (dispcnt & DISPCNT_SCREEN_DISPLAY_BG2) {
        render_affine_background(gba, 2, line);
    }
}

void GBAPPU::render_background_mode3(GBASystem& gba, int line) {
    // Mode 3: Single 240x160 16-bit color bitmap
    uint32_t vram_offset = line * GBA_SCREEN_WIDTH * 2;

    for (int x = 0; x < GBA_SCREEN_WIDTH; x++) {
        uint16_t pixel = gba.memory.read16(VRAM_BASE + vram_offset + x * 2);
        framebuffer[line * GBA_SCREEN_WIDTH + x] = convert_color(pixel);
    }
}

void GBAPPU::render_background_mode4(GBASystem& gba, int line) {
    // Mode 4: Single 240x160 8-bit color bitmap (with palette)
    uint32_t frame_base = (dispcnt & DISPCNT_DISPLAY_FRAME) ? 0xA000 : 0;
    uint32_t vram_offset = frame_base + line * GBA_SCREEN_WIDTH;

    for (int x = 0; x < GBA_SCREEN_WIDTH; x++) {
        uint8_t palette_index = gba.memory.read8(VRAM_BASE + vram_offset + x);
        uint16_t color = gba.memory.read16(PALETTE_RAM_BASE + palette_index * 2);
        framebuffer[line * GBA_SCREEN_WIDTH + x] = convert_color(color);
    }
}

void GBAPPU::render_background_mode5(GBASystem& gba, int line) {
    // Mode 5: Single 160x128 16-bit color bitmap (scaled)
    if (line >= 128) return; // Mode 5 only has 128 lines

    uint32_t frame_base = (dispcnt & DISPCNT_DISPLAY_FRAME) ? 0xA000 : 0;
    uint32_t vram_offset = frame_base + line * 160 * 2;

    for (int x = 0; x < 160 && x < GBA_SCREEN_WIDTH; x++) {
        uint16_t pixel = gba.memory.read16(VRAM_BASE + vram_offset + x * 2);
        framebuffer[line * GBA_SCREEN_WIDTH + x] = convert_color(pixel);
    }
}

void GBAPPU::render_sprites(GBASystem& gba, int line) {
    // Simple sprite rendering - iterate through OAM
    for (int sprite = 0; sprite < 128; sprite++) {
        uint32_t oam_offset = sprite * 8; // Each sprite entry is 8 bytes

        // Read sprite attributes
        uint16_t attr0 = gba.memory.read16(OAM_BASE + oam_offset);
        uint16_t attr1 = gba.memory.read16(OAM_BASE + oam_offset + 2);
        uint16_t attr2 = gba.memory.read16(OAM_BASE + oam_offset + 4);

        // Check if sprite is disabled
        if ((attr0 & 0x0300) == 0x0200) continue; // Double-size disabled

        // Get sprite Y position
        int sprite_y = attr0 & 0xFF;
        if (sprite_y >= 160) sprite_y -= 256; // Handle signed position

        // Get sprite size
        int sprite_size = (attr0 >> 14) & 3;
        int sprite_shape = (attr1 >> 14) & 3;

        // Determine sprite dimensions (simplified)
        int sprite_height = 8;
        int sprite_width = 8;

        switch (sprite_size) {
            case 0: // Small
                sprite_width = sprite_height = (sprite_shape == 0) ? 8 : (sprite_shape == 1) ? 8 : 8;
                break;
            case 1: // Medium
                sprite_width = sprite_height = (sprite_shape == 0) ? 16 : (sprite_shape == 1) ? 16 : 16;
                break;
            case 2: // Large
                sprite_width = sprite_height = (sprite_shape == 0) ? 32 : (sprite_shape == 1) ? 32 : 32;
                break;
            case 3: // Extra large
                sprite_width = sprite_height = (sprite_shape == 0) ? 64 : (sprite_shape == 1) ? 64 : 64;
                break;
        }

        // Check if sprite intersects current line
        if (line < sprite_y || line >= sprite_y + sprite_height) continue;

        // Get sprite X position
        int sprite_x = attr1 & 0x1FF;
        if (sprite_x >= 240) sprite_x -= 512; // Handle signed position

        // Simple sprite pixel rendering (8bpp for now)
        int tile_num = attr2 & 0x3FF;
        bool palette_mode = (attr0 & 0x2000) != 0; // 0 = 16 color, 1 = 256 color

        for (int px = 0; px < sprite_width; px++) {
            int screen_x = sprite_x + px;
            if (screen_x < 0 || screen_x >= GBA_SCREEN_WIDTH) continue;

            // Simple tile lookup (would need proper implementation)
            uint8_t pixel_data = 1; // Placeholder - would read from tile data
            if (pixel_data == 0) continue; // Transparent pixel

            // Use sprite palette
            uint32_t palette_base = PALETTE_RAM_BASE + 0x200; // Sprite palette
            uint16_t color = gba.memory.read16(palette_base + pixel_data * 2);
            framebuffer[line * GBA_SCREEN_WIDTH + screen_x] = convert_color(color);
        }
    }
}

uint32_t GBAPPU::convert_color(uint16_t gba_color) const {
    // Convert GBA 15-bit BGR555 to 32-bit RGBA8888
    uint8_t r = (gba_color & 0x001F) << 3;
    uint8_t g = ((gba_color & 0x03E0) >> 5) << 3;
    uint8_t b = ((gba_color & 0x7C00) >> 10) << 3;

    // Expand 5-bit components to 8-bit
    r |= r >> 5;
    g |= g >> 5;
    b |= b >> 5;

    return (0xFF << 24) | (b << 16) | (g << 8) | r; // RGBA format
}

void GBAPPU::render_text_background(GBASystem& gba, int bg_num, int line) {
    uint16_t bg_cnt = bg_control[bg_num];

    // Extract background control parameters
    int priority = bg_cnt & 3;
    int char_base = (bg_cnt >> 2) & 3;
    int mosaic = (bg_cnt >> 6) & 1;
    int palette_mode = (bg_cnt >> 7) & 1; // 0 = 16/16, 1 = 256/1
    int screen_base = (bg_cnt >> 8) & 0x1F;
    int wraparound = (bg_cnt >> 13) & 1;
    int screen_size = (bg_cnt >> 14) & 3;

    // Calculate dimensions
    int map_width = (screen_size & 1) ? 64 : 32;
    int map_height = (screen_size & 2) ? 64 : 32;

    // Get scroll values
    int scroll_x = bg_scroll_x[bg_num] & 0x1FF;
    int scroll_y = bg_scroll_y[bg_num] & 0x1FF;

    // Calculate which tile row is being rendered
    int bg_line = (line + scroll_y) % (map_height * 8);
    int tile_y = bg_line / 8;
    int pixel_y = bg_line % 8;

    uint32_t char_base_addr = VRAM_BASE + char_base * 0x4000;
    uint32_t screen_base_addr = VRAM_BASE + screen_base * 0x800;

    for (int x = 0; x < GBA_SCREEN_WIDTH; x++) {
        int bg_x = (x + scroll_x) % (map_width * 8);
        int tile_x = bg_x / 8;
        int pixel_x = bg_x % 8;

        // Calculate screen entry address
        uint32_t screen_entry_addr = screen_base_addr + (tile_y * map_width + tile_x) * 2;
        uint16_t screen_entry = gba.memory.read16(screen_entry_addr);

        // Extract tile info
        int tile_num = screen_entry & 0x3FF;
        int h_flip = (screen_entry >> 10) & 1;
        int v_flip = (screen_entry >> 11) & 1;
        int palette_num = (screen_entry >> 12) & 0xF;

        // Apply flipping
        int final_pixel_x = h_flip ? (7 - pixel_x) : pixel_x;
        int final_pixel_y = v_flip ? (7 - pixel_y) : pixel_y;

        // Get pixel data
        uint8_t pixel_data;
        if (palette_mode) {
            // 256 color mode
            uint32_t tile_addr = char_base_addr + tile_num * 64 + final_pixel_y * 8 + final_pixel_x;
            pixel_data = gba.memory.read8(tile_addr);
        } else {
            // 16 color mode
            uint32_t tile_addr = char_base_addr + tile_num * 32 + final_pixel_y * 4 + final_pixel_x / 2;
            uint8_t byte_data = gba.memory.read8(tile_addr);
            pixel_data = (final_pixel_x & 1) ? (byte_data >> 4) : (byte_data & 0xF);
        }

        // Skip transparent pixels
        if (pixel_data == 0) continue;

        // Get color from palette
        uint32_t palette_addr;
        if (palette_mode) {
            palette_addr = PALETTE_RAM_BASE + pixel_data * 2;
        } else {
            palette_addr = PALETTE_RAM_BASE + (palette_num * 16 + pixel_data) * 2;
        }

        uint16_t color = gba.memory.read16(palette_addr);
        framebuffer[line * GBA_SCREEN_WIDTH + x] = convert_color(color);
    }
}

void GBAPPU::render_affine_background(GBASystem& gba, int bg_num, int line) {
    // Simplified affine background rendering
    // This would require proper affine transformation matrix calculations
    // For now, render as a basic text background without transformation
    render_text_background(gba, bg_num, line);
}