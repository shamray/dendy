#include <libnes/ppu.hpp>
#include <libnes/literals.hpp>

#include <catch2/catch.hpp>

#include <ranges>
#include <unordered_map>

using namespace nes::literals;

namespace {

void tick(auto& ppu, auto& screen, int times = 1) {
    for (auto i = 0; i < times; ++i) {
        ppu.tick(screen);
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

template <class... args_t>
void write(std::uint16_t addr, nes::ppu& ppu, std::uint8_t byte, args_t... args) {
    ppu.write(addr, byte);
    write(addr, ppu, args...);
}

template <>
void write(std::uint16_t addr, nes::ppu& ppu, std::uint8_t byte) {
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

}

TEST_CASE("PPU") {
    auto screen = test_screen{};
    auto ppu = nes::ppu{nes::DEFAULT_COLORS};

    constexpr auto BLACK = nes::DEFAULT_COLORS[63];
    constexpr auto VIOLET = nes::DEFAULT_COLORS[3];
    constexpr auto OLIVE = nes::DEFAULT_COLORS[8];
    constexpr auto RASPBERRY = nes::DEFAULT_COLORS[21];
    constexpr auto ORANGE = nes::DEFAULT_COLORS[22];
    constexpr auto BLUE = nes::DEFAULT_COLORS[33];
    constexpr auto CYAN = nes::DEFAULT_COLORS[44];
    constexpr auto WHITE = nes::DEFAULT_COLORS[48];

    SECTION("power up state") {
        CHECK(ppu.control == 0x00);
        CHECK(ppu.mask == 0x00);
    }

    SECTION("start of the frame") {
        ppu.status |= 0x40;
        ppu.tick(screen);

        CHECK((ppu.status & 0x40) == 0);
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

    SECTION("loading sprites") {
        SECTION("OAMADDR/OAMDATA") {
            write(0x2003, ppu, 1 * sizeof(nes::sprite));

            auto sprite = nes::sprite{.y = 0, .tile = 1, .attr = 0x00, .x = 0};
            write(0x2004, ppu, sprite.y);
            write(0x2004, ppu, sprite.tile);
            write(0x2004, ppu, sprite.attr);
            write(0x2004, ppu, sprite.x);

            CHECK(ppu.oam().sprites[1] == sprite);
        }

        SECTION("DMA") {
            auto sprites = std::array<nes::sprite, 64>{};
            auto mempage = reinterpret_cast<std::uint8_t*>(sprites.data());

            sprites[1] = nes::sprite{.y = 0, .tile = 1, .attr = 0x00, .x = 0};

            ppu.dma_write(mempage);

            CHECK(ppu.oam().sprites == sprites);
        }
    }

    SECTION ("rendering frame") {
        // Palette
        write(0x2006, ppu, 0x3F, 0x00);
        write(0x2007, ppu,
              63,
              3 , 8 , 21, 63,
              48, 33, 22, 63,
              0 , 0 , 0 , 63,
              0 , 0 , 0 , 63,
              33, 22, 44, 63,
              8 , 21, 48, 63,
              0 , 0 , 0 , 63,
              0 , 0 , 0 , 63
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

        SECTION("computing nametable addresses") {

            SECTION("nametable 0") {
                CHECK(ppu.nametable_address(0, 0) == 0x0000);
            }
            SECTION("nametable 1") {
                CHECK(ppu.nametable_address(1, 0) == 0x0400);
            }
            SECTION("nametable 2") {
                CHECK(ppu.nametable_address(0, 1) == 0x0800);
            }
            SECTION("nametable 3") {
                CHECK(ppu.nametable_address(1, 1) == 0x0C00);
            }
        }

        SECTION("rendering background") {
            SECTION("point at (0,0)") {
                write(0x2006, ppu, 0x20, 0x00); // Nametable
                write(0x2007, ppu, 1);

                tick(ppu, screen, 242 * 341);           // Wait one frame

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

                tick(ppu, screen, 242 * 341);           // Wait one frame

                CHECK(screen.pixels.at(nes::point{7, 1}) == RASPBERRY);
            }

            SECTION("point at (15,1)") {
                write(0x2006, ppu, 0x20, 0x00); // Nametable
                write(0x2007, ppu, 0, 42);

                tick(ppu, screen, 242 * 341);           // Wait one frame

                CHECK(screen.pixels.at(nes::point{15, 1}) == RASPBERRY);
            }

            SECTION("4 palette colors") {
                write(0x2006, ppu, 0x20, 0x00); // Nametable
                write(0x2007, ppu, 99);

                tick(ppu, screen, 242 * 341);           // Wait one frame

                CHECK(screen.pixels.at(nes::point{0, 0}) == RASPBERRY);
                CHECK(screen.pixels.at(nes::point{1, 0}) == OLIVE);
                CHECK(screen.pixels.at(nes::point{2, 0}) == VIOLET);
                CHECK(screen.pixels.at(nes::point{3, 0}) == BLACK);
            }

            SECTION("scroll X") {
                write(0x2006, ppu, 0x20, 0x00); // Nametable
                write(0x2007, ppu, 0, 42);
                write(0x2006, ppu, 0x24, 0x00); // Nametable
                write(0x2007, ppu, 42);

                SECTION("1 pixel") {
                    write(0x2005, ppu, 1, 0);
                    tick(ppu, screen, 242 * 341);       // Wait one frame

                    CHECK(screen.pixels.at(nes::point{14, 1}) == RASPBERRY);
                }
                SECTION("8 pixels") {
                    write(0x2005, ppu, 8, 0);
                    tick(ppu, screen, 242 * 341);       // Wait one frame

                    CHECK(screen.pixels.at(nes::point{7, 1}) == RASPBERRY);
                    CHECK(screen.pixels.at(nes::point{255, 1}) == RASPBERRY);
                }
                SECTION("201 pixels") {
                    write(0x2005, ppu, 201, 0);
                    tick(ppu, screen, 242 * 341);       // Wait one frame

                    CHECK(screen.pixels.at(nes::point{62, 1}) == RASPBERRY);
                }
                SECTION("Flip nametables") {
                    tick(ppu, screen, 1 * 341);         // Wait prerender scanline

                    write(0x2000, ppu, 0x01);   // Make nametable #1 base nametable
                    tick(ppu, screen, 241 * 341);       // Wait one frame

                    CHECK(screen.pixels.at(nes::point{7, 1}) == RASPBERRY);
                }
            }

            SECTION("scroll Y") {
                ppu.nametable_mirroring(nes::name_table_mirroring::horizontal);

                write(0x2006, ppu, 0x20, 0x00); // Nametable
                write(0x2007, ppu, 0, 42);
                write(0x2006, ppu, 0x24, 0x00); // Nametable
                write(0x2007, ppu, 42);

                SECTION("1 pixel") {
                    write(0x2005, ppu, 0, 1);
                    tick(ppu, screen, 242 * 341);       // Wait one frame

                    CHECK(screen.pixels.at(nes::point{15, 0}) == RASPBERRY);
                }
            }
        }

        SECTION("rendering sprites") {
            auto sprites = std::array<nes::sprite, 64>{};
            auto mempage = reinterpret_cast<std::uint8_t*>(sprites.data());

            SECTION("single point, sprite at (0, 0)") {
                sprites[1] = nes::sprite{.y = 0, .tile = 1, .attr = 0x00, .x = 0};
                ppu.dma_write(mempage);

                tick(ppu, screen, 242 * 341); // Wait one frame

                CHECK(screen.pixels.at(nes::point{0, 0}) == CYAN);
            }
            SECTION("single point, sprite at (3, 2)") {
                sprites[1] = nes::sprite{.y = 2, .tile = 1, .attr = 0x00, .x = 3};
                ppu.dma_write(mempage);

                tick(ppu, screen, 242 * 341); // Wait one frame

                CHECK(screen.pixels.at(nes::point{3, 2}) == CYAN);
            }
            SECTION("single point, sprite palette #1") {
                sprites[1] = nes::sprite{.y = 0, .tile = 1, .attr = 0x01, .x = 0};
                ppu.dma_write(mempage);

                tick(ppu, screen, 242 * 341); // Wait one frame

                CHECK(screen.pixels.at(nes::point{0, 0}) == WHITE);
            }
            SECTION("single point, flip vertically") {
                sprites[1] = nes::sprite{.y = 0, .tile = 1, .attr = 0x80, .x = 0};
                ppu.dma_write(mempage);

                tick(ppu, screen, 242 * 341); // Wait one frame

                CHECK(screen.pixels.at(nes::point{0, 7}) == CYAN);
            }
            SECTION("single point, flip horizontally") {
                sprites[1] = nes::sprite{.y = 0, .tile = 1, .attr = 0x40, .x = 0};
                ppu.dma_write(mempage);

                tick(ppu, screen, 242 * 341); // Wait one frame

                CHECK(screen.pixels.at(nes::point{7, 0}) == CYAN);
            }

            SECTION("sprite 0 hit") {
                sprites[0] = nes::sprite{.y = 0, .tile = 1, .attr = 0x00, .x = 128};
                ppu.dma_write(mempage);

                write(0x2006, ppu, 0x20, 0x10); // Nametable
                write(0x2007, ppu, 1);

                REQUIRE((ppu.status & 0x40) == 0);
                tick(ppu, screen, 1 * 341);             // Wait prerender scanline
                tick(ppu, screen, 2 + 129);             // Wait for first pixel to hit sprite 0

                CHECK((ppu.status & 0x40) != 0);
            }
        }
    }
}