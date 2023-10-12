#include <catch2/catch_all.hpp>
#include <libnes/ppu_name_table.hpp>

TEST_CASE("nametable") {

    SECTION("table 0") {
        auto nt = nes::name_table{[]() { return nes::name_table_mirroring::vertical; }};

        nt.write(0x007, 0x55);

        CHECK((int) nt.read(0x007) == 0x55);
        CHECK((int) nt.table(0)[0x007] == 0x55);
    }
    SECTION("vertical mirroring") {
        auto nt = nes::name_table{[]() { return nes::name_table_mirroring::vertical; }};

        nt.write(0x007, 0x55);
        nt.write(0x407, 0x11);

        CHECK((int) nt.read(0x007) == 0x55);
        CHECK((int) nt.read(0x807) == 0x55);

        CHECK((int) nt.read(0x407) == 0x11);
        CHECK((int) nt.read(0xC07) == 0x11);
    }

    SECTION("horizontal mirroring") {
        auto nt = nes::name_table{[]() { return nes::name_table_mirroring::horizontal; }};

        nt.write(0x007, 0x55);
        nt.write(0x807, 0x11);

        CHECK((int) nt.read(0x007) == 0x55);
        CHECK((int) nt.read(0x407) == 0x55);

        CHECK((int) nt.read(0x807) == 0x11);
        CHECK((int) nt.read(0xC07) == 0x11);
    }
}