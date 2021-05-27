#pragma once

namespace nes
{

inline namespace literals
{
constexpr auto operator""_Kb(size_t const x) { return x * 1024; }
}

}