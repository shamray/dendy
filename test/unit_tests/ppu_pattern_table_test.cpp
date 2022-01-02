#include <libnes/ppu_pattern_table.hpp>
#include <libnes/literals.hpp>
#include <catch2/catch.hpp>

using namespace nes::literals;

TEST_CASE("pattern_table") {
    SECTION("not connected") {
        auto chr = nes::pattern_table{};

        CHECK_FALSE(chr.is_connected());
        CHECK_THROWS(chr.read(0x1234));
    }

    auto bank = std::array<std::uint8_t, 8_Kb>{};
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