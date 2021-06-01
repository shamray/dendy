#include <libnes/ppu.hpp>
#include <random>

namespace nes
{

const auto VISIBLE_SCANLINES = 240;
const auto VERTICAL_BLANK_SCANLINES = 20;
const auto POST_RENDER_SCANLINES = 1;
const auto SCANLINE_DOTS = 341;

void ppu::tick() noexcept {
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