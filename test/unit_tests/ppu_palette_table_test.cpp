#include <libnes/ppu_palette_table.hpp>
#include <catch2/catch_all.hpp>

TEST_CASE("palette address") {
    auto pt = nes::palette_table{nes::DEFAULT_COLORS};

    SECTION("zeroeth and first") {
        CHECK(pt.palette_address(0x00) == 0x00);
        CHECK(pt.palette_address(0x01) == 0x01);
    }

    SECTION("random colors in the middle") {
        CHECK(pt.palette_address(0x07) == 0x07);
        CHECK(pt.palette_address(0x1D) == 0x1D);
        CHECK(pt.palette_address(0x13) == 0x13);
    }

    SECTION("out of range") {
        CHECK(pt.palette_address(0x20) == 0x00);
        CHECK(pt.palette_address(0x21) == 0x01);
    }

    SECTION("mapping palette addresses") {
        CHECK(pt.palette_address(0x10) == 0x00);
        CHECK(pt.palette_address(0x14) == 0x04);
        CHECK(pt.palette_address(0x18) == 0x08);
        CHECK(pt.palette_address(0x1C) == 0x0C);
    }

    SECTION("pixel 0 is always background color") {
        pt.write(0, 0x0F);
        pt.write(8, 0x11);

        CHECK(pt.color_of(0, 2) == nes::DEFAULT_COLORS[0x0F]);
    }
}