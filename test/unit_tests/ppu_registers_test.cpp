#include <libnes/ppu_registers.hpp>
#include <catch2/catch_all.hpp>

TEST_CASE("PPU - Control Register") {
    nes::control_register ctrl;

    CHECK(ctrl.value() == 0x00);

    SECTION("Assign") {
        ctrl.assign(0x55);
        CHECK(ctrl.value() == 0x55);
    }

    SECTION("VBLANK") {
        ctrl.assign(0x00);
        CHECK(!ctrl.raise_vblank_nmi());

        ctrl.assign(0x80);
        CHECK(ctrl.raise_vblank_nmi());
    }

    SECTION("Nametable Indices") {
        ctrl.assign(0x01);
        CHECK(ctrl.nametable_index_x() == 1);
        CHECK(ctrl.nametable_index_y() == 0);

        ctrl.assign(0x02);
        CHECK(ctrl.nametable_index_x() == 0);
        CHECK(ctrl.nametable_index_y() == 1);
    }
}