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
    void chr_write(uint16_t addr, uint8_t value) { mem[addr] = value; }
    uint8_t chr_read(uint16_t addr) const { return mem[addr]; }

    std::array<uint8_t, 2_Kb> mem;
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