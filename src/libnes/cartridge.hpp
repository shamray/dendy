#pragma once

#include <libnes/literals.hpp>
#include <libnes/ppu_name_table.hpp>

#include <cstdint>
#include <optional>

namespace nes {

struct ines_header{
    char name[4];
    std::uint8_t prg_rom_chunks;
    std::uint8_t chr_rom_chunks;
    std::uint8_t mapper1;
    std::uint8_t mapper2;
    std::uint8_t prg_ram_size;
    std::uint8_t tv_system1;
    std::uint8_t tv_system2;
    char unused[5];
};

static_assert(sizeof(ines_header) == 16);

class cartridge
{
public:
    virtual ~cartridge() = default;

    [[nodiscard]] virtual auto chr() const -> const pattern_table::memory_bank& = 0;
    [[nodiscard]] virtual auto mirroring() const -> name_table_mirroring = 0;

    [[nodiscard]] virtual auto write(std::uint16_t addr, std::uint8_t value) -> bool = 0;
    [[nodiscard]] virtual auto read(std::uint16_t addr) -> std::optional<std::uint8_t>  = 0;
};

class nrom final: public cartridge
{
public:
    nrom(std::vector<std::array<std::uint8_t, 16_Kb>> prg, std::array<std::uint8_t, 8_Kb> chr, name_table_mirroring mirroring)
        : prg_{std::move(prg)}
        , chr_{chr}
        , mirroring_{mirroring}
    {}

    [[nodiscard]] auto chr() const -> const pattern_table::memory_bank& override { return chr_; }
    [[nodiscard]] auto mirroring() const -> name_table_mirroring override { return mirroring_; }

    [[nodiscard]] auto write(std::uint16_t addr, std::uint8_t value) -> bool override {
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
    std::array<std::uint8_t, 8_Kb> chr_;
    name_table_mirroring mirroring_;

};

}