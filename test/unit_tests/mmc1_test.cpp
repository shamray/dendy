#include <libnes/cartridge.hpp>
#include <catch2/catch.hpp>

#include <libnes/mappers/mmc1.hpp>

TEST_CASE("MMC1 registers") {

    SECTION("shift register") {
        nes::mmc1_shift_register sr;

        SECTION("empty") {
            CHECK(sr.get_value() == std::nullopt);
        }

        SECTION("load one bit") {
            sr.load(1);

            CHECK(sr.get_value() == std::nullopt);
            CHECK_FALSE(sr.is_reset());
        }

        SECTION("load four bits") {
            sr.load(1);
            sr.load(0);
            sr.load(1);
            sr.load(0);

            CHECK(sr.get_value() == std::nullopt);
        }

        SECTION("load five bits") {
            sr.load(1);
            sr.load(1);
            sr.load(1);
            sr.load(0);
            sr.load(1);

            CHECK(sr.get_value() == 0b10111);
        }

        SECTION("load five bits - getting value mutates state") {
            sr.load(1);
            sr.load(1);
            sr.load(1);
            sr.load(0);
            sr.load(1);

            [[maybe_unused]] auto _ = sr.get_value();

            CHECK(sr.get_value() == std::nullopt);
        }

        SECTION("load five bits - higher bits are ignored") {
            sr.load(0x7f);
            sr.load(0x7e);
            sr.load(0x7f);
            sr.load(0x7e);
            sr.load(0x7f);

            CHECK(sr.get_value() == 0b10101);
        }
    }
}