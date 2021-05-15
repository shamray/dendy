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
        load(0xfffc, std::array{0x00, 0x80});
    }

    void load(uint16_t addr, auto program)
    {
        std::ranges::copy(program, mem.begin() + addr);
    }

    std::vector<uint8_t> mem;
    uint16_t prgadr{0x8000};

};

TEST_CASE_METHOD(cpu_test, "Power Up")
{
    nes::cpu cpu{mem};

    CHECK(cpu.read_word(0xfffc) == cpu.pc);
    CHECK(cpu.s == 0xfd);
}

TEST_CASE_METHOD(cpu_test, "read_word")
{
    nes::cpu cpu{mem};

    load(0x0017, std::array{0x10, 0xd0}); // $D010

    CHECK(cpu.read_word(0x0017) == 0xd010);
}

TEST_CASE_METHOD(cpu_test, "LDA-IMM")
{
    nes::cpu cpu{mem};
         
    load(prgadr, std::array{0xa9, 0x55}); // LDA #$55
    cpu.tick(2);

    CHECK(cpu.a == 0x55);
}

TEST_CASE_METHOD(cpu_test, "LDA-Flags")
{
    nes::cpu cpu{mem};

    SECTION("Zero")
    {
        load(prgadr, std::array{0xa9, 0x00}); // LDA #$00
        cpu.tick(2);

        CHECK(cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
    }

    SECTION("Negative")
    {
        load(prgadr, std::array{0xa9, 0xFF}); // LDA #$FF
        cpu.tick(2);

        CHECK(cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
    }
}

TEST_CASE_METHOD(cpu_test, "Unsupported opcode")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0x02});
    CHECK_THROWS_AS(cpu.tick(), std::runtime_error);
}

TEST_CASE_METHOD(cpu_test, "LDA-ZP")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xa5, 0x10}); // LDA $10
    load(0x0010, std::array{0x42});

    cpu.tick(3);

    CHECK(cpu.a == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDA-ZPX")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xb5, 0x10}); // LDA $10,X

    cpu.x = 0x02;
    load(0x0012, std::array{0x89});

    cpu.tick(4);

    CHECK(cpu.a == 0x89);
}

TEST_CASE_METHOD(cpu_test, "LDA-ABS")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xad, 0x10, 0xd0}); // LDA $D010
    load(0xd010, std::array{0x42});

    cpu.tick(4);

    CHECK(cpu.a == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDA-ABX")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xbd, 0x0A, 0xd0}); // LDA $D00A,X

    cpu.x = 0x06;
    load(0xd010, std::array{0x42});

    cpu.tick(4);

    CHECK(cpu.a == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDA-ABY")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xb9, 0x0B, 0xd0}); // LDA $D00B,Y

    cpu.y = 0x05;
    load(0xd010, std::array{0x42});

    cpu.tick(4);

    CHECK(cpu.a == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDA-IZX")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xa1, 0x15}); // LDA ($15,X)

    cpu.x = 0x02;
    load(0x0017, std::array{0x10, 0xd0}); // $D010
    load(0xd010, std::array{0x0F});

    cpu.tick(6);

    CHECK(cpu.a == 0x0F);
}

TEST_CASE_METHOD(cpu_test, "LDA-IZY")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xb1, 0x2a}); // LDA ($2A),Y

    cpu.y = 0x03;
    load(0x002A, std::array{0x35, 0xc2}); // $C235
    load(0xc238, std::array{0x2F});

    cpu.tick(5);

    CHECK(cpu.a == 0x2F);
}

TEST_CASE_METHOD(cpu_test, "LDX-IMM")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xa2, 0x42}); // LDX #$42
    cpu.tick(2);

    CHECK(cpu.x == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDX-ZP")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xa6, 0x10}); // LDX $10
    load(0x0010, std::array{0x88});

    cpu.tick(3);

    CHECK(cpu.x == 0x88);
}

TEST_CASE_METHOD(cpu_test, "LDX-ZPY")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xb6, 0x10}); // LDX $10,Y
	cpu.y = 0x03;
    load(0x0013, std::array{0x77});
    cpu.tick(4);

    CHECK(cpu.x == 0x77);
}

TEST_CASE_METHOD(cpu_test, "LDA-ZPX-Overflow")
{
    nes::cpu cpu{mem};

    cpu.x = 0x01;
    load(0x0000, std::array{0x77});
    load(prgadr, std::array{0xb5, 0xff}); // LDX $10,Y
    cpu.tick(4);

    CHECK(cpu.a == 0x77);
}

TEST_CASE_METHOD(cpu_test, "LDX-ABS")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xae, 0x10, 0xd0}); // LDX $D010
    load(0xd010, std::array{0x42});

    cpu.tick(4);

    CHECK(cpu.x == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDX-ABY")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xbe, 0x0B, 0xd0}); // LDX $D00B,Y

    cpu.y = 0x05;
    load(0xd010, std::array{0x42});

    cpu.tick(4);

    CHECK(cpu.x == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDX-Flags")
{
    nes::cpu cpu{mem};

    SECTION("Zero")
    {
        load(prgadr, std::array{0xa2, 0x00}); // LDX #$00
        cpu.tick(2);

        CHECK(cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
    }

    SECTION("Negative")
    {
        load(prgadr, std::array{0xa2, 0xFF}); // LDX #$FF
        cpu.tick(2);

        CHECK(cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
    }
}

TEST_CASE_METHOD(cpu_test, "STA-ZP")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0x85, 0x10}); // STA $10
    cpu.a = 0x42;

    cpu.tick(3);

    CHECK(mem[0x0010] == 0x42);
}

TEST_CASE_METHOD(cpu_test, "STA-ABS")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0x8d, 0x77, 0xd0}); // STA $D077
    cpu.a = 0x55;

    cpu.tick(4);

    CHECK(mem[0xd077] == 0x55);
}

TEST_CASE_METHOD(cpu_test, "TAX")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xaa}); // TAX
    cpu.a = 0xDA;
    cpu.x = 0x00;

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    cpu.tick(2);

    CHECK(cpu.x == cpu.a);
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TXA")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0x8a}); // TXA
    cpu.x = 0xDA;
    cpu.a = 0x00;

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    cpu.tick(2);

    CHECK(cpu.a == cpu.x);
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TAY")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xa8}); // TAY
    cpu.a = 0xDA;
    cpu.y = 0x00;

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    cpu.tick(2);

    CHECK(cpu.y == cpu.a);
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TYA")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0x98}); // TYA
    cpu.y = 0xDA;
    cpu.a = 0x00;

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    cpu.tick(2);

    CHECK(cpu.a == cpu.y);
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TSX")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xba}); // TSX
    cpu.s = 0xDA;
    cpu.x = 0x00;

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    cpu.tick(2);

    CHECK(cpu.x == cpu.s);
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TXS")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0x9a}); // TXS
    cpu.s = 0x00;
    cpu.x = 0xDA;

    cpu.p.reset(nes::cpu_flag::negative);
    cpu.tick(2);

    CHECK(cpu.s == cpu.x);
    CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "PHA")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0x48}); // PHA
    cpu.a = 0x55;

    REQUIRE(cpu.s == 0xfd);

    cpu.tick(3);

    CHECK(cpu.s == 0xfc);
    CHECK(mem[0x01fd] == 0x55);
}

TEST_CASE_METHOD(cpu_test, "PLA")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0x68}); // PLA
    cpu.s = 0xfc;

    SECTION("positive")
    {
        mem[0x01fd] = 0x55;
        cpu.tick(4);

        CHECK(cpu.a == 0x55);

        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
    }
    SECTION("zero")
    {
        mem[0x01fd] = 0x00;
        cpu.tick(4);

        CHECK(cpu.a == 0x00);

        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
        CHECK      (cpu.p.test(nes::cpu_flag::zero));
    }
    SECTION("negative")
    {
        mem[0x01fd] = 0xA0;
        cpu.tick(4);

        CHECK(cpu.a == 0xA0);

        CHECK      (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
    }
}

TEST_CASE_METHOD(cpu_test, "PHP")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0x08}); // PHP
    cpu.p.assign(0x42); // zero, overflow

    cpu.tick(3);

    CHECK(cpu.s == 0xfc);
    CHECK(mem[0x01fd] == 0x42);
}

TEST_CASE_METHOD(cpu_test, "PLP")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0x28}); // PLP
    load(0x01fd, std::array{0xDF}); // all flags
    cpu.s = 0xfc;

    REQUIRE(cpu.p.value() == 0x00);

    cpu.tick(4);

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
    nes::cpu cpu{mem};
    load(prgadr, std::array{0x69, 0x07});

    SECTION("A = A + M")
    {
        cpu.a = 0x04;
        cpu.tick(2);

        CHECK(cpu.a == 0x0B); // 4 + 7 == 11
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }

    SECTION("A = A + M == 0")
    {
        cpu.a = 0xF9;
        cpu.tick(2);

        CHECK(cpu.a == 0x00); // 7 + (-7) == 0
        CHECK(cpu.p.test(nes::cpu_flag::zero));
    }

    SECTION("A = A + M < 0")
    {
        cpu.a = 0xE3;
        cpu.tick(2);

        CHECK(cpu.a == 0xEA); // 7 + (-29) == -22
        CHECK(cpu.p.test(nes::cpu_flag::negative));
    }

    SECTION("A = A + M + C")
    {
        cpu.a = 0x04;
        cpu.p.set(nes::cpu_flag::carry);
        cpu.tick(2);

        CHECK(cpu.a == 0x0C); // 4 + 7 + 1== 12
    }

    SECTION("C,A = A + M")
    {
        cpu.a = 0xFF;
        cpu.tick(2);

        CHECK(cpu.a == 0x06); // 255 + 7 == 6 + carry
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }

    SECTION("Overflow: [+] + [+] = [-]")
    {
        cpu.a = 0x7D;
        cpu.tick(2);

        CHECK(cpu.a == 0x84); // 132 + 7 == "-84"
        CHECK(cpu.p.test(nes::cpu_flag::overflow));
    }

    SECTION("Overflow: [-] + [-] = [+]")
    {
        load(prgadr, std::array{0x69, 0xFE});
        cpu.a = 0x80;
        cpu.tick(2);

        CHECK(cpu.a == 0x7E); // -2 + (-128) == "+7E"
        CHECK(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
}

TEST_CASE_METHOD(cpu_test, "ADC-ABX")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0x7d, 0x01, 0xc0}); // ADC $C001,X
    load(0xc003, std::array{0x5a});
    cpu.x = 0x02;
    cpu.a = 0x01;

    cpu.tick(4);
    CHECK(cpu.a == 0x5b);
}

TEST_CASE_METHOD(cpu_test, "SBC")
{
    nes::cpu cpu{mem};

    // STA $00,X
    // TYA
    // SBC $00,X
    load(prgadr, std::array{0x95, 0x00, 0x98, 0xf5, 0x00}); // A = Y - A
    int program_cycles = 4 + 2 + 4;

    SECTION("0x50-0xf0=0x60")
    {
        cpu.y = 0x50;
        cpu.a = 0xf0;

        cpu.tick(program_cycles);
        CHECK(cpu.a == 0x60);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0x50-0xb0=0xa0")
    {
        cpu.y = 0x50;
        cpu.a = 0xb0;

        cpu.tick(program_cycles);
        CHECK(cpu.a == 0xa0);
        CHECK(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0x50-0x70=0xe0")
    {
        cpu.y = 0x50;
        cpu.a = 0x70;

        cpu.tick(program_cycles);
        CHECK(cpu.a == 0xe0);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0x50-0x30=0x20")
    {
        cpu.y = 0x50;
        cpu.a = 0x30;

        cpu.tick(program_cycles);
        CHECK(cpu.a == 0x20);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0xf0=0xe0")
    {
        cpu.y = 0xd0;
        cpu.a = 0xf0;

        cpu.tick(program_cycles);
        CHECK(cpu.a == 0xe0);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0xb0=0x20")
    {
        cpu.y = 0xd0;
        cpu.a = 0xb0;

        cpu.tick(program_cycles);
        CHECK(cpu.a == 0x20);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0x70=0x60")
    {
        cpu.y = 0xd0;
        cpu.a = 0x70;

        cpu.tick(program_cycles);
        CHECK(cpu.a == 0x60);
        CHECK(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0x30=0xa0")
    {
        cpu.y = 0xd0;
        cpu.a = 0x30;

        cpu.tick(program_cycles);
        CHECK(cpu.a == 0xa0);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
}

TEST_CASE_METHOD(cpu_test, "CMP")
{
    nes::cpu cpu{mem};

    load(prgadr, std::array{0xc9, 0x2A});
    
    SECTION("A < M")
    {
        cpu.a = 0x29;
        cpu.tick(2);

        CHECK       (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("A = M")
    {
        cpu.a = 0x2A;
        cpu.tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::negative));
        CHECK       (cpu.p.test(nes::cpu_flag::zero));
        CHECK       (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("A > M")
    {
        cpu.a = 0x2B;
        cpu.tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::zero));
        CHECK       (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("V = 0")
    {
        load(prgadr, std::array{0xc9, 0xd0});

        cpu.a = 0x70;
        cpu.tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
}