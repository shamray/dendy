#pragma once

#include <ranges>
#include <vector>
#include <functional>
#include <algorithm>

namespace nes
{

class clock
{
public:
    clock() = default;
    clock(clock& source, int division_factor) : division_factor_(division_factor) {
        source.add_watcher([this](){ tick(); });
    }

    void tick() noexcept {
        ++ticks_;
        std::ranges::for_each(watchers_, [](auto f) { f(); });
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

    void add_watcher(auto watcher) {
        watchers_.push_back(watcher);
    }

private:
    int ticks_{0};
    int division_factor_{1};
    std::vector<std::function<void()>> watchers_;
};

}
