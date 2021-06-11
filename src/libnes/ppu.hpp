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

enum class name_table_mirroring { horizontal, vertical };

class name_table
{
    [[nodiscard]] constexpr auto bank_index(uint16_t addr) const {
        using enum name_table_mirroring;
        switch(mirroring) {
            case horizontal:    return (addr >> 11) & 0x01;
            case vertical:      return (addr >> 10) & 0x01;
        }
        throw std::logic_error("Unhandled mirroring scenario");
    }

    [[nodiscard]] constexpr auto bank_offset(uint16_t addr) const noexcept {
        return addr & 0x3FF;
    }

public:
    constexpr void write(uint16_t addr, uint8_t value) {
        auto i = bank_index(addr);
        auto j = bank_offset(addr);

        vram_[i][j] = value;
    }
    [[nodiscard]] constexpr auto read(uint16_t addr) const {
        auto i = bank_index(addr);
        auto j = bank_offset(addr);
        return vram_[i][j];
    }

    [[nodiscard]] constexpr auto table(int bank) const -> auto& {
        return vram_[bank & 1];
    }

    name_table_mirroring mirroring{name_table_mirroring::vertical};

private:
    using bank = std::array<uint8_t, 2_Kb>;
    std::array<bank, 2> vram_;
};

class palette_table
{
public:
    [[nodiscard]] static constexpr auto palette_address(uint8_t address) noexcept {
        address &= 0x1F;

        if ((address & 0x03) == 0x00) {
            address = 0;
        }

        return address;
    }

    [[nodiscard]] constexpr auto read(uint8_t address) const noexcept {
        return palette_ram_[palette_address(address)];
    }

    constexpr void write(uint8_t address, uint8_t value) noexcept {
        palette_ram_[palette_address(address)] = value;
    }

    [[nodiscard]] auto color(uint8_t pixel, uint8_t palette) const noexcept -> uint32_t {
        auto rpc = read((palette << 2) + pixel);
        return COLORS[rpc & 0x3F];
    }

private:
    std::array<uint8_t, 32> palette_ram_;
};

class crt_scan
{
public:
    constexpr crt_scan(int dots, int visible_scanlines, int postrender_scanlines, int vblank_scanlines) noexcept
        : dots_{dots}
        , visible_scanlines_{visible_scanlines}
        , postrender_scanlines_{postrender_scanlines}
        , vblank_scanlines_{vblank_scanlines}
    {}

    [[nodiscard]] constexpr auto line() const noexcept { return line_; }
    [[nodiscard]] constexpr auto cycle() const noexcept { return cycle_; }
    [[nodiscard]] constexpr auto is_odd_frame() const noexcept { return frame_is_odd_; }
    [[nodiscard]] constexpr auto is_frame_finished() const noexcept { return line_ == -1 and cycle_ == 0; }

    [[nodiscard]] constexpr auto is_prerender() const noexcept {
        return line_ == -1;
    }

    [[nodiscard]] constexpr auto is_visible() const noexcept {
        return line_ >= 0 and line_ < visible_scanlines_;
    }

    [[nodiscard]] constexpr auto is_postrender() const noexcept {
        return line_ >= visible_scanlines_ and line_ < visible_scanlines_ + postrender_scanlines_;
    }

    [[nodiscard]] constexpr auto is_vblank() const noexcept {
        return line_ >= visible_scanlines_ + postrender_scanlines_ and
               line_ < visible_scanlines_ + postrender_scanlines_ + vblank_scanlines_;
    }

    constexpr void advance() noexcept {
        if (++cycle_ >= dots_) {
            cycle_ = 0;

            if (++line_ >= visible_scanlines_ + postrender_scanlines_ + vblank_scanlines_) {
                line_ = -1;
                frame_is_odd_ = not frame_is_odd_;
            }
        }
    }

private:
    const int dots_;
    const int visible_scanlines_;
    const int postrender_scanlines_;
    const int vblank_scanlines_;

    int line_{-1};
    int cycle_{0};
    bool frame_is_odd_{false};
};

const auto VISIBLE_SCANLINES = 240;
const auto VERTICAL_BLANK_SCANLINES = 20;
const auto POST_RENDER_SCANLINES = 1;
const auto SCANLINE_DOTS = 341;

class ppu
{
public:
    ppu() = default;
    ppu(const pattern_table::memory_bank& chr)
        : chr_{&chr}
    {}

    constexpr void connect_pattern_table(auto new_bank) noexcept { chr_.connect(new_bank); }

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

    void tick();

    [[nodiscard]] constexpr auto is_frame_ready() const noexcept { return scan_.is_frame_finished(); }

    std::array<uint32_t, 256 * 240> frame_buffer;

    [[nodiscard]] auto read(uint16_t addr) -> std::optional<uint8_t> const {
        switch (addr) {
            case 0x2002: {
                auto d = status & 0xE0;
                status &= 0x60;
                address_latch = 0;
                return d;
            }
            case 0x2007: {
                auto addr = address++;
                if (addr >= 0x2000 and addr <= 0x2FFF) {
                    return vram_.read(addr & 0xFFF);
                } else if (addr >= 0x3000 and addr <= 0x3EFF) {
                    throw std::range_error("not implemented");
                } else if (addr >= 0x3F00 and addr <= 0x3FFF) {
                    return palette_table_.read(addr & 0x001F);
                } else {
                    return chr_.read(addr);
                }
            }
            default: return std::nullopt;
        }
    }

    auto write(uint16_t addr, uint8_t value) -> bool {
        switch (addr) {
            case 0x2000: {
                control = value;
                return true;
            }
            case 0x2006: {
                if (address_latch == 0) {
                    address = (address & 0x00FF) | (value << 8);
                    address_latch = 1;
                } else {
                    address = (address & 0xFF00) | (value << 0);
                    address_latch = 0;

                    address &= 0x3FFF;
                }
                return true;
            }
            case 0x2007: {
                if (address >= 0x2000 and address <= 0x3000) {
                    vram_.write(address & 0xFFF, value);
                } else if (address >= 0x3F00 and address <= 0x3FFF) {
                    palette_table_.write(address & 0x001F, value);
                } else {
                    // ppu chr write
                }
                auto inc = (control & 0x04) ? 32 : 1;
                address += inc;

                return true;
            }
            default: return false;
        }
    }

    auto display_pattern_table(auto i, auto palette) const -> std::array<uint32_t, 128 * 128>;


    void render_noise(auto get_noise) {
        // The sky above the port was the color of television, tuned to a dead channel
        for (auto &&pixel: frame_buffer) {
            auto r = get_noise();
            pixel = (0xFF << 24) | (r << 16) | (r << 8) | (r);
        }
    }

private:
    constexpr void prerender_scanline() noexcept {}

    constexpr void visible_scanline() {
        auto y = scan_.line();
        auto x = scan_.cycle() - 2;

        if (y < 240 and x >= 0 and x < 256) {
            auto tile_x = x / 8;
            auto tile_y = y / 8;

            auto row = y % 8;
            auto col = x % 8;

            auto offset = static_cast<uint16_t>((tile_y * 32 + tile_x) | ((control & 0x03) << 10));
            auto ch = vram_.read(offset);

            auto ix = (control & 0x10) >> 4;

            auto tile_lsb = chr_.read(ix * 0x1000 + ch * 0x10+ row + 0);
            auto tile_msb = chr_.read(ix * 0x1000 + ch * 0x10 + row + 8);
            tile_lsb >>= (7 - col); tile_msb >>= (7 - col);

            auto pixel = static_cast<uint8_t>((tile_lsb & 0x01) | ((tile_msb & 0x01) <<1));

            auto attr_byte_index = (0x3c0 + tile_y / 4 * 8 + tile_x / 4) | ((control & 0x03) << 10);
            auto attr_byte = vram_.read(attr_byte_index);

            auto palette = [tile_x, tile_y](auto attr_byte) {
                if ((tile_x % 4) >> 1) {
                    attr_byte = attr_byte >> 2;
                }
                if ((tile_y % 4) >> 1) {
                    attr_byte = attr_byte >> 4;
                }
                return attr_byte & 0x03;
            }(attr_byte);

            frame_buffer[y * 256 + x] = palette_table_.color(pixel, palette);
        }
    }

    constexpr void postrender_scanline() noexcept {
        status |= 0x80;
        if (control & 0x80)
            nmi = true;
    }
    constexpr void vertical_blank_line() noexcept {};

private:
    crt_scan scan_{SCANLINE_DOTS, VISIBLE_SCANLINES, POST_RENDER_SCANLINES, VERTICAL_BLANK_SCANLINES};

    pattern_table chr_;
    name_table vram_;
    palette_table palette_table_;
};

inline void ppu::tick() {
    if      (scan_.is_prerender())  { prerender_scanline(); }
    else if (scan_.is_visible())    { visible_scanline(); }
    else if (scan_.is_postrender()) { postrender_scanline(); }
    else if (scan_.is_vblank())     { vertical_blank_line(); }

    scan_.advance();
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