#pragma once
#include <vector>
#include <array>

namespace nes
{

class ppu
{
public:
    uint8_t control;
    uint8_t status;
    uint8_t mask;
    uint8_t oam_addr;
    uint8_t oam_data;
    uint8_t scroll;
    uint8_t addr;
    uint8_t data;
    uint8_t oam_dma;

    void tick() noexcept;

    [[nodiscard]] constexpr auto scan_line() const noexcept { return scan_.line; }
    [[nodiscard]] constexpr auto scan_cycle() const noexcept { return scan_.cycle; }
    [[nodiscard]] constexpr auto is_odd_frame() const noexcept { return frame_is_odd_; }

    std::array<uint32_t, 256 * 240> frame_buffer;

private:
    void render_frame();

    void prerender_scanline() noexcept {};
    void visible_scanline() noexcept {};
    void postrender_scanline() noexcept {};
    void vertical_blank_line() noexcept {};

private:
    struct {
        int line {-1};
        int cycle {0};
    } scan_;
    bool frame_is_odd_{0};
};

}