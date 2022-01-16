#pragma once

#include <concepts>

namespace nes {

struct point
{
    short x;
    short y;
};

auto operator== (nes::point a, nes::point b) {
    return a.x == b.x and a.y == b.y;
}

template <class S>
concept screen = requires(S s, point p, color c) {
    { s.draw_pixel(p, c) };
    { s.width() } -> std::same_as<short>;
    { s.height() } -> std::same_as<short>;
};

}
