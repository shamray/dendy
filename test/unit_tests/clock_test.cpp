#include <catch2/catch.hpp>

/*
TEST_CASE("primary_clock") {
    auto master_clock = nes::primary_clock{21.477272_MHz};
    auto cpu_clock = nes::primary_clock{master_clock, 12};
    auto ppu_clock = nes::primary_clock{master_clock, 4};

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

#include <ranges>

namespace nes
{

class primary_clock
{
public:
    primary_clock() = default;

    void tick() noexcept {
        ++ticks_;
        std::ranges::for_each(watchers_, [](auto f) { f(); });
    }

    auto pop_tick() noexcept {
        if (ticks_ == 0)
            return false;

        --ticks_;
        return true;
    }

    [[nodiscard]]
    auto ticks_happened() const noexcept {
        return ticks_;
    }

    auto add_watcher(auto watcher) {
        watchers_.push_back(watcher);
    }

private:
    int ticks_{0};
    std::vector<std::function<void()>> watchers_;
};

class secondary_clock
{
public:
    secondary_clock(primary_clock& source, int division_factor) : division_factor_(division_factor) {
        source.add_watcher([this](){ on_primary_tick(); });
    }

    void on_primary_tick() {
        ++ticks_;
    }

    [[nodiscard]]
    auto ticks_happened() const noexcept {
        return ticks_ / division_factor_;
    }

    auto pop_tick() noexcept {
        if (ticks_happened() == 0)
            return false;

        --ticks_;
        return true;
    }

private:
    int division_factor_{1};
    int ticks_{0};
};
}

auto clock_push_ticks(nes::primary_clock clock, int times) {
    for (auto i = 0; i < times; ++i) {
        clock.tick();
    }
    return clock;
}

auto clock_pop_ticks(nes::primary_clock clock, int times) {
    for (auto i = 0; i < times; ++i) {
        clock.pop_tick();
    }
    return clock;
}

TEST_CASE("master primary_clock, one tick") {
    auto clock = nes::primary_clock{};
    auto after_1_tick = clock_push_ticks(clock, 1);

    SECTION("has one tick event") {
        CHECK(after_1_tick.ticks_happened() == 1);
    }

    SECTION("pop one tick event") {
        auto pop_1_tick = clock_pop_ticks(after_1_tick, 1);
        CHECK(pop_1_tick.ticks_happened() == 0);
    }

    SECTION("pop ten tick events") {
        auto pop_ticks = clock_pop_ticks(after_1_tick, 10);
        CHECK(pop_ticks.ticks_happened() == 0);
    }
}

TEST_CASE("master primary_clock, two ticks") {
    auto clock = nes::primary_clock{};
    auto after_2_ticks = clock_push_ticks(clock, 2);

    SECTION("has two tick event") {
        CHECK(after_2_ticks.ticks_happened() == 2);
    }

    SECTION("pop one tick event") {
        auto pop_1_tick = clock_pop_ticks(after_2_ticks, 1);
        CHECK(pop_1_tick.ticks_happened() == 1);
    }

    SECTION("pop two tick events") {
        auto pop_2_ticks = clock_pop_ticks(after_2_ticks, 2);
        CHECK(pop_2_ticks.ticks_happened() == 0);
    }
}

TEST_CASE("divided primary_clock") {
    auto master_clock = nes::primary_clock{};
    nes::secondary_clock slave_clock(master_clock, 3);
    master_clock.tick();
    master_clock.tick();

    REQUIRE(slave_clock.ticks_happened() == 0);

    master_clock.tick();
    CHECK(slave_clock.ticks_happened() == 1);
}