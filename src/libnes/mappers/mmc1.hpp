#pragma once

#include <libnes/ppu_name_table.hpp>
#include <libnes/cartridge.hpp>

#include <vector>
#include <optional>
#include <array>

namespace nes {

class mmc1_registers
{
    enum class prg_mode: std::uint8_t { switch32k, switch_16k_lo, switch_16k_hi};
    enum class chr_mode: std::uint8_t { switch8k, switch4k };
    enum class mirroring_mode: std::uint8_t { one_screen_lo, one_screen_hi, vertical, horizontal};

    constexpr void load(std::uint8_t byte);

    [[nodiscard]] constexpr auto control() const -> std::uint8_t;
    [[nodiscard]] constexpr auto mirroring() const -> name_table_mirroring;

    [[nodiscard]] constexpr auto chr0() const -> std::uint8_t;
    [[nodiscard]] constexpr auto chr1() const -> std::uint8_t;
    [[nodiscard]] constexpr auto prg() const -> std::uint8_t;
};

class mmc1 final: public cartridge
{
public:
    mmc1(std::vector<std::array<std::uint8_t, 16_Kb>> prg, std::vector<std::array<std::uint8_t, 4_Kb>> chr)
        : prg_{std::move(prg)}
        , chr_{std::move(chr)}
    {}

    [[nodiscard]] auto chr() const -> const pattern_table::memory_bank& override {
        memcpy(mapped_chr_.data(),          chr_[0].data(), 4_Kb);
        memcpy(mapped_chr_.data() + 4_Kb,   chr_[1].data(), 4_Kb);
        return mapped_chr_;
    }

    [[nodiscard]] auto mirroring() const -> name_table_mirroring override { return mirroring_; }

    auto write(std::uint16_t addr, std::uint8_t value) -> bool override {
        if (addr >= 0x8000 and addr <= 0xFFFF)
            return true;
        return false;
    }

    [[nodiscard]] auto read(std::uint16_t addr) -> std::optional<std::uint8_t> override {
        if (addr >= 0x8000 and addr <= 0xBFFF) {
            auto address = addr & 0x3FFF;
            auto& prg = prg_.front();

            return prg[address];
        }

        if (addr >= 0x8000 and addr <= 0xFFFF) {
            auto address = addr & 0x3FFF;
            auto& prg = prg_.back();

            return prg[address];
        }

        return std::nullopt;
    }

private:
    std::vector<std::array<std::uint8_t, 16_Kb>> prg_;
    std::vector<std::array<std::uint8_t, 4_Kb>> chr_;
    name_table_mirroring mirroring_{name_table_mirroring::horizontal};

    mutable std::array<std::uint8_t, 8_Kb> mapped_chr_{};
};

}
