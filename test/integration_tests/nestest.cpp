#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <fstream>
#include <ranges>

#include "libnes/cpu.h"

constexpr auto operator""_Kb(size_t const x)
{
    return x * 1024;
}

class test_cpu: public nes::cpu
{
public:
    test_cpu(std::vector<uint8_t>& memory)
        : nes::cpu(memory)
    {
        pc.assign(0xC000);
        p.set(nes::cpu_flag::int_disable);
    }
};

auto load_nestest()
{
    auto memory = std::vector<uint8_t>(64_Kb, 0);
    auto romfile = std::ifstream("rom/nestest.nes", std::ifstream::binary);

    romfile.seekg(16); // header

    auto prg = std::vector<uint8_t>(16_Kb, 0);
    romfile.read(reinterpret_cast<char*>(prg.data()), prg.size());

    std::ranges::copy(prg, memory.begin() + 0x8000);
    std::ranges::copy(prg, memory.begin() + 0xC000);

    return memory;
}

TEST_CASE("first")
{
    auto m = load_nestest();
    auto cpu = test_cpu{m};
    auto instruction_count = 0;
    auto cycle = 0;

    try
    {
        for (;; ++cycle) {
            if (cycle > 10000000)
                throw std::runtime_error("Probably an infinite loop");

            if (cpu.pc.value() == 0xC66E)
                break;

            if (!cpu.is_executing()) {
                UNSCOPED_INFO(std::hex << std::setw(4) << std::setfill('0') << cpu.pc.value() << '\t'
                << "A:" << std::setw(2) << std::setfill('0') << (int)cpu.a.value() << ' '
                << "X:" << std::setw(2) << std::setfill('0') << (int)cpu.x.value() << ' '
                << "Y:" << std::setw(2) << std::setfill('0') << (int)cpu.y.value() << ' '
                << "P:" << std::setw(2) << std::setfill('0') << (int)cpu.p.value() << ' '
                << "SP:" << std::setw(2) << std::setfill('0') << (int)cpu.s.value() << ' '
                << "m[00]:" << std::setw(2) << std::setfill('0') << (int)m[0] << ' '
                << "m[10]:" << std::setw(2) << std::setfill('0') << (int)m[0x10] << ' ');

                instruction_count++;
            }
            cpu.tick();
        }
    }
    catch(const std::exception& ex)
    {
        FAIL_CHECK(ex.what());
    }
    std::cout << std::dec << instruction_count << " instructions, " << cycle << " cycles" << std::endl;

    CHECK((int)m[0x02] == 0x00);
    CHECK((int)m[0x03] == 0x00);
}