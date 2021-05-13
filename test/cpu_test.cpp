#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "libnes/cpu.h"

#include <ranges>
#include <array>

constexpr auto operator""_Kb(size_t const x) 
{
    return x * 1024;
}

class cpu_test
{
public:
    cpu_test()
        : mem(64_Kb, 0)
    {
        load(0xFFFC, std::array{0x00, 0x80});
    }

    void load(uint16_t addr, auto program)
    {
        std::ranges::copy(program, mem.begin() + addr);
    }

    std::vector<uint8_t> mem;
    uint16_t prgadr{0x8000};

};

TEST_CASE_METHOD(cpu_test, "read_word")
{
    nes::cpu cpu(mem);

    load(0x0017, std::array{0x10, 0xd0}); // $D010

    CHECK(cpu.read_word(0x0017) == 0xd010);
}

TEST_CASE_METHOD(cpu_test, "LDA-IMM")
{
    nes::cpu cpu(mem);
         
    load(prgadr, std::array{0xa9, 0x55}); // LDA #$55
    cpu.tick();

    CHECK(cpu.a == 0x55);
}

TEST_CASE_METHOD(cpu_test, "Unsupported opcode")
{
    nes::cpu cpu(mem);

    load(prgadr, std::array{0x02});
    CHECK_THROWS_AS(cpu.tick(), std::runtime_error);
}

TEST_CASE_METHOD(cpu_test, "LDA-ZP")
{
    nes::cpu cpu(mem);

    load(0x0010, std::array{0x42});
    load(prgadr, std::array{0xa5, 0x10}); // LDA $10
    cpu.tick();

    CHECK(cpu.a == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDA-ZPX")
{
    nes::cpu cpu(mem);

    cpu.x = 0x02;
    load(0x0012, std::array{0x89});
    load(prgadr, std::array{0xb5, 0x10}); // LDA $10,X
    cpu.tick();

    CHECK(cpu.a == 0x89);
}

TEST_CASE_METHOD(cpu_test, "LDA-ABS")
{
    nes::cpu cpu(mem);

    load(prgadr, std::array{ 0xad, 0x10, 0xd0 }); // LDA $D010
    load(0xd010, std::array{ 0x42 });

    cpu.tick();

    CHECK(cpu.a == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDA-ABX")
{
    nes::cpu cpu(mem);

    load(prgadr, std::array{ 0xbd, 0x0A, 0xd0 }); // LDA $D00A,X

    cpu.x = 0x06;
    load(0xd010, std::array{ 0x42 });

    cpu.tick();

    CHECK(cpu.a == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDA-ABY")
{
    nes::cpu cpu(mem);

    load(prgadr, std::array{ 0xb9, 0x0B, 0xd0 }); // LDA $D00B,X

    cpu.y = 0x05;
    load(0xd010, std::array{ 0x42 });

    cpu.tick();

    CHECK(cpu.a == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDA-IZX")
{
    nes::cpu cpu(mem);

    cpu.x = 0x02;
    load(0x0017, std::array{0x10, 0xd0}); // $D010
    load(0xd010, std::array{0x0F});
    load(prgadr, std::array{0xa1, 0x15}); // LDA ($15,X)
    cpu.tick();

    CHECK(cpu.a == 0x0F);
}

TEST_CASE_METHOD(cpu_test, "LDA-IZY")
{
    nes::cpu cpu(mem);

    cpu.y = 0x03;
    load(0x002A, std::array{0x35, 0xc2}); // $C235
    load(0xc238, std::array{0x2F});
    load(prgadr, std::array{0xb1, 0x2a}); // LDA ($2A),Y
    cpu.tick();

    CHECK(cpu.a == 0x2F);
}

TEST_CASE_METHOD(cpu_test, "LDX-IMM")
{
    nes::cpu cpu(mem);

    load(prgadr, std::array{0xa2, 0x42}); // LDX #$42
    cpu.tick();

    CHECK(cpu.x == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDX-ZP")
{
    nes::cpu cpu(mem);

    load(0x0010, std::array{0x88});
    load(prgadr, std::array{0xa6, 0x10}); // LDX $10
    cpu.tick();

    CHECK(cpu.x == 0x88);
}

TEST_CASE_METHOD(cpu_test, "LDX-ZPY")
{
    nes::cpu cpu(mem);

    cpu.y = 0x03;
    load(0x0013, std::array{0x77});
    load(prgadr, std::array{0xb6, 0x10}); // LDX $10,Y
    cpu.tick();

    CHECK(cpu.x == 0x77);
}