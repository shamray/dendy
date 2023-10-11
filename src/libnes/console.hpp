#pragma once

#include <libnes/cartridge.hpp>
#include <libnes/cpu.hpp>
#include <libnes/mappers/mmc1.hpp>
#include <libnes/mappers/nrom.hpp>
#include <libnes/ppu.hpp>

namespace nes
{

template <typename T, typename U>
concept PPU = requires(T t, std::uint16_t address, std::uint8_t value, U u) {
    { t.write(address, value) } -> std::same_as<bool>;
    { t.read(address) } -> std::same_as<std::optional<std::uint8_t>>;
    { t.dma_write(address, u) };
};

template <typename P>
struct console_bus {
    struct controller_hack {
        std::uint8_t keys{0};
        std::uint8_t snapshot{0};
    } j1;

    explicit constexpr console_bus(P& ppu, cartridge* cartridge = nullptr)
        : ppu_{ppu} {

        load_cartridge(cartridge);
    }

    constexpr auto ppu() -> auto& {
        return ppu_.get();
    }

    constexpr void load_cartridge(cartridge* new_cartridge) {
        cartridge = new_cartridge;

        ppu().load_cartridge(cartridge);
        if (cartridge != nullptr)
            ppu().nametable_mirroring(cartridge->mirroring());
    }

    constexpr void eject_cartridge() {
        ppu().eject_cartridge();
        cartridge = nullptr;
    }

    [[nodiscard]] constexpr auto nmi() {
        if (not ppu().nmi_raised)
            return false;

        auto nmi_signal = ppu().nmi_raised and not ppu().nmi_seen;
        ppu().nmi_seen = true;

        return nmi_signal;
    }

    constexpr void write(std::uint16_t addr, std::uint8_t value) {
        if (ppu().write(addr, value))
            return;

        if (addr == 0x4014) {
            ppu().dma_write(value << 8U, [this](auto addr) { return read(addr); });
            return;
        }

        if (addr == 0x4016) {
            j1.snapshot = j1.keys;
        }

        if (addr < 0x2000) {
            mem[addr % 0x0800] = value;
        }

        if (cartridge != nullptr)
            cartridge->write(addr, value);
    }

    constexpr std::uint8_t read(std::uint16_t addr) {
        if (addr <= 0x1FFF) {
            return mem[addr & 0x07FF];
        }

        if (auto r = ppu().read(addr); r.has_value())
            return r.value();

        if (addr == 0x4016) {
            auto r = (j1.snapshot & 0x80) ? std::uint8_t{1} : std::uint8_t{0};
            j1.snapshot <<= 1;
            return r;
        }

        if (addr == 0x4017) {
            return 0;
        }

        if (auto r = cartridge->read(addr); r.has_value())
            return r.value();

        return 0;
    }

    cartridge* cartridge{nullptr};
    std::array<std::uint8_t, 2_Kb> mem{};

private:
    std::reference_wrapper<P> ppu_;
};

class console
{
public:
    using bus = console_bus<ppu>;
    using cpu = cpu<bus>;

    explicit console(std::unique_ptr<cartridge> rom)
        : cartridge_{std::move(rom)}
        , bus_{ppu_, cartridge_.get()} {
    }

    template <screen screen_t>
    void render_frame(screen_t& screen) {
        auto count = 0;
        for (;; ++count) {
            cpu_.tick();

            ppu_.tick(screen);
            if (ppu_.is_frame_ready()) break;
            ppu_.tick(screen);
            if (ppu_.is_frame_ready()) break;
            ppu_.tick(screen);
            if (ppu_.is_frame_ready()) break;
        }
        assert(count == 29780 || count == 29781);
    }

    void controller_input(std::uint8_t keys) {
        bus_.j1.keys = keys;
    }

private:
    std::unique_ptr<cartridge> cartridge_;
    ppu ppu_{nes::DEFAULT_COLORS};
    bus bus_{ppu_};
    cpu cpu_{bus_};
};

}// namespace nes