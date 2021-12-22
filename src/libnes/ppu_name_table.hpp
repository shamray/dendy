#pragma once

#include <cstdint>
#include <ranges>
#include <algorithm>
#include <stdexcept>
#include <array>

#include <libnes/literals.hpp>

namespace nes {

enum class name_table_mirroring { horizontal, vertical };

class name_table
{
public:
    name_table_mirroring mirroring{name_table_mirroring::vertical};

    constexpr void write(std::uint16_t addr, std::uint8_t value) {
        auto i = bank_index(addr);
        auto j = bank_offset(addr);

        vram_[i][j] = value;
    }
    [[nodiscard]] constexpr auto read(std::uint16_t addr) const {
        auto i = bank_index(addr);
        auto j = bank_offset(addr);
        return vram_[i][j];
    }

    [[nodiscard]] constexpr auto table(int bank) const -> auto& {
        return vram_[bank & 1];
    }

    name_table() {
        std::ranges::for_each(vram_, [](auto& x){ std::ranges::fill(x, 0); });
    }

private:
    [[nodiscard]] constexpr auto bank_index(std::uint16_t addr) const -> int {
        using enum name_table_mirroring;
        switch(mirroring) {
            case horizontal:    return (addr >> 11) & 0x01;
            case vertical:      return (addr >> 10) & 0x01;
        }
        throw std::logic_error("Unhandled mirroring scenario");
    }

    [[nodiscard]] constexpr static auto bank_offset(std::uint16_t addr) noexcept -> std::uint16_t  {
        return addr & 0x3FF;
    }

private:
    using bank = std::array<std::uint8_t, 2_Kb>;
    std::array<bank, 2> vram_;
};

}