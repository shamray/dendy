#pragma once

#include <cstdint>
#include <array>

namespace nes {

struct sprite
{
    std::uint8_t y   {0xFF};
    std::uint8_t tile{0xFF};
    std::uint8_t attr{0xFF};
    std::uint8_t x   {0xFF};
};
static_assert(sizeof(sprite) == 4);

inline auto operator==(const sprite& a, const sprite& b) {
    return a.y == b.y and a.tile == b.tile and a.attr == b.attr and a.x == b.x;
}

class object_attribute_memory
{
public:
    std::array<sprite, 64> sprites;

    constexpr void dma_write(const std::uint8_t* from) {
        for (auto& s: sprites) {
            s = *std::bit_cast<sprite*>(from);
            from += sizeof(sprite);
        }
    }

    void write(std::uint8_t data) {
        auto ram = reinterpret_cast<std::uint8_t*>(sprites.data());
        ram[address++] = data;
    }

    std::uint8_t address{0};
};

}
