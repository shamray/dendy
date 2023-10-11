#include <catch2/catch_all.hpp>

#include <libnes/console.hpp>

using namespace nes::literals;

struct bus_test {
    struct test_ppu {
        void write(std::uint16_t addr, std::uint8_t value) { mem[addr] = value; }
        [[nodiscard]] auto read(std::uint16_t addr) const -> std::optional<std::uint8_t> { return mem[addr]; }

        void load_cartridge(nes::cartridge* rom) noexcept { cartridge = rom; }
        void eject_cartridge() noexcept { load_cartridge(nullptr); }
        void nametable_mirroring(auto) noexcept {}

        std::vector<std::uint8_t> mem{};
        nes::cartridge* cartridge{nullptr};
    };

    struct test_cartridge: nes::cartridge {
        std::array<std::uint8_t, 8_Kb> cart_chr{};
        nes::name_table_mirroring cart_mirroring{nes::name_table_mirroring::vertical};

        [[nodiscard]] auto chr() const -> const nes::pattern_table::memory_bank& override {
            return cart_chr;
        }

        [[nodiscard]] auto mirroring() const -> nes::name_table_mirroring override {
            return cart_mirroring;
        }

        auto write([[maybe_unused]] std::uint16_t addr, [[maybe_unused]] std::uint8_t value) -> bool override {
            return false;
        }

        [[nodiscard]] auto read([[maybe_unused]] std::uint16_t addr) -> std::optional<std::uint8_t> override {
            return std::nullopt;
        }
    };

    using test_bus = nes::console_bus<test_ppu>;

    test_cartridge cartridge;
    test_ppu ppu;
    test_bus bus{ppu, &cartridge};
};


TEST_CASE_METHOD(bus_test, "create bus") {
    SECTION("create bus with no cartridge") {
        auto new_bus = test_bus{ppu};
        CHECK(new_bus.cartridge == nullptr);
        CHECK(ppu.cartridge == nullptr);
    }

    SECTION("create bus with cartridge loaded") {
        auto new_bus = test_bus{ppu, &cartridge};
        CHECK(new_bus.cartridge == &cartridge);
    }

    SECTION("create bus, then load cartridge") {
        auto new_bus = test_bus{ppu};

        new_bus.load_cartridge(&cartridge);

        CHECK(new_bus.cartridge == &cartridge);
        CHECK(ppu.cartridge == &cartridge);
    }

    SECTION("create bus, eject cartridge") {
        auto new_bus = test_bus{ppu, &cartridge};
        REQUIRE(new_bus.cartridge == &cartridge);

        new_bus.eject_cartridge();

        CHECK(new_bus.cartridge == nullptr);
        CHECK(ppu.cartridge == nullptr);
    }

}