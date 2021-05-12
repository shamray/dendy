#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "libnes/mos6502.h"

#include <ranges>
#include <array>

constexpr auto operator""_Kb(size_t const x) 
{
    return x * 1024;
}

class fixture
{
public:
    fixture()
        : mem(64_Kb, 0)
    {}

    void load(uint16_t addr, auto program)
    {
        std::ranges::copy(program, mem.begin() + addr);
    }

    std::vector<uint8_t> mem;

};

TEST_CASE_METHOD(fixture, "LDA-IMM")
{
    nes::mos6502 cpu(mem);

    load(0x8000, std::array{0xa9, 0x55});
    cpu.tick();

    CHECK(cpu.a == 0x55);
}

TEST_CASE_METHOD(fixture, "LDA-ZP")
{
    nes::mos6502 cpu(mem);

    load(0x0010, std::array{ 0x42 });
    load(0x8000, std::array{ 0xa5, 0x10 });
    cpu.tick();

    CHECK(cpu.a == 0x42);
}

TEST_CASE_METHOD(fixture, "LDX-IMM")
{
    nes::mos6502 cpu(mem);

    load(0x8000, std::array{ 0xa2, 0x42 });
    cpu.tick();

    CHECK(cpu.x == 0x42);
}

TEST_CASE_METHOD(fixture, "LDX-ZP")
{
    nes::mos6502 cpu(mem);

    load(0x0010, std::array{ 0x88 });
    load(0x8000, std::array{ 0xa6, 0x10 });
    cpu.tick();

    CHECK(cpu.x == 0x88);
}