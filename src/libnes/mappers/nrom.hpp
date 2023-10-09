#pragma once

#include <libnes/cartridge.hpp>
#include <libnes/ppu_name_table.hpp>

#include <array>
#include <optional>
#include <vector>

namespace nes
{

class nrom final: public cartridge
{
public:
    nrom(std::vector<std::array<std::uint8_t, 16_Kb>> prg, std::array<std::uint8_t, 8_Kb> chr, name_table_mirroring mirroring)
        : prg_{std::move(prg)}
        , chr_{chr}
        , mirroring_{mirroring} {}

    [[nodiscard]] auto chr() const -> const pattern_table::memory_bank& override { return chr_; }
    [[nodiscard]] auto mirroring() const -> name_table_mirroring override { return mirroring_; }

    auto write([[maybe_unused]] std::uint16_t addr, [[maybe_unused]] std::uint8_t value) -> bool override {
        return false;
    }

    [[nodiscard]] auto read(std::uint16_t addr) -> std::optional<std::uint8_t> override {
        if (addr >= 0x8000 and addr <= 0xBFFF) {
            auto address = addr & 0x3FFFu;
            auto& prg = prg_.front();

            return prg[address];
        }

        if (addr >= 0x8000 and addr <= 0xFFFF) {
            auto address = addr & 0x3FFFu;
            auto& prg = prg_.back();

            return prg[address];
        }

        return std::nullopt;
    }

private:
    std::vector<std::array<std::uint8_t, 16_Kb>> prg_;
    std::array<std::uint8_t, 8_Kb> chr_;
    name_table_mirroring mirroring_;
};

}