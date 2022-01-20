#pragma once

#include <libnes/ppu.hpp>
#include <libnes/cpu.hpp>

namespace nes {

struct console_bus
{
    struct controller_hack
    {
        std::uint8_t keys{0};
        std::uint8_t snapshot{0};
    } j1;

    console_bus(std::tuple<std::array<std::uint8_t, 64_Kb>, std::array<std::uint8_t, 8_Kb>, nes::name_table_mirroring>&& rom, nes::ppu& ppu)
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

    nes::ppu& ppu;
};

class console
{
public:
    console(std::tuple<std::array<std::uint8_t, 64_Kb>, std::array<std::uint8_t, 8_Kb>, nes::name_table_mirroring>&& rom)
        : bus_{std::move(rom), ppu_}
    {
    }

    template <screen screen_t>
    void render_frame(screen_t& screen) {
        auto count = 0;
        for (;;++count) {
            cpu_.tick();

            ppu_.tick(screen); if (ppu_.is_frame_ready()) break;
            ppu_.tick(screen); if (ppu_.is_frame_ready()) break;
            ppu_.tick(screen); if (ppu_.is_frame_ready()) break;
        }
        assert(count == 29780 || count == 29781);
    }

    void controller_input(std::uint8_t keys) {
        bus_.j1.keys = keys;
    }

private:
    ppu ppu_{nes::DEFAULT_COLORS};
    console_bus bus_;
    cpu<console_bus> cpu_{bus_};
};

}