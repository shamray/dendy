#include <catch2/catch.hpp>

/*
TEST_CASE("clock") {
    auto master_clock = nes::clock{21.477272_MHz};
    auto cpu_clock = nes::clock{master_clock, 12};
    auto ppu_clock = nes::clock{master_clock, 4};

    do {
        master_clock.tick();
        for (auto c: cpu_clock.poll_clock_events()) {
            cpu.tick();
        }
        for (auto c: ppu_clock.poll_clock_events()) {
            ppu.tick();
        }
    } while(!ppu.is_frame_ready());
}
 */

namespace nes
{
class clock
{
public:
    clock() = default;

    constexpr void tick() {
        ++ticks_;
    }

    constexpr auto pop_tick() {
        if (ticks_ == 0)
            return false;

        --ticks_;
        return true;
    }

    [[nodiscard]]
    constexpr auto ticks_happened() const {
        return ticks_;
    }

private:
    int ticks_{0};
};
}

template<bool B> bool static_test()
{
    static_assert(B);
    return B;
}

#if defined(RELAXED_CONSTEXPR)
#define TEST(X) X
#define CONSTEXPR
#else
#define TEST(X) static_test<X>()
#define CONSTEXPR constexpr
#endif

CONSTEXPR auto clock_push_ticks(nes::clock clock, int times) {
    for (auto i = 0; i < times; ++i) {
        clock.tick();
    }
    return clock;
}

CONSTEXPR auto clock_pop_ticks(nes::clock clock, int times) {
    for (auto i = 0; i < times; ++i) {
        clock.pop_tick();
    }
    return clock;
}

TEST_CASE("master clock, one tick") {
    CONSTEXPR auto clock = nes::clock{};
    CONSTEXPR auto after_1_tick = clock_push_ticks(clock, 1);

    SECTION("has one tick event") {
        CHECK(TEST(after_1_tick.ticks_happened() == 1));
    }

    SECTION("pop one tick event") {
        CONSTEXPR auto pop_1_tick = clock_pop_ticks(after_1_tick, 1);
        CHECK(TEST(pop_1_tick.ticks_happened() == 0));
    }
}
