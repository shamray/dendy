#pragma once

#include <libnes/ppu.hpp>

namespace nes {

template <screen screen_t>
struct console
{
    struct controller_hack
    {
        std::uint8_t keys{0};
        std::uint8_t snapshot{0};
    } j1;

    console(std::tuple<std::array<std::uint8_t, 64_Kb>, std::array<std::uint8_t, 8_Kb>, nes::name_table_mirroring>&& rom, nes::ppu<screen_t>& ppu)
        : mem{std::move(std::get<0>(rom))}
        , chr{std::move(std::get<1>(rom))}
        , mirroring{std::get<2>(rom)}
        , ppu{ppu}
    {
        ppu.connect_pattern_table(&chr);
        ppu.nametable_mirroring(mirroring);
    }

    auto nmi() {
        if (not ppu.nmi)
            return false;

        ppu.nmi = false;
        return true;
    }

    void write(std::uint16_t addr, std::uint8_t value) {
        if (ppu.write(addr, value))
            return;

        if (addr == 0x4014) {
            ppu.dma_write(&mem[value << 8]);
            return;
        }

        if (addr == 0x4016) {
            j1.snapshot = j1.keys;
        }

        mem[addr] = value;
    }

    std::uint8_t read(std::uint16_t addr) {
        if (auto r = ppu.read(addr); r.has_value())
            return r.value();

        if (addr == 0x4016) {
            auto r = (j1.snapshot & 0x80) ? 1 : 0;
            j1.snapshot <<= 1;
            return r;
        }

        return mem[addr];
    }

    std::array<std::uint8_t, 64_Kb>  mem;
    std::array<std::uint8_t, 8_Kb>   chr;

    nes::name_table_mirroring mirroring;

    nes::ppu<screen_t>& ppu;
};
}