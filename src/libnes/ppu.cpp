#include "libnes/ppu.hpp"
#include <random>

namespace nes
{

void ppu::tick() {
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