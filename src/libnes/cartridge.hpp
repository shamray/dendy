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
    cartridge(std::vector<std::array<std::uint8_t, 16_Kb>> prg, std::array<std::uint8_t, 8_Kb> chr, name_table_mirroring mirroring)
        : prg_{prg}
        , chr_{chr}
        , mirroring_{mirroring}
    {}

    [[nodiscard]] auto chr() const -> auto& { return chr_; }
    [[nodiscard]] auto mirroring() const { return mirroring_; }

    [[nodiscard]] auto write(std::uint16_t addr, std::uint8_t value) -> bool {
        return false;
    }

    [[nodiscard]] auto read(std::uint16_t addr) -> std::optional<std::uint8_t> {
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