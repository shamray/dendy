#pragma once

#include <array>
#include <optional>
#include <stdexcept>
#include <cassert>

#include <libnes/literals.hpp>

namespace nes {

constexpr auto COLORS = std::to_array<uint32_t>({
    0xFF7C7C7C, 0xFF0000FC, 0xFF0000BC, 0xFF4428BC,
    0xFF940084, 0xFFA80020, 0xFFA81000, 0xFF881400,
    0xFF503000, 0xFF007800, 0xFF006800, 0xFF005800,
    0xFF004058, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFBCBCBC, 0xFF0078F8, 0xFF0058F8, 0xFF6844FC,
    0xFFD800CC, 0xFFE40058, 0xFFF83800, 0xFFE45C10,
    0xFFAC7C00, 0xFF00B800, 0xFF00A800, 0xFF00A844,
    0xFF008888, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFF8F8F8, 0xFF3CBCFC, 0xFF6888FC, 0xFF9878F8,
    0xFFF878F8, 0xFFF85898, 0xFFF87858, 0xFFFCA044,
    0xFFF8B800, 0xFFB8F818, 0xFF58D854, 0xFF58F898,
    0xFF00E8D8, 0xFF787878, 0xFF000000, 0xFF000000,
    0xFFFCFCFC, 0xFFA4E4FC, 0xFFB8B8F8, 0xFFD8B8F8,
    0xFFF8B8F8, 0xFFF8A4C0, 0xFFF0D0B0, 0xFFFCE0A8,
    0xFFF8D878, 0xFFD8F878, 0xFFB8F8B8, 0xFFB8F8D8,
    0xFF00FCFC, 0xFFF8D8F8, 0xFF000000, 0xFF000000
});

class pattern_table {
public:
    using memory_bank = std::array<uint8_t, 8_Kb>;

    pattern_table() = default;
    pattern_table(const memory_bank* bank)
        : bank_{bank}
    {}

    constexpr void connect(const memory_bank* new_bank) {
        bank_ = new_bank;
    }

    [[nodiscard]] constexpr auto is_connected() const { return bank_ != nullptr; }

    [[nodiscard]] constexpr auto read(uint16_t addr) const {
        if (bank_ == nullptr)
            throw std::range_error("No CHR bank connected");

        return bank_->at(addr);
    }
    constexpr void write(uint16_t, uint8_t )    {}

private:
    const memory_bank* bank_{nullptr};
};

class ppu {
public:
    ppu() = default;
    ppu(const pattern_table::memory_bank& chr)
        : chr_{&chr}
    {}

    uint8_t control;
    uint8_t status;
    uint8_t mask;
    uint8_t oam_addr;
    uint8_t oam_data;
    uint8_t scroll;
    uint8_t addr;
    uint8_t data;
    uint8_t oam_dma;

    bool nmi{false};

    int address_latch{0};
    uint16_t address;
    uint8_t data_buffer;

    void tick() noexcept;

    [[nodiscard]] constexpr auto scan_line() const noexcept { return scan_.line; }
    [[nodiscard]] constexpr auto scan_cycle() const noexcept { return scan_.cycle; }
    [[nodiscard]] constexpr auto is_odd_frame() const noexcept { return frame_is_odd_; }
    [[nodiscard]] constexpr auto is_frame_ready() const noexcept { return frame_is_ready_; }

    std::array<uint32_t, 256 * 240> frame_buffer;

    void render_noise(auto get_noise) {
        // The sky above the port was the color of television, tuned to a dead channel
        for (auto &&pixel: frame_buffer) {
            auto r = get_noise();
            pixel = (0xFF << 24) | (r << 16) | (r << 8) | (r);
        }
    }

    [[nodiscard]] static constexpr auto palette_address(uint8_t address) noexcept {
        address &= 0x1F;

        if ((address & 0x03) == 0x00) {
            address = 0;
        }

        return address;
    }

    [[nodiscard]] constexpr auto read_palette_color(uint8_t address) const noexcept {
        return palette_[palette_address(address)];
    }

    constexpr void write_palette_color(uint8_t address, uint8_t value) noexcept {
        palette_[palette_address(address)] = value;
    }

    auto get_palette_color(uint8_t pixel, uint8_t palette) {
        auto rpc = read_palette_color((palette << 2) + pixel);
        return COLORS[rpc];
    }

    auto chr_read(uint16_t addr) const { return chr_.read(addr); } //TODO: get rid of

    auto display_pattern_table(auto i, auto palette) const -> std::array<uint32_t, 128 * 128>;

private:
    void prerender_scanline() noexcept {};
    void visible_scanline() noexcept {};
    void postrender_scanline() noexcept {};
    void vertical_blank_line() noexcept {};

private:
    struct {
        int line{-1};
        int cycle{0};
    } scan_;

    bool frame_is_odd_{false};
    bool frame_is_ready_{false};

    pattern_table chr_;
    std::array<uint8_t, 32> palette_;
};

const auto VISIBLE_SCANLINES = 240;
const auto VERTICAL_BLANK_SCANLINES = 20;
const auto POST_RENDER_SCANLINES = 1;
const auto SCANLINE_DOTS = 341;

[[nodiscard]] constexpr auto is_prerender(auto scanline) noexcept {
    return scanline == -1;
}

[[nodiscard]] constexpr auto is_visible(auto scanline) noexcept {
    return scanline >= 0 and scanline < VISIBLE_SCANLINES;
}

[[nodiscard]] constexpr auto is_postrender(auto scanline) noexcept {
    return scanline >= VISIBLE_SCANLINES and scanline < VISIBLE_SCANLINES + POST_RENDER_SCANLINES;
}

[[nodiscard]] constexpr auto is_vblank(auto scanline) noexcept {
    return scanline >= VISIBLE_SCANLINES + POST_RENDER_SCANLINES and
           scanline < VISIBLE_SCANLINES + POST_RENDER_SCANLINES + VERTICAL_BLANK_SCANLINES;
}

inline void ppu::tick() noexcept {
    if (is_prerender(scan_line())) {
        prerender_scanline();
    } else if (is_visible(scan_line())) {
        visible_scanline();
    } else if (is_postrender(scan_line())) {
        postrender_scanline();
    } else if (is_vblank(scan_line())) {
        vertical_blank_line();
    }

    frame_is_ready_ = false;

    if (++scan_.cycle >= SCANLINE_DOTS) {
        scan_.cycle = 0;

        ++scan_.line;

        if (scan_.line == VISIBLE_SCANLINES + POST_RENDER_SCANLINES) {
            status |= 0x80;
            if (control & 0x80)
                nmi = true;
        }
        else if (scan_.line >= VISIBLE_SCANLINES + POST_RENDER_SCANLINES + VERTICAL_BLANK_SCANLINES) {
            scan_.line = -1;
            frame_is_odd_ = not frame_is_odd_;
            frame_is_ready_ = true;
        }
    }
}

inline auto ppu::display_pattern_table(auto i, auto palette) const -> std::array<uint32_t, 128 * 128> {
    auto result = std::array<uint32_t, 128 * 128>{};

    for (uint16_t tile_y = 0; tile_y < 16; ++tile_y) {
        for (uint16_t tile_x = 0; tile_x < 16; ++tile_x) {

            auto offset = static_cast<uint16_t>(tile_y * 256 + tile_x * 16);

            for (uint16_t row = 0; row < 8; ++row) {

                auto tile_lsb = chr_.read(i * 0x1000 + offset + row + 0);
                auto tile_msb = chr_.read(i * 0x1000 + offset + row + 8);

                for (uint16_t col = 0; col < 8; col++) {
                    auto pixel = static_cast<uint8_t>((tile_lsb & 0x01) | ((tile_msb & 0x01) <<1));

                    auto result_offset = (tile_y * 8 + row) * 128 + tile_x * 8 + (7 - col);
                    auto result_color = get_palette_color(pixel, palette);

                    result[result_offset] = result_color;

                    tile_lsb >>= 1; tile_msb >>= 1;
                }
            }
        }
    }
    return result;
}
}