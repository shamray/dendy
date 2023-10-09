#pragma once

#include <libnes/color.hpp>

#include <array>
#include <cstdint>

namespace nes
{

class palette_table
{
public:
    explicit constexpr palette_table(const auto& system_color_palette)
        : system_colors_{system_color_palette} {
        palette_ram_.fill(0);
    }

    [[nodiscard]] static constexpr auto palette_address(std::uint8_t address) noexcept {
        address &= 0x1F;

        if ((address & 0x13) == 0x10) {
            address &= ~0x10;
        }

        return address;
    }

    [[nodiscard]] constexpr auto read(std::uint8_t address) const noexcept {
        return palette_ram_[palette_address(address)];
    }

    constexpr void write(std::uint8_t address, std::uint8_t value) noexcept {
        palette_ram_[palette_address(address)] = value;
    }

    [[nodiscard]] auto color_of(std::uint8_t pixel, std::uint8_t palette) const noexcept -> color {
        auto rpc = pixel ? read((palette << 2) + pixel) : read(0x00);
        return system_colors_[rpc & 0x3F];
    }

private:
    std::array<std::uint8_t, 32> palette_ram_{};
    const std::array<color, 64>& system_colors_;
};

}