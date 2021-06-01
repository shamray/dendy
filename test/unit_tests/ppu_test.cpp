#include <libnes/ppu.hpp>
#include <catch2/catch.hpp>

void tick(auto& ppu, int times = 1) {
    for (auto i = 0; i < times; ++i) {
        ppu.tick();
    }
}

TEST_CASE("scanline cycles") {
    auto ppu = nes::ppu{};

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
        CHECK(ppu.is_odd_frame());
    }
}