#pragma once

#include <libnes/literals.hpp>
#include <libnes/ppu_name_table.hpp>
#include <libnes/ppu_pattern_table.hpp>

#include <cstdint>
#include <optional>
#include <vector>

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

    virtual auto write(std::uint16_t addr, std::uint8_t value) -> bool = 0;
    [[nodiscard]] virtual auto read(std::uint16_t addr) -> std::optional<std::uint8_t>  = 0;
};

}