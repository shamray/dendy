#include <libnes/ppu.hpp>
#include <random>

namespace nes
{

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
    return scanline >= VISIBLE_SCANLINES + POST_RENDER_SCANLINES and scanline < VISIBLE_SCANLINES + POST_RENDER_SCANLINES + VERTICAL_BLANK_SCANLINES;
}

void ppu::tick() noexcept {
    if (is_prerender(scan_line())) {
        prerender_scanline();
    } else if (is_visible(scan_line())) {
        visible_scanline();
    } else if (is_postrender(scan_line())) {
        postrender_scanline();
    } else if (is_vblank(scan_line())) {
        vertical_blank_line();
    }

    if (++scan_.cycle >= SCANLINE_DOTS) {
        scan_.cycle = 0;

        if (++scan_.line >= VISIBLE_SCANLINES + POST_RENDER_SCANLINES + VERTICAL_BLANK_SCANLINES) {
            scan_.line = -1;
            frame_is_odd_ = not frame_is_odd_;
            render_frame();
        }
    }
}

void ppu::render_frame() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    auto distrib = std::uniform_int_distribution<short>(0, 255);

    // The sky above the port was the color of television, tuned to a dead channel

    for (auto&& pixel: frame_buffer) {
        auto r = static_cast<uint8_t>(distrib(gen));
        pixel = (0xFF << 24) | (r << 16) | (r << 8) | (r);
    }
}

}