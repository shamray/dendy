#include <libnes/ppu_object_attribute_memory.hpp>
#include <catch2/catch_all.hpp>

TEST_CASE("DMA Write") {
    auto oam = nes::object_attribute_memory{};
    auto buf = std::array<std::uint8_t, 4 * 64>{};

    // Data for sprite #13
    buf[13*4 + 0] = 0x1;    // y
    buf[13*4 + 1] = 0x42;   // tile
    buf[13*4 + 2] = 0x23;   // attributes
    buf[13*4 + 3] = 0x2;    // x

    oam.dma_write(buf.data());

    auto sprite13 = oam.sprites[13];
    CHECK(sprite13.x == 0x2);
    CHECK(sprite13.y == 0x1);
    CHECK(sprite13.tile == 0x42);
    CHECK(sprite13.attr == 0x23);
}