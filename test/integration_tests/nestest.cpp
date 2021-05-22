#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <fstream>
#include <ranges>
#include <ctime>
#include <chrono>

#include "libnes/cpu.h"

constexpr auto operator""_Kb(size_t const x)
{
    return x * 1024;
}

class test_cpu: public nes::cpu
{
public:
    explicit test_cpu(std::vector<uint8_t>& memory)
        : nes::cpu(memory)
    {
        pc.assign(0xC000);

        s.assign(0xFF);
        write(s.push(), 0x19);
        write(s.push(), 0x82);

        p.set(nes::cpu_flag::int_disable);
    }

    [[nodiscard]]
    auto is_test_finished() const { return pc.value() == 0x1983; }

    std::ostream& print_status(std::ostream& stream)
    {
        return stream << std::hex << std::setw(4) << std::setfill('0') << pc.value() << '\t'
                      << "A:" << std::setw(2) << std::setfill('0') << (int)a.value() << ' '
                      << "X:" << std::setw(2) << std::setfill('0') << (int)x.value() << ' '
                      << "Y:" << std::setw(2) << std::setfill('0') << (int)y.value() << ' '
                      << "P:" << std::setw(2) << std::setfill('0') << (int)p.value() << ' '
                      << "SP:" << std::setw(2) << std::setfill('0') << (int)s.value();
    }
};

auto load_nestest()
{
    auto memory = std::vector<uint8_t>(64_Kb, 0);
    auto romfile = std::ifstream{"rom/nestest.nes", std::ifstream::binary};

    romfile.seekg(16); // header

    auto prg = std::vector<uint8_t>(16_Kb, 0);
    romfile.read(reinterpret_cast<char*>(prg.data()), prg.size());

    std::ranges::copy(prg, memory.begin() + 0x8000);
    std::ranges::copy(prg, memory.begin() + 0xC000);

    return memory;
}

TEST_CASE("The ultimate NES CPU test ROM (aka nestest)")
{
    auto m = load_nestest();
    auto cpu = test_cpu{m};

    auto log = std::ofstream{"nestest.log"};
    auto start_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    auto instruction_count = 0;
    auto cycle = 0;

    try
    {
        log << "nestest, test started: " << std::ctime(&start_time) << '\n' << std::endl;

        for (;!cpu.is_test_finished(); ++cycle) {
            if (cycle > 10000000)
                throw std::runtime_error("Probably an infinite loop");

            if (!cpu.is_executing()) {
                cpu.print_status(log) << '\n';
                instruction_count++;
            }
            cpu.tick();
        }
    }
    catch(const std::exception& ex)
    {
        FAIL_CHECK(ex.what());
    }

    log << std::endl;
    log << "test finished, " << std::dec << instruction_count << " instructions, " << cycle << " cycles" << std::endl;

    CHECK((int)m[0x02] == 0x00);
    CHECK((int)m[0x03] == 0x00);
}