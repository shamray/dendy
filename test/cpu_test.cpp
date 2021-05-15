#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "libnes/cpu.h"

#include <array>

constexpr auto operator""_Kb(size_t const x) 
{
    return x * 1024;
}

auto create_memory()
{
    auto mem = std::vector<uint8_t>(64_Kb, 0);
    std::ranges::copy(std::array{0x00, 0x80}, mem.begin() + 0xfffc);
    return mem;
}

class cpu_test
{
public:
    cpu_test()
        : mem{create_memory()}
        , cpu{mem}
    {
    }

    void load(uint16_t addr, auto program)
    {
        std::ranges::copy(program, mem.begin() + addr);
    }

    auto tick(int count, bool expected_to_finish = true)
    {
        for (auto i = 0; i < count; ++i) {
            cpu.tick();
        }
        CHECK(cpu.is_executing() != expected_to_finish);
    }

    std::vector<uint8_t> mem;
    nes::cpu cpu;
    uint16_t prgadr{0x8000};

};

TEST_CASE_METHOD(cpu_test, "Power Up")
{
    CHECK(cpu.read_word(0xfffc) == cpu.pc.value());
    CHECK(cpu.s == 0xfd);
}

TEST_CASE_METHOD(cpu_test, "read_word")
{
    load(0x0017, std::array{0x10, 0xd0}); // $D010

    CHECK(cpu.read_word(0x0017) == 0xd010);
}

TEST_CASE_METHOD(cpu_test, "LDA-IMM")
{
    load(prgadr, std::array{0xa9, 0x55}); // LDA #$55
    tick(2);

    CHECK(cpu.a.value() == 0x55);
}

TEST_CASE_METHOD(cpu_test, "LDA-Flags")
{
    SECTION("Zero")
    {
        load(prgadr, std::array{0xa9, 0x00}); // LDA #$00
        tick(2);

        CHECK(cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
    }

    SECTION("Negative")
    {
        load(prgadr, std::array{0xa9, 0xFF}); // LDA #$FF
        tick(2);

        CHECK(cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
    }
}

TEST_CASE_METHOD(cpu_test, "Unsupported opcode")
{
    load(prgadr, std::array{0x02});
    CHECK_THROWS_AS(cpu.tick(), std::runtime_error);
}

TEST_CASE_METHOD(cpu_test, "LDA-ZP")
{
    load(prgadr, std::array{0xa5, 0x10}); // LDA $10
    load(0x0010, std::array{0x42});

    tick(3);

    CHECK(cpu.a.value() == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDA-ZPX")
{
    load(prgadr, std::array{0xb5, 0x10}); // LDA $10,X

    cpu.x = 0x02;
    load(0x0012, std::array{0x89});

    tick(4);

    CHECK(cpu.a.value() == 0x89);
}

TEST_CASE_METHOD(cpu_test, "LDA-ABS")
{
    load(prgadr, std::array{0xad, 0x10, 0xd0}); // LDA $D010
    load(0xd010, std::array{0x42});

    tick(4);

    CHECK(cpu.a.value() == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDA-ABX")
{
    load(prgadr, std::array{0xbd, 0x0A, 0xd0}); // LDA $D00A,X

    load(0xd010, std::array{0x42});
    load(0xd109, std::array{0x43});

    SECTION("No Page Cross")
    {
        cpu.x = 0x06;
        tick(4);

        CHECK(cpu.a.value() == 0x42);
    }
    SECTION("Page Cross")
    {
        cpu.x = 0xFF;
        tick(4, false);
        tick(1);

        CHECK(cpu.a.value() == 0x43);
    }
}

TEST_CASE_METHOD(cpu_test, "LDA-ABY")
{
    load(prgadr, std::array{0xb9, 0x0B, 0xd0}); // LDA $D00B,Y

    load(0xd010, std::array{0x42});
    load(0xd109, std::array{0x43});

    SECTION("No Page Cross")
    {
        cpu.y = 0x05;
        tick(4);

        CHECK(cpu.a.value() == 0x42);
    }
    SECTION("Page Cross")
    {
        cpu.y = 0xFE;
        tick(4, false);
        tick(1);

        CHECK(cpu.a.value() == 0x43);
    }
}

TEST_CASE_METHOD(cpu_test, "LDA-IZX")
{
    load(prgadr, std::array{0xa1, 0x15}); // LDA ($15,X)

    cpu.x = 0x02;
    load(0x0017, std::array{0x10, 0xd0}); // $D010
    load(0xd010, std::array{0x0F});

    tick(6);

    CHECK(cpu.a.value() == 0x0F);
}

TEST_CASE_METHOD(cpu_test, "LDA-IZY")
{
    load(prgadr, std::array{0xb1, 0x2a}); // LDA ($2A),Y

    load(0x002A, std::array{0x35, 0xc2}); // $C235
    load(0xc238, std::array{0x2F});
    load(0xc300, std::array{0x21});

    SECTION("No Page Cross")
    {
        cpu.y = 0x03;
        tick(5);

        CHECK(cpu.a.value() == 0x2F);
    }
    SECTION("Page Cross")
    {
        cpu.y = 0xCB;
        tick(5, false);
        tick(1);

        CHECK(cpu.a.value() == 0x21);
    }
}

TEST_CASE_METHOD(cpu_test, "LDX-IMM")
{
    load(prgadr, std::array{0xa2, 0x42}); // LDX #$42
    tick(2);

    CHECK(cpu.x.value() == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDX-ZP")
{
    load(prgadr, std::array{0xa6, 0x10}); // LDX $10
    load(0x0010, std::array{0x88});

    tick(3);

    CHECK(cpu.x.value() == 0x88);
}

TEST_CASE_METHOD(cpu_test, "LDX-ZPY")
{
    load(prgadr, std::array{0xb6, 0x10}); // LDX $10,Y
	cpu.y = 0x03;
    load(0x0013, std::array{0x77});
    tick(4);

    CHECK(cpu.x.value() == 0x77);
}

TEST_CASE_METHOD(cpu_test, "LDA-ZPX-Overflow")
{
    cpu.x = 0x01;
    load(0x0000, std::array{0x77});
    load(prgadr, std::array{0xb5, 0xff}); // LDX $10,Y
    tick(4);

    CHECK(cpu.a.value() == 0x77);
}

TEST_CASE_METHOD(cpu_test, "LDX-ABS")
{
    load(prgadr, std::array{0xae, 0x10, 0xd0}); // LDX $D010
    load(0xd010, std::array{0x42});

    tick(4);

    CHECK(cpu.x.value() == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDX-ABY")
{
    load(prgadr, std::array{0xbe, 0x0B, 0xd0}); // LDX $D00B,Y

    cpu.y = 0x05;
    load(0xd010, std::array{0x42});

    tick(4);

    CHECK(cpu.x.value() == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDX-Flags")
{
    SECTION("Zero")
    {
        load(prgadr, std::array{0xa2, 0x00}); // LDX #$00
        tick(2);

        CHECK(cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
    }

    SECTION("Negative")
    {
        load(prgadr, std::array{0xa2, 0xFF}); // LDX #$FF
        tick(2);

        CHECK(cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
    }
}

TEST_CASE_METHOD(cpu_test, "STA-ZP")
{
    load(prgadr, std::array{0x85, 0x10}); // STA $10
    cpu.a = 0x42;

    tick(3);

    CHECK(mem[0x0010] == 0x42);
}

TEST_CASE_METHOD(cpu_test, "STA-ABS")
{
    load(prgadr, std::array{0x8d, 0x77, 0xd0}); // STA $D077
    cpu.a = 0x55;

    tick(4);

    CHECK(mem[0xd077] == 0x55);
}

TEST_CASE_METHOD(cpu_test, "TAX")
{
    load(prgadr, std::array{0xaa}); // TAX
    cpu.a = 0xDA;
    cpu.x = 0x00;

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    tick(2);

    CHECK(cpu.x.value() == cpu.a.value());
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TXA")
{
    load(prgadr, std::array{0x8a}); // TXA
    cpu.x = 0xDA;
    cpu.a = 0x00;

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    tick(2);

    CHECK(cpu.a.value() == cpu.x.value());
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TAY")
{
    load(prgadr, std::array{0xa8}); // TAY
    cpu.a = 0xDA;
    cpu.y = 0x00;

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    tick(2);

    CHECK(cpu.y.value() == cpu.a.value());
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TYA")
{
    load(prgadr, std::array{0x98}); // TYA
    cpu.y = 0xDA;
    cpu.a = 0x00;

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    tick(2);

    CHECK(cpu.a.value() == cpu.y.value());
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TSX")
{
    load(prgadr, std::array{0xba}); // TSX
    cpu.s = 0xDA;
    cpu.x = 0x00;

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    tick(2);

    CHECK(cpu.x.value() == cpu.s);
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TXS")
{
    load(prgadr, std::array{0x9a}); // TXS
    cpu.s = 0x00;
    cpu.x = 0xDA;

    cpu.p.reset(nes::cpu_flag::negative);
    tick(2);

    CHECK(cpu.s == cpu.x.value());
    CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "PHA")
{
    load(prgadr, std::array{0x48}); // PHA
    cpu.a = 0x55;

    REQUIRE(cpu.s == 0xfd);

    tick(3);

    CHECK(cpu.s == 0xfc);
    CHECK(mem[0x01fd] == 0x55);
}

TEST_CASE_METHOD(cpu_test, "PLA")
{
    load(prgadr, std::array{0x68}); // PLA
    cpu.s = 0xfc;

    SECTION("positive")
    {
        mem[0x01fd] = 0x55;
        tick(4);

        CHECK(cpu.a.value() == 0x55);

        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
    }
    SECTION("zero")
    {
        mem[0x01fd] = 0x00;
        tick(4);

        CHECK(cpu.a.value() == 0x00);

        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
        CHECK      (cpu.p.test(nes::cpu_flag::zero));
    }
    SECTION("negative")
    {
        mem[0x01fd] = 0xA0;
        tick(4);

        CHECK(cpu.a.value() == 0xA0);

        CHECK      (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
    }
}

TEST_CASE_METHOD(cpu_test, "PHP")
{
    load(prgadr, std::array{0x08}); // PHP
    cpu.p.assign(0x42); // zero, overflow

    tick(3);

    CHECK(cpu.s == 0xfc);
    CHECK(mem[0x01fd] == 0x42);
}

TEST_CASE_METHOD(cpu_test, "PLP")
{
    load(prgadr, std::array{0x28}); // PLP
    load(0x01fd, std::array{0xDF}); // all flags
    cpu.s = 0xfc;

    REQUIRE(cpu.p.value() == 0x00);

    tick(4);

    CHECK(cpu.s == 0xfd);
    CHECK(cpu.p.test(nes::cpu_flag::carry));
    CHECK(cpu.p.test(nes::cpu_flag::zero));
    CHECK(cpu.p.test(nes::cpu_flag::int_disable));
    CHECK(cpu.p.test(nes::cpu_flag::decimal));
    CHECK(cpu.p.test(nes::cpu_flag::break_called));
    CHECK(cpu.p.test(nes::cpu_flag::overflow));
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "ADC-IMM")
{
    load(prgadr, std::array{0x69, 0x07});

    SECTION("A = A + M")
    {
        cpu.a = 0x04;
        tick(2);

        CHECK(cpu.a.value() == 0x0B); // 4 + 7 == 11
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }

    SECTION("A = A + M == 0")
    {
        cpu.a = 0xF9;
        tick(2);

        CHECK(cpu.a.value() == 0x00); // 7 + (-7) == 0
        CHECK(cpu.p.test(nes::cpu_flag::zero));
    }

    SECTION("A = A + M < 0")
    {
        cpu.a = 0xE3;
        tick(2);

        CHECK(cpu.a.value() == 0xEA); // 7 + (-29) == -22
        CHECK(cpu.p.test(nes::cpu_flag::negative));
    }

    SECTION("A = A + M + C")
    {
        cpu.a = 0x04;
        cpu.p.set(nes::cpu_flag::carry);
        tick(2);

        CHECK(cpu.a.value() == 0x0C); // 4 + 7 + 1== 12
    }

    SECTION("C,A = A + M")
    {
        cpu.a = 0xFF;
        tick(2);

        CHECK(cpu.a.value() == 0x06); // 255 + 7 == 6 + carry
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }

    SECTION("Overflow: [+] + [+] = [-]")
    {
        cpu.a = 0x7D;
        tick(2);

        CHECK(cpu.a.value() == 0x84); // 132 + 7 == "-84"
        CHECK(cpu.p.test(nes::cpu_flag::overflow));
    }

    SECTION("Overflow: [-] + [-] = [+]")
    {
        load(prgadr, std::array{0x69, 0xFE});
        cpu.a = 0x80;
        tick(2);

        CHECK(cpu.a.value() == 0x7E); // -2 + (-128) == "+7E"
        CHECK(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
}

TEST_CASE_METHOD(cpu_test, "ADC-ABX")
{
    load(prgadr, std::array{0x7d, 0x01, 0xc0}); // ADC $C001,X
    load(0xc003, std::array{0x5a});
    cpu.x = 0x02;
    cpu.a = 0x01;

    tick(4);
    CHECK(cpu.a.value() == 0x5b);
}

TEST_CASE_METHOD(cpu_test, "SBC")
{
    // STA $00,X
    // TYA
    // SBC $00,X
    load(prgadr, std::array{0x95, 0x00, 0x98, 0xf5, 0x00}); // A = Y - A
    int program_cycles = 4 + 2 + 4;

    SECTION("0x50-0xf0=0x60")
    {
        cpu.y = 0x50;
        cpu.a = 0xf0;

        tick(program_cycles);
        CHECK(cpu.a.value() == 0x60);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0x50-0xb0=0xa0")
    {
        cpu.y = 0x50;
        cpu.a = 0xb0;

        tick(program_cycles);
        CHECK(cpu.a.value() == 0xa0);
        CHECK(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0x50-0x70=0xe0")
    {
        cpu.y = 0x50;
        cpu.a = 0x70;

        tick(program_cycles);
        CHECK(cpu.a.value() == 0xe0);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0x50-0x30=0x20")
    {
        cpu.y = 0x50;
        cpu.a = 0x30;

        tick(program_cycles);
        CHECK(cpu.a.value() == 0x20);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0xf0=0xe0")
    {
        cpu.y = 0xd0;
        cpu.a = 0xf0;

        tick(program_cycles);
        CHECK(cpu.a.value() == 0xe0);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0xb0=0x20")
    {
        cpu.y = 0xd0;
        cpu.a = 0xb0;

        tick(program_cycles);
        CHECK(cpu.a.value() == 0x20);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0x70=0x60")
    {
        cpu.y = 0xd0;
        cpu.a = 0x70;

        tick(program_cycles);
        CHECK(cpu.a.value() == 0x60);
        CHECK(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0x30=0xa0")
    {
        cpu.y = 0xd0;
        cpu.a = 0x30;

        tick(program_cycles);
        CHECK(cpu.a.value() == 0xa0);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
}

TEST_CASE_METHOD(cpu_test, "CMP")
{
    load(prgadr, std::array{0xc9, 0x2A});
    
    SECTION("A < M")
    {
        cpu.a = 0x29;
        tick(2);

        CHECK       (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("A = M")
    {
        cpu.a = 0x2A;
        tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::negative));
        CHECK       (cpu.p.test(nes::cpu_flag::zero));
        CHECK       (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("A > M")
    {
        cpu.a = 0x2B;
        tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::zero));
        CHECK       (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("V = 0")
    {
        load(prgadr, std::array{0xc9, 0xd0});

        cpu.a = 0x70;
        tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
}