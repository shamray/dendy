#pragma once

#include <cstddef>

namespace nes::inline literals
{
constexpr auto operator""_Kb(std::size_t const x) { return x * 1024; }
}
