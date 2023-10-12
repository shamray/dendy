#pragma once

namespace nes::inline literals
{
constexpr auto operator""_Kb(unsigned long long x) { return x * 1024; }
}
