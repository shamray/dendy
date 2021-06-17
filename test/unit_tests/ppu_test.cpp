#include <libnes/ppu.hpp>
#include <libnes/literals.hpp>
#include <catch2/catch.hpp>
#include <ranges>

using namespace nes::literals;

void tick(nes::crt_scan& scan, int times = 1) {
    for (auto i = 0; i < times; ++i) {
        scan.advance();
    }
}

void tick(auto& ppu, int times = 1) {
    for (auto i = 0; i < times; ++i) {
        ppu.tick();
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

TEST_CASE("palette address") {
    using pt = nes::palette_table;

    SECTION("zeroeth and first") {
        CHECK(pt::palette_address(0x00) == 0x00);
        CHECK(pt::palette_address(0x01) == 0x01);
    }

    SECTION("random colors in the middle") {
        CHECK(pt::palette_address(0x07) == 0x07);
        CHECK(pt::palette_address(0x1D) == 0x1D);
        CHECK(pt::palette_address(0x13) == 0x13);
    }

    SECTION("out of range") {
        CHECK(pt::palette_address(0x20) == 0x00);
        CHECK(pt::palette_address(0x21) == 0x01);
    }

    SECTION("mapping background color") {
        CHECK(pt::palette_address(0x10) == 0x00);
    }
}

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

namespace nes
{
auto operator== (nes::color a, nes::color b) {
    return a.value() == b.value();
}

auto operator== (nes::point a, nes::point b) {
    return a.x == b.x and a.y == b.y;
}

std::ostream& operator<< (std::ostream& s, nes::color c) {
    return s << ( c.value() >> 16 & 0xFF) << "," << ( c.value() >> 8 & 0xFF) << "," << ( c.value() & 0xFF);
}
}

struct test_screen
{
    class hasher
    {
    public:
        [[nodiscard]] constexpr auto operator()(nes::point p) const noexcept {
            return p.x << 8 | p.y;
        }
    };

    std::unordered_map<nes::point, nes::color, hasher> pixels;

    [[nodiscard]] constexpr static auto width() -> short { return 256; }
    [[nodiscard]] constexpr static auto height() -> short { return 240; }

    void draw_pixel(nes::point where, nes::color color) {
        pixels[where] = color;
    }
};

template <class ppu_t, class... args_t>
void write(std::uint16_t addr, ppu_t& ppu, std::uint8_t byte, args_t... args) {
    ppu.write(addr, byte);
    write(addr, ppu, args...);
}

template <class ppu_t>
void write(std::uint16_t addr, ppu_t& ppu, std::uint8_t byte) {
    ppu.write(addr, byte);
}

auto empty_pattern_table() {
    return std::array<std::uint8_t, 8_Kb>{0x00};
}

template <class input_type>
auto pattern_table(std::uint8_t index, input_type&& tile) {
    assert(tile.size() == 16);
    auto chr = empty_pattern_table();
    std::ranges::copy(tile, std::next(std::begin(chr), index * tile.size()));
    return chr;
}

template <class input_type, class... args_t>
auto pattern_table(std::uint8_t index, input_type&& tile, args_t... args) {
    assert(tile.size() == 16);
    auto chr = pattern_table(args...);
    std::ranges::copy(tile, std::next(std::begin(chr), index * tile.size()));
    return chr;
}

TEST_CASE("PPU") {
    auto screen = test_screen{};
    auto ppu = nes::ppu{nes::DEFAULT_COLORS, screen};

    constexpr auto BLACK = nes::DEFAULT_COLORS[63];
    constexpr auto VIOLET = nes::DEFAULT_COLORS[3];
    constexpr auto OLIVE = nes::DEFAULT_COLORS[8];
    constexpr auto RASPBERRY = nes::DEFAULT_COLORS[21];

    SECTION("power up state") {
        CHECK(ppu.control == 0x00);
        CHECK(ppu.mask == 0x00);
    }

    SECTION("write to palette ram") {
        write(0x2000, ppu, 0x00);
        write(0x2006, ppu, 0x3F, 0x00);
        write(0x2007, ppu,
            63,
            3 , 8 , 21
        );

        CHECK(ppu.palette_table().read(0) == 63);
        CHECK(ppu.palette_table().read(1) == 3);
        CHECK(ppu.palette_table().read(2) == 8);
        CHECK(ppu.palette_table().read(3) == 21);
    }

    SECTION("rendering background") {
        // Palette
        write(0x2006, ppu, 0x3F, 0x00);
        write(0x2007, ppu,
              63,
              3 , 8 , 21
        );

        // Pattern table
        auto chr = pattern_table(
                1 , std::array{0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // single point in top left corner
                               0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

                42, std::array{0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // single point in the second row
                               0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

                99, std::array{0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // points in top row; colors: 3, 2, 1, 0
                               0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
        );
        ppu.connect_pattern_table(&chr);

        SECTION("point at (0,0)") {
            write(0x2006, ppu, 0x20, 0x00); // Nametable
            write(0x2007, ppu, 1);

            tick(ppu, 240 * 341);           // Wait one frame

            CHECK(screen.pixels.at(nes::point{0, 0}) == RASPBERRY);
            CHECK(screen.pixels.at(nes::point{0, 0}) != BLACK);

            for (auto y = 1; y < 240; ++y) {
                for (auto x = 1; x < 256; ++x) {
                    CHECK(screen.pixels.at(nes::point{1, 1}) == BLACK);
                }
            }
        }

        SECTION("point at (7,1)") {
            write(0x2006, ppu, 0x20, 0x00); // Nametable
            write(0x2007, ppu, 42);

            tick(ppu, 240 * 341);           // Wait one frame

            CHECK(screen.pixels.at(nes::point{7, 1}) == RASPBERRY);
        }

        SECTION("point at (15,1)") {
            write(0x2006, ppu, 0x20, 0x00); // Nametable
            write(0x2007, ppu, 0, 42);

            tick(ppu, 240 * 341);           // Wait one frame

            CHECK(screen.pixels.at(nes::point{15, 1}) == RASPBERRY);
        }

        SECTION("4 palette colors") {
            write(0x2006, ppu, 0x20, 0x00); // Nametable
            write(0x2007, ppu, 99);

            tick(ppu, 240 * 341);           // Wait one frame

            CHECK(screen.pixels.at(nes::point{0, 0}) == RASPBERRY);
            CHECK(screen.pixels.at(nes::point{1, 0}) == OLIVE);
            CHECK(screen.pixels.at(nes::point{2, 0}) == VIOLET);
            CHECK(screen.pixels.at(nes::point{3, 0}) == BLACK);
        }
    }
}