#include <iostream>
#include <vector>
#include <ranges>
#include <string_view>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <string>

#include "libnes/cpu.hpp"

using namespace std::string_literals;

constexpr auto operator""_Kb(size_t const x) {
    return x * 1024;
}

auto load_rom(auto filename) {
    auto memory = std::vector<uint8_t>(64_Kb, 0);
    auto romfile = std::ifstream{filename, std::ifstream::binary};

    romfile.seekg(16); // header

    auto prg = std::vector<uint8_t>(16_Kb, 0);
    romfile.read(reinterpret_cast<char*>(prg.data()), prg.size());

    std::ranges::copy(prg, memory.begin() + 0x8000);
    std::ranges::copy(prg, memory.begin() + 0xC000);

    return memory;
}

struct grabbr {

    enum class access_type { read, write };

    void write(uint16_t addr, uint8_t value) {
        on_access(access_type::write, addr, value);
        mem[addr] = value;
    }

    auto read(uint16_t addr) {
        on_access(access_type::read, addr);
        return mem[addr];
    }

    void on_access(auto access, uint16_t addr, std::optional<uint8_t> value = std::nullopt) {
        if ((addr >= 0x2000 and addr <= 0x2007) or addr == 0x2014) {
            ppu_access = std::tuple{addr, access, value};
        }
    }

    void tick() { ++cycle; }

    std::vector<uint8_t> mem;
    int cycle{0};

    using record = std::tuple<uint16_t, access_type, std::optional<uint8_t>>;
    std::optional<record> ppu_access;
};

auto access_type(auto at) {
    return at == grabbr::access_type::read ? "READ" : "WRITE";
}

auto& print_data(auto& s, auto d) {
    return d.has_value()
        ? s << std::setw(2) <<  std::setfill('0') << static_cast<int>(d.value())
        : s << "";
}

int main(int argc, char* argv[]) {
    try {
        auto bus = grabbr{load_rom(argv[1])};
        auto cpu = nes::cpu{bus};

        bus.mem[0x2002] = 0x80;

        while(bus.cycle < 1000000) {
            bus.tick();
            cpu.tick();

            if (bus.ppu_access.has_value()) {
                auto [addr, type, data] = bus.ppu_access.value();
                std::cout
                        << std::dec << bus.cycle << '\t'
                        << access_type(type) << '\t'
                        << '$' << std::hex << addr << '\t';
                print_data(std::cout, data) << '\n';
                bus.ppu_access = std::nullopt;
            }
        }
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
}