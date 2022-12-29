#include <libnes/ppu_crt_scan.hpp>
#include <catch2/catch_all.hpp>

namespace {

void tick(nes::crt_scan& scan, int times = 1) {
    for (auto i = 0; i < times; ++i) {
        scan.advance();
    }
}

}

TEST_CASE("scanline cycles") {
    auto scan = nes::crt_scan{341, 240, 1, 20};

    SECTION("at power up") {
        CHECK(scan.line() == -1);
        CHECK(scan.cycle() == 0);
        CHECK_FALSE(scan.is_odd_frame());
    }

    SECTION("one dot") {
        tick(scan);
        CHECK(scan.line() == -1);
        CHECK(scan.cycle() == 1);
    }

    SECTION("full line") {
        tick(scan, 340);
        CHECK(scan.line() == -1);
        CHECK(scan.cycle() == 340);
    }

    SECTION("next line") {
        tick(scan, 341);
        CHECK(scan.line() == 0);
        CHECK(scan.cycle() == 0);
    }

    SECTION("last line") {
        tick(scan, 341 * 261);
        CHECK(scan.line() == 260);
        CHECK(scan.cycle() == 0);
    }

    SECTION("next frame") {
        tick(scan, 341 * 262);
        CHECK(scan.line() == -1);
        CHECK(scan.cycle() == 0);
        CHECK(scan.is_frame_finished());
    }

    SECTION("frame ready") {
        tick(scan, 1);
        CHECK_FALSE(scan.is_frame_finished());

        tick(scan, 341 * 262 - 1);
        CHECK(scan.is_frame_finished());

        tick(scan, 341 * 262);
        CHECK(scan.is_frame_finished());
    }

    SECTION("even/odd frames") {
        CHECK_FALSE(scan.is_odd_frame());

        tick(scan, 341 * 262);
        CHECK(scan.is_odd_frame());

        tick(scan, 341 * 262);
        CHECK_FALSE(scan.is_odd_frame());
    }
}
