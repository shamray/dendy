#include <libnes/ppu.hpp>
#include <libnes/literals.hpp>
#include <catch2/catch.hpp>

using namespace nes::literals;

void tick(auto& ppu, int times = 1) {
    for (auto i = 0; i < times; ++i) {
        ppu.tick();
    }
}

TEST_CASE("scanline cycles") {
    std::array<uint8_t, 8_Kb> chr;
    auto ppu = nes::ppu{chr};

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
    auto ppu = nes::ppu{};

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

TEST_CASE("pattern_table") {
    SECTION("not connected") {
        auto chr = nes::pattern_table{};

        CHECK_FALSE(chr.is_connected());
        CHECK_THROWS(chr.read(0x1234));
    }

    auto bank = std::array<uint8_t, 8_Kb>{};
    bank[0x1234] = 0x42;

    SECTION("created connected") {
        auto chr = nes::pattern_table{&bank};

        SECTION("read") {
            CHECK(chr.is_connected());
            CHECK((int)chr.read(0x1234) == 0x42);
        }

        SECTION("disconnect") {
            chr.connect(nullptr);

            CHECK_FALSE(chr.is_connected());
        }
    }

    SECTION("connect") {
        auto chr = nes::pattern_table{};
        chr.connect(&bank);

        CHECK(chr.is_connected());
        CHECK((int)chr.read(0x1234) == 0x42);
    }
}

TEST_CASE("nametable") {
    auto nt = nes::name_table{};

    SECTION("table 0") {
        nt.write(0x007, 0x55);

        CHECK((int)nt.read(0x007) == 0x55);
        CHECK((int)nt.table(0)[0x007] == 0x55);
    }
    SECTION("vertical mirroring") {
        nt.mirroring = nes::name_table_mirroring::vertical;

        nt.write(0x007, 0x55);
        nt.write(0x407, 0x11);

        CHECK((int)nt.read(0x007) == 0x55);
        CHECK((int)nt.read(0x807) == 0x55);

        CHECK((int)nt.read(0x407) == 0x11);
        CHECK((int)nt.read(0xC07) == 0x11);
    }

    SECTION("horizontal mirroring") {
        nt.mirroring = nes::name_table_mirroring::horizontal;

        nt.write(0x007, 0x55);
        nt.write(0x807, 0x11);

        CHECK((int)nt.read(0x007) == 0x55);
        CHECK((int)nt.read(0x407) == 0x55);

        CHECK((int)nt.read(0x807) == 0x11);
        CHECK((int)nt.read(0xC07) == 0x11);
    }
}