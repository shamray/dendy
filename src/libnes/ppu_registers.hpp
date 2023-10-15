#pragma once

#include <cstdint>


namespace nes
{
enum class sprite_size {
    sprite8x8,
    sprite8x16
};

class control_register
{
public:
    constexpr void assign(std::uint8_t v) noexcept {
        value_ = v;
    }
    [[nodiscard]] constexpr auto value() const noexcept {
        return value_;
    }

    [[nodiscard]] constexpr auto nametable_index_x() const {
        return static_cast<std::uint8_t>((value_ >> 0) & 0x01);
    }
    [[nodiscard]] constexpr auto nametable_index_y() const {
        return static_cast<std::uint8_t>((value_ >> 1) & 0x01);
    }

    constexpr void smb_hotfix() {
//        value_ &= 0xFE;
    }

    [[nodiscard]] constexpr auto vram_address_increment() const {
        return (value_ & 0x04) == 0
            ? std::int8_t{1}
            : std::int8_t{32};
    }

    [[nodiscard]] constexpr auto pattern_table_fg_index() const {
        return (value_ & 0x08) >> 3;
    }

    [[nodiscard]] constexpr auto pattern_table_bg_index() const {
        return (value_ & 0x10) >> 4;
    }

    [[nodiscard]] constexpr auto sprite_size() const {
        return (value_ & 0x20) == 0
            ? nes::sprite_size::sprite8x8
            : nes::sprite_size::sprite8x16;
    }

    [[nodiscard]] constexpr auto raise_vblank_nmi() const {
        return (value_ & 0x80) != 0;
    }

private:
    std::uint8_t value_{0};
};
}