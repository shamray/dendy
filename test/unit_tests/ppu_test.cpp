#include <libnes/ppu.hpp>
#include <libnes/literals.hpp>
#include <catch2/catch.hpp>

using namespace nes::literals;

void tick(auto& ppu, int times = 1) {
    for (auto i = 0; i < times; ++i) {
        ppu.tick();
    }
}

struct dummy_bus
{
    void chr_write(uint16_t addr, uint8_t value) { chr[addr] = value; }
    uint8_t chr_read(uint16_t addr) const { return chr[addr]; }

    std::array<uint8_t, 2_Kb> chr;
};
dummy_bus bus;

TEST_CASE("scanline cycles") {
    auto ppu = nes::ppu{bus};

    SECTION("at power up") {
        CHECK(ppu.scan_line() == -1);
        CHECK(ppu.scan_cycle() == 0);
        CHECK_FALSE(ppu.is_odd_frame());
    }

    SECTION("one dot") {
        tick(ppu);
        CHECK(ppu.scan_line() == -1);
        CHECK(ppu.scan_cycle() == 1);
    }

    SECTION("full line") {
        tick(ppu, 340);
        CHECK(ppu.scan_line() == -1);
        CHECK(ppu.scan_cycle() == 340);
    }

    SECTION("next line") {
        tick(ppu, 341);
        CHECK(ppu.scan_line() == 0);
        CHECK(ppu.scan_cycle() == 0);
    }

    SECTION("last line") {
        tick(ppu, 341 * 261);
        CHECK(ppu.scan_line() == 260);
        CHECK(ppu.scan_cycle() == 0);
    }

    SECTION("next frame") {
        tick(ppu, 341 * 262);
        CHECK(ppu.scan_line() == -1);
        CHECK(ppu.scan_cycle() == 0);
        CHECK(ppu.is_frame_ready());
    }

    SECTION("frame ready") {
        CHECK_FALSE(ppu.is_frame_ready());

        tick(ppu, 341 * 262);
        CHECK(ppu.is_frame_ready());

        tick(ppu, 341 * 262);
        CHECK(ppu.is_frame_ready());
    }

    SECTION("even/odd frames") {
        CHECK_FALSE(ppu.is_odd_frame());

        tick(ppu, 341 * 262);
        CHECK(ppu.is_odd_frame());

        tick(ppu, 341 * 262);
        CHECK_FALSE(ppu.is_odd_frame());
    }
}

TEST_CASE("palette address") {
    auto ppu = nes::ppu{bus};

    SECTION("zeroeth and first") {
        CHECK(ppu.palette_address(0x00) == 0x00);
        CHECK(ppu.palette_address(0x01) == 0x01);
    }

    SECTION("random colors in the middle") {
        CHECK(ppu.palette_address(0x07) == 0x07);
        CHECK(ppu.palette_address(0x1D) == 0x1D);
        CHECK(ppu.palette_address(0x13) == 0x13);
    }

    SECTION("out of range") {
        CHECK(ppu.palette_address(0x20) == 0x00);
        CHECK(ppu.palette_address(0x21) == 0x01);
    }

    SECTION("mapping background color") {
        CHECK(ppu.palette_address(0x10) == 0x00);
        CHECK(ppu.palette_address(0x04) == 0x00);
        CHECK(ppu.palette_address(0x08) == 0x00);
        CHECK(ppu.palette_address(0x0C) == 0x00);
        CHECK(ppu.palette_address(0x14) == 0x00);
        CHECK(ppu.palette_address(0x18) == 0x00);
        CHECK(ppu.palette_address(0x1C) == 0x00);
    }
}