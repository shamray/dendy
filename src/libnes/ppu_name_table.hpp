#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>
#include <stdexcept>
#include <functional>
#include <optional>

#include <libnes/literals.hpp>
#include <utility>

namespace nes
{

enum class name_table_mirroring {
    single_screen_lo,
    single_screen_hi,
    vertical,
    horizontal,
};

class name_table
{
public:
    using mirroring_callback = std::function<std::optional<name_table_mirroring>()>;
    explicit name_table(mirroring_callback mirroring)
        : mirroring_{std::move(mirroring)} {}

    void write(std::uint16_t addr, std::uint8_t value) {
        auto i = bank_index(addr);
        auto j = bank_offset(addr);

        vram_[i][j] = value;
    }
    [[nodiscard]] auto read(std::uint16_t addr) const {
        auto i = bank_index(addr);
        auto j = bank_offset(addr);
        return vram_[i][j];
    }

    [[nodiscard]] constexpr auto table(int bank) const -> auto& {
        return vram_[bank & 1];
    }

private:
    [[nodiscard]] auto bank_index(std::uint16_t addr) const -> std::size_t {
        using enum name_table_mirroring;
        const auto mirroring = mirroring_().value_or(vertical);
        switch (mirroring) {
            case horizontal:
                return (addr >> 11u) & 0x01u;
            case vertical:
                return (addr >> 10u) & 0x01u;
            case single_screen_lo:
                return 0;
            case single_screen_hi:
                return 1;
        }
        std::unreachable();
    }

    [[nodiscard]] constexpr static auto bank_offset(std::uint16_t addr) noexcept -> std::uint16_t {
        return addr & 0x3FFu;
    }

private:
    using bank = std::array<std::uint8_t, 2_Kb>;
    std::array<bank, 2> vram_{};
    mirroring_callback mirroring_;
};

}// namespace nes