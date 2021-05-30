#include <catch2/catch.hpp>

#include "libnes/cpu.hpp"
#include "libnes/literals.hpp"

#include <array>

using namespace nes::literals;

auto create_memory()
{
    auto mem = std::vector<uint8_t>(64_Kb, 0);
    std::ranges::copy(std::array{0x00, 0x80}, mem.begin() + 0xfffc);
    return mem;
}

class cpu_test
{
public:
    struct test_bus
    {
        void write(uint16_t addr, uint8_t value) { mem[addr] = value; }
        uint8_t read(uint16_t addr) const { return mem[addr]; }

        std::vector<uint8_t>& mem;
    };

    cpu_test()
        : mem{create_memory()}
        , cpu{b}
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
    test_bus b{mem};
    nes::cpu<test_bus> cpu;
    uint16_t prgadr{0x8000};

};

TEST_CASE_METHOD(cpu_test, "Power Up")
{
    CHECK(cpu.read_word(0xfffc) == cpu.pc.value());
    CHECK(cpu.s.value() == 0xfd);
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
        load(prgadr, std::array{0xa9, 0xff}); // LDA #$FF
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

    cpu.x.assign(0x02);
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
        cpu.x.assign(0x06);
        tick(4);

        CHECK(cpu.a.value() == 0x42);
    }
    SECTION("Page Cross")
    {
        cpu.x.assign(0xff);
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
        cpu.y.assign(0x05);
        tick(4);

        CHECK(cpu.a.value() == 0x42);
    }
    SECTION("Page Cross")
    {
        cpu.y.assign(0xFE);
        tick(4, false);
        tick(1);

        CHECK(cpu.a.value() == 0x43);
    }
}

TEST_CASE_METHOD(cpu_test, "LDA-IZX")
{
    load(prgadr, std::array{0xa1, 0x15}); // LDA ($15,X)

    cpu.x.assign(0x02);
    load(0x0017, std::array{0x10, 0xd0}); // $D010
    load(0xd010, std::array{0x0f});

    tick(6);

    CHECK(cpu.a.value() == 0x0f);
}

TEST_CASE_METHOD(cpu_test, "LDA-IZX, X > 128")
{
    load(prgadr, std::array{0xa1, 0x15}); // LDA ($15,X)

    cpu.x.assign(0xc2);
    load(0x00d7, std::array{0x10, 0xd0}); // $D010
    load(0xd010, std::array{0x0f});

    tick(6);

    CHECK(cpu.a.value() == 0x0f);
}

TEST_CASE_METHOD(cpu_test, "LDA-IZX, Overflows")
{
    load(prgadr, std::array{0xa1, 0xff}); // LDA ($FF,X)

    SECTION("Address on Zero Page Border")
    {
        load(0x00ff, std::array{0x00});
        load(0x0000, std::array{0x04});
        cpu.x.assign(0x00);

        load(0x0400, std::array{0x5d});

        tick(6);

        CHECK(cpu.a.value() == 0x5d);
    }
    SECTION("LDA-IZX, Page Wrap")
    {
        load(0x0080, std::array{0x00, 0x02});
        cpu.x.assign(0x81);

        load(0x0200, std::array{0x5a});

        tick(6);

        CHECK(cpu.a.value() == 0x5a);
    }
}

TEST_CASE_METHOD(cpu_test, "LDA-IZY")
{
    load(prgadr, std::array{0xb1, 0x2a}); // LDA ($2A),Y

    load(0x002A, std::array{0x35, 0xc2}); // $C235
    load(0xc238, std::array{0x2F});
    load(0xc300, std::array{0x21});

    SECTION("No Page Cross")
    {
        cpu.y.assign(0x03);
        tick(5);

        CHECK(cpu.a.value() == 0x2F);
    }
    SECTION("Page Cross")
    {
        cpu.y.assign(0xCB);
        tick(5, false);
        tick(1);

        CHECK(cpu.a.value() == 0x21);
    }
}

TEST_CASE_METHOD(cpu_test, "LDA-IZY, Address on Zero Page Border")
{
    load(prgadr, std::array{0xb1, 0xff}); // LDA ($FF),Y

    load(0x00ff, std::array{0x46});
    load(0x0000, std::array{0x01});

    cpu.y.assign(0xff);

    load(0x0245, std::array{0x12});

    tick(6);

    CHECK(cpu.a.value() == 0x12);
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
	cpu.y.assign(0x03);
    load(0x0013, std::array{0x77});
    tick(4);

    CHECK(cpu.x.value() == 0x77);
}

TEST_CASE_METHOD(cpu_test, "LDA-ZPX-Overflow")
{
    cpu.x.assign(0x01);
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

    cpu.y.assign(0x05);
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
        load(prgadr, std::array{0xa2, 0xff}); // LDX #$FF
        tick(2);

        CHECK(cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
    }
}

TEST_CASE_METHOD(cpu_test, "STA-ZP")
{
    load(prgadr, std::array{0x85, 0x10}); // STA $10
    cpu.a.assign(0x42);

    tick(3);

    CHECK(mem[0x0010] == 0x42);
}

TEST_CASE_METHOD(cpu_test, "STA-ABS")
{
    load(prgadr, std::array{0x8d, 0x77, 0xd0}); // STA $D077
    cpu.a.assign(0x55);

    tick(4);

    CHECK(mem[0xd077] == 0x55);
}

TEST_CASE_METHOD(cpu_test, "STX")
{
    load(prgadr, std::array{0x86, 0x10}); // STX $10
    cpu.x.assign(0x42);

    tick(3);

    CHECK(mem[0x0010] == 0x42);
}

TEST_CASE_METHOD(cpu_test, "LDY-ABS")
{
    load(prgadr, std::array{0xac, 0x10, 0xd0}); // LDY $D010
    load(0xd010, std::array{0xba});

    tick(4);

    CHECK(cpu.y.value() == 0xba);
}

TEST_CASE_METHOD(cpu_test, "LDY-IMM")
{
    load(prgadr, std::array{0xa0, 0x40}); // LDY $40

    tick(2);
    CHECK(cpu.y.value() == 0x40);
}

TEST_CASE_METHOD(cpu_test, "STY-ABS")
{
    load(prgadr, std::array{0x8c, 0x77, 0xd0}); // STY $D077
    cpu.y.assign(0xba);

    tick(4);

    CHECK(mem[0xd077] == 0xba);
}

TEST_CASE_METHOD(cpu_test, "STY-ZP")
{
    load(prgadr, std::array{0x84, 0x78}); // STY $78
    cpu.y.assign(0x46);

    tick(3);

    CHECK(mem[0x0078] == 0x46);
}

TEST_CASE_METHOD(cpu_test, "TAX")
{
    load(prgadr, std::array{0xaa}); // TAX
    cpu.a.assign(0xdA);
    cpu.x.assign(0x00);

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    tick(2);

    CHECK(cpu.x.value() == cpu.a.value());
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TXA")
{
    load(prgadr, std::array{0x8a}); // TXA
    cpu.x.assign(0xdA);
    cpu.a.assign(0x00);

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    tick(2);

    CHECK(cpu.a.value() == cpu.x.value());
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TAY")
{
    load(prgadr, std::array{0xa8}); // TAY
    cpu.a.assign(0xdA);
    cpu.y.assign(0x00);

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    tick(2);

    CHECK(cpu.y.value() == cpu.a.value());
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TYA")
{
    load(prgadr, std::array{0x98}); // TYA
    cpu.y.assign(0xdA);
    cpu.a.assign(0x00);

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    tick(2);

    CHECK(cpu.a.value() == cpu.y.value());
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TSX")
{
    load(prgadr, std::array{0xba}); // TSX
    cpu.s.assign(0xdA);
    cpu.x.assign(0x00);

    REQUIRE_FALSE(cpu.p.test(nes::cpu_flag::negative));
    tick(2);

    CHECK(cpu.x.value() == cpu.s.value());
    CHECK(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "TXS")
{
    load(prgadr, std::array{0x9a}); // TXS
    cpu.s.assign(0x00);
    cpu.x.assign(0xdA);

    cpu.p.reset(nes::cpu_flag::negative);
    tick(2);

    CHECK(cpu.s.value() == cpu.x.value());
    CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
}

TEST_CASE_METHOD(cpu_test, "PHA")
{
    load(prgadr, std::array{0x48}); // PHA
    cpu.a.assign(0x55);

    REQUIRE(cpu.s.value() == 0xfd);

    tick(3);

    CHECK(cpu.s.value() == 0xfc);
    CHECK(mem[0x01fd] == 0x55);
}

TEST_CASE_METHOD(cpu_test, "PLA")
{
    load(prgadr, std::array{0x68}); // PLA
    cpu.s.assign(0xfc);

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
    cpu.p.assign(0x62); // zero, overflow

    tick(3);

    CHECK((int)cpu.s.value() == 0xfc);
    CHECK((int)mem[0x01fd] == 0x62);
}

TEST_CASE_METHOD(cpu_test, "PLP")
{
    load(prgadr, std::array{0x28}); // PLP
    load(0x01fd, std::array{0xDF}); // all flags
    cpu.s.assign(0xfc);

    REQUIRE((cpu.p.value() & 0xdf) == 0x00);

    tick(4);

    CHECK(cpu.s.value() == 0xfd);
    CHECK(cpu.p.test(nes::cpu_flag::carry));
    CHECK(cpu.p.test(nes::cpu_flag::zero));
    CHECK(cpu.p.test(nes::cpu_flag::int_disable));
    CHECK(cpu.p.test(nes::cpu_flag::decimal));
    CHECK(cpu.p.test(nes::cpu_flag::overflow));
    CHECK(cpu.p.test(nes::cpu_flag::negative));
    CHECK_FALSE(cpu.p.test(nes::cpu_flag::break_called));
}

TEST_CASE_METHOD(cpu_test, "ADC-IMM")
{
    load(prgadr, std::array{0x69, 0x07});

    SECTION("A = A + M")
    {
        cpu.a.assign(0x04);
        tick(2);

        CHECK(cpu.a.value() == 0x0B); // 4 + 7 == 11
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }

    SECTION("A = A + M == 0")
    {
        cpu.a.assign(0xF9);
        tick(2);

        CHECK(cpu.a.value() == 0x00); // 7 + (-7) == 0
        CHECK(cpu.p.test(nes::cpu_flag::zero));
    }

    SECTION("A = A + M < 0")
    {
        cpu.a.assign(0xE3);
        tick(2);

        CHECK(cpu.a.value() == 0xEA); // 7 + (-29) == -22
        CHECK(cpu.p.test(nes::cpu_flag::negative));
    }

    SECTION("A = A + M + C")
    {
        cpu.a.assign(0x04);
        cpu.p.set(nes::cpu_flag::carry);
        tick(2);

        CHECK(cpu.a.value() == 0x0C); // 4 + 7 + 1== 12
    }

    SECTION("C,A = A + M")
    {
        cpu.a.assign(0xff);
        tick(2);

        CHECK(cpu.a.value() == 0x06); // 255 + 7 == 6 + carry
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }

    SECTION("Overflow: [+] + [+] = [-]")
    {
        cpu.a.assign(0x7D);
        tick(2);

        CHECK(cpu.a.value() == 0x84); // 132 + 7 == "-84"
        CHECK(cpu.p.test(nes::cpu_flag::overflow));
    }

    SECTION("Overflow: [-] + [-] = [+]")
    {
        load(prgadr, std::array{0x69, 0xFE});
        cpu.a.assign(0x80);
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
    cpu.x.assign(0x02);
    cpu.a.assign(0x01);

    tick(4);
    CHECK(cpu.a.value() == 0x5b);
}
TEST_CASE_METHOD(cpu_test, "SBC, Borrow")
{
    load(prgadr, std::array{0xe9, 0x00}); // SBC #$00
    cpu.a.assign(0x80);
    cpu.p.assign(0xA4);

    tick(2);

    CHECK((int)cpu.a.value() == 0x7f);
    CHECK((int)cpu.p.value() == 0x65);
}

TEST_CASE_METHOD(cpu_test, "SBC")
{
    // SEC
    // STA $00,X
    // TYA
    // SBC $00,X
    load(prgadr, std::array{0x38, 0x95, 0x00, 0x98, 0xf5, 0x00}); // A = Y - A
    int program_cycles = 2 + 4 + 2 + 4;

    SECTION("0x50-0xf0=0x60")
    {
        cpu.y.assign(0x50);
        cpu.a.assign(0xf0);

        tick(program_cycles);
        CHECK(cpu.a.value() == 0x60);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0x50-0xb0=0xa0")
    {
        cpu.y.assign(0x50);
        cpu.a.assign(0xb0);

        tick(program_cycles);
        CHECK(cpu.a.value() == 0xa0);
        CHECK(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0x50-0x70=0xe0")
    {
        cpu.y.assign(0x50);
        cpu.a.assign(0x70);

        tick(program_cycles);
        CHECK(cpu.a.value() == 0xe0);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0x50-0x30=0x20")
    {
        cpu.y.assign(0x50);
        cpu.a.assign(0x30);

        tick(program_cycles);
        CHECK(cpu.a.value() == 0x20);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0xf0=0xe0")
    {
        cpu.y.assign(0xd0);
        cpu.a.assign(0xf0);

        tick(program_cycles);
        CHECK(cpu.a.value() == 0xe0);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0xb0=0x20")
    {
        cpu.y.assign(0xd0);
        cpu.a.assign(0xb0);

        tick(program_cycles);
        CHECK(cpu.a.value() == 0x20);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0x70=0x60")
    {
        cpu.y.assign(0xd0);
        cpu.a.assign(0x70);

        tick(program_cycles);
        CHECK(cpu.a.value() == 0x60);
        CHECK(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("0xd0-0x30=0xa0")
    {
        cpu.y.assign(0xd0);
        cpu.a.assign(0x30);

        tick(program_cycles);
        CHECK(cpu.a.value() == 0xa0);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
}

TEST_CASE_METHOD(cpu_test, "CMP")
{
    load(prgadr, std::array{0xc9, 0x2a});
    
    SECTION("A < M")
    {
        cpu.a.assign(0x29);
        tick(2);

        CHECK       (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("A = M")
    {
        cpu.a.assign(0x2A);
        tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::negative));
        CHECK       (cpu.p.test(nes::cpu_flag::zero));
        CHECK       (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("A > M")
    {
        cpu.a.assign(0x2B);
        tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::zero));
        CHECK       (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("V = 0")
    {
        load(prgadr, std::array{0xc9, 0xd0});

        cpu.a.assign(0x70);
        tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
}

TEST_CASE_METHOD(cpu_test, "CMP, Carry flag interference")
{
    load(prgadr, std::array{0xc9, 0x6f});
    cpu.a.assign(0x6f);
    cpu.p.set(nes::cpu_flag::carry);

    tick(2);

    CHECK(cpu.p.test(nes::cpu_flag::zero));
}

TEST_CASE_METHOD(cpu_test, "CPX")
{
    load(prgadr, std::array{0xe0, 0x2a});

    SECTION("X < M")
    {
        cpu.x.assign(0x29);
        tick(2);

        CHECK       (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("X = M")
    {
        cpu.x.assign(0x2A);
        tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::negative));
        CHECK       (cpu.p.test(nes::cpu_flag::zero));
        CHECK       (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("X > M")
    {
        cpu.x.assign(0x2B);
        tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::zero));
        CHECK       (cpu.p.test(nes::cpu_flag::carry));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
    SECTION("V = 0")
    {
        load(prgadr, std::array{0xc9, 0xd0});

        cpu.x.assign(0x70);
        tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
}

TEST_CASE_METHOD(cpu_test, "CPY")
{
    load(prgadr, std::array{0xc0, 0x40});
    cpu.p.assign(0x65);

    auto v = cpu.p.test(nes::cpu_flag::overflow);

    SECTION("Y < M")
    {
        cpu.y.assign(0x39);
        tick(2);

        CHECK       (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::zero));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::carry));
        CHECK       (cpu.p.test(nes::cpu_flag::overflow) == v);
    }
    SECTION("Y = M")
    {
        cpu.y.assign(0x40);
        tick(2);

        CHECK((int)cpu.p.value() == 0x67);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::negative));
        CHECK       (cpu.p.test(nes::cpu_flag::zero));
        CHECK       (cpu.p.test(nes::cpu_flag::carry));
        CHECK       (cpu.p.test(nes::cpu_flag::overflow) == v);
    }
    SECTION("Y > M")
    {
        cpu.y.assign(0x41);
        tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::negative));
        CHECK_FALSE (cpu.p.test(nes::cpu_flag::zero));
        CHECK       (cpu.p.test(nes::cpu_flag::carry));
        CHECK       (cpu.p.test(nes::cpu_flag::overflow) == v);
    }
    SECTION("V = 0")
    {
        cpu.p.reset(nes::cpu_flag::overflow);
        load(prgadr, std::array{0xc9, 0xd0});

        cpu.y.assign(0x70);
        tick(2);

        CHECK_FALSE (cpu.p.test(nes::cpu_flag::overflow));
    }
}

TEST_CASE_METHOD(cpu_test, "JMP-ABS")
{
    load(prgadr, std::array{0x4c, 0x34, 0x12});
    tick(3);
    CHECK(cpu.pc.value() == 0x1234);
}

TEST_CASE_METHOD(cpu_test, "JMP-IND")
{
    load(prgadr, std::array{0x6c, 0x34, 0x12});
    load(0x1234, std::array{0x78, 0x56});
    tick(5);
    CHECK(cpu.pc.value() == 0x5678);
}

TEST_CASE_METHOD(cpu_test, "JMP-IND (Page Crossing Bug)")
{
    load(prgadr, std::array{0x6c, 0xff, 0x11});
    load(0x11FF, std::array{0x34});
    load(0x1100, std::array{0x12});
    tick(5);
    CHECK(cpu.pc.value() == 0x1234);
}

TEST_CASE_METHOD(cpu_test, "JSR")
{
    load(prgadr, std::array{0x20, 0x00, 0xa0});
    tick(6);

    CHECK(cpu.pc.value() == 0xa000);

    CHECK(cpu.s.value() == 0xfb);
    CHECK(mem[0x1fd] == 0x80);
    CHECK(mem[0x1fc] == 0x02);
}

TEST_CASE_METHOD(cpu_test, "RTS")
{
    load(prgadr, std::array{0x60});
    load(0x1fd, std::array{0xb0});
    load(0x1fc, std::array{0x02});
    cpu.s.assign(0xfb);
    tick(6);

    CHECK(cpu.pc.value() == 0xb003);
    CHECK(cpu.s.value() == 0xfd);
}

TEST_CASE_METHOD(cpu_test, "JSR-RTS, New instruction on new page")
{
    load(0xc5fd, std::array{0x20, 0x00, 0xa0});
    load(0xa000, std::array{0x60});
    cpu.pc.assign(0xc5fd);
    tick(6);

    CHECK(mem[0x1fd] == 0xc5);
    CHECK(mem[0x1fc] == 0xff);

    tick(6);
    CHECK(cpu.pc.value() == 0xc600);

}

TEST_CASE_METHOD(cpu_test, "BPL, offset > 0")
{
    load(prgadr, std::array{0x10, 0x20}); // +32

    SECTION("Branch")
    {
        cpu.p.reset(nes::cpu_flag::negative);
        tick(3);
        CHECK(cpu.pc.value() == 0x8022);
    }
    SECTION("Else")
    {
        cpu.p.set(nes::cpu_flag::negative);
        tick(2);
        CHECK(cpu.pc.value() == 0x8002);
    }
}

TEST_CASE_METHOD(cpu_test, "BPL, offset < 0, page cross")
{
    load(prgadr, std::array{0x10, 0xce}); // -32

    cpu.p.reset(nes::cpu_flag::negative);
    tick(4);
    CHECK(cpu.pc.value() == 0x7fd0);
}

TEST_CASE_METHOD(cpu_test, "BMI")
{
    load(prgadr, std::array{0x30, 0x20});

    SECTION("Branch")
    {
        cpu.p.set(nes::cpu_flag::negative);
        tick(3);
        CHECK(cpu.pc.value() == 0x8022);
    }
    SECTION("Else")
    {
        cpu.p.reset(nes::cpu_flag::negative);
        tick(2);
        CHECK(cpu.pc.value() == 0x8002);
    }
}

TEST_CASE_METHOD(cpu_test, "BVC")
{
    load(prgadr, std::array{0x50, 0x20});

    SECTION("Branch")
    {
        cpu.p.reset(nes::cpu_flag::overflow);
        tick(3);
        CHECK(cpu.pc.value() == 0x8022);
    }
    SECTION("Else")
    {
        cpu.p.set(nes::cpu_flag::overflow);
        tick(2);
        CHECK(cpu.pc.value() == 0x8002);
    }
}

TEST_CASE_METHOD(cpu_test, "BVS")
{
    load(prgadr, std::array{0x70, 0x20});

    SECTION("Branch")
    {
        cpu.p.set(nes::cpu_flag::overflow);
        tick(3);
        CHECK(cpu.pc.value() == 0x8022);
    }
    SECTION("Else")
    {
        cpu.p.reset(nes::cpu_flag::overflow);
        tick(2);
        CHECK(cpu.pc.value() == 0x8002);
    }
}

TEST_CASE_METHOD(cpu_test, "BCC")
{
    load(prgadr, std::array{0x90, 0x20});

    SECTION("Branch")
    {
        cpu.p.reset(nes::cpu_flag::carry);
        tick(3);
        CHECK(cpu.pc.value() == 0x8022);
    }
    SECTION("Else")
    {
        cpu.p.set(nes::cpu_flag::carry);
        tick(2);
        CHECK(cpu.pc.value() == 0x8002);
    }
}

TEST_CASE_METHOD(cpu_test, "BCS")
{
    load(prgadr, std::array{0xb0, 0x20});

    SECTION("Branch")
    {
        cpu.p.set(nes::cpu_flag::carry);
        tick(3);
        CHECK(cpu.pc.value() == 0x8022);
    }
    SECTION("Else")
    {
        cpu.p.reset(nes::cpu_flag::carry);
        tick(2);
        CHECK(cpu.pc.value() == 0x8002);
    }
}

TEST_CASE_METHOD(cpu_test, "BNE")
{
    load(prgadr, std::array{0xd0, 0x20});

    SECTION("Branch")
    {
        cpu.p.reset(nes::cpu_flag::zero);
        tick(3);
        CHECK(cpu.pc.value() == 0x8022);
    }
    SECTION("Else")
    {
        cpu.p.set(nes::cpu_flag::zero);
        tick(2);
        CHECK(cpu.pc.value() == 0x8002);
    }
}

TEST_CASE_METHOD(cpu_test, "BEQ")
{
    load(prgadr, std::array{0xf0, 0x20});

    SECTION("Branch")
    {
        cpu.p.set(nes::cpu_flag::zero);
        tick(3);
        CHECK(cpu.pc.value() == 0x8022);
    }
    SECTION("Else")
    {
        cpu.p.reset(nes::cpu_flag::zero);
        tick(2);
        CHECK(cpu.pc.value() == 0x8002);
    }
}

TEST_CASE_METHOD(cpu_test, "AND")
{
    load(prgadr, std::array{0x29, 0x01}); // AND #01
    cpu.a.assign(0x02);

    tick(2);
    CHECK(cpu.a.value() == 0x00);
    CHECK(cpu.p.test(nes::cpu_flag::zero));
}

TEST_CASE_METHOD(cpu_test, "ORA")
{
    load(prgadr, std::array{0x09, 0x01}); // ORA #01
    cpu.a.assign(0x02);

    tick(2);
    CHECK(cpu.a.value() == 0x03);
}

TEST_CASE_METHOD(cpu_test, "EOR")
{
    load(prgadr, std::array{0x49, 0x03}); // EOR #03
    cpu.a.assign(0x01);

    tick(2);
    CHECK(cpu.a.value() == 0x02);
}

TEST_CASE_METHOD(cpu_test, "BIT")
{
    load(prgadr, std::array{0x24, 0x00}); // BIT $00

    SECTION("Bit 0, Mask match")
    {
        load(0x0000, std::array{0x01});
        cpu.a.assign(0x0F);

        tick(3);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::zero));
    }
    SECTION("Bit 0, Mask mismatch")
    {
        load(0x0000, std::array{0x01});
        cpu.a.assign(0x0E);

        tick(3);
        CHECK(cpu.p.test(nes::cpu_flag::zero));
    }
    SECTION("Bit 6 = 1")
    {
        load(0x0000, std::array{0x40});
        cpu.a.assign(0x0E);

        tick(3);

        CHECK      (cpu.p.test(nes::cpu_flag::overflow));
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::negative));
    }
    SECTION("Bit 7 = 1")
    {
        load(0x0000, std::array{0x80});
        cpu.a.assign(0x0E);

        tick(3);

        CHECK_FALSE(cpu.p.test(nes::cpu_flag::overflow));
        CHECK      (cpu.p.test(nes::cpu_flag::negative));
    }
}

TEST_CASE_METHOD(cpu_test, "INC")
{
    load(prgadr, std::array{0xfe, 0x80, 0xf0}); // INC $F080,X
    load(0xf081, std::array{0x33});
    load(0xf120, std::array{0x75});

    SECTION("Page Cross")
    {
        cpu.x.assign(0x01);
        tick(7);
        CHECK(mem[0xf081] == 0x34);
    }
    SECTION("No Page Cross")
    {
        cpu.x.assign(0xa0);
        tick(7);
        CHECK(mem[0xf120] == 0x76);
    }
}

TEST_CASE_METHOD(cpu_test, "INC, Value Wrap")
{
    load(prgadr, std::array{0xe6, 0x78}); // INC $78
    load(0x0078, std::array{0xff});

    tick(5);

    CHECK(mem[0x0078] == 0x00);
    CHECK(cpu.p.test(nes::cpu_flag::zero));
}


TEST_CASE_METHOD(cpu_test, "DEC")
{
    load(prgadr, std::array{0xde, 0x80, 0xf0}); // DEC $F080,X
    load(0xf081, std::array{0x33});
    load(0xf120, std::array{0x75});

    SECTION("Page Cross")
    {
        cpu.x.assign(0x01);
        tick(7);
        CHECK(mem[0xf081] == 0x32);
    }
    SECTION("No Page Cross")
    {
        cpu.x.assign(0xa0);
        tick(7);
        CHECK(mem[0xf120] == 0x74);
    }
}

TEST_CASE_METHOD(cpu_test, "INX")
{
    load(prgadr, std::array{0xe8}); // INX
    cpu.x.assign(0x42);

    tick(2);
    CHECK((int)cpu.x.value() == 0x43);
}

TEST_CASE_METHOD(cpu_test, "DEX")
{
    load(prgadr, std::array{0xca}); // INX
    cpu.x.assign(0x42);

    tick(2);
    CHECK((int)cpu.x.value() == 0x41);
}

TEST_CASE_METHOD(cpu_test, "INY")
{
    load(prgadr, std::array{0xc8}); // INY
    cpu.y.assign(0x42);

    tick(2);
    CHECK((int)cpu.y.value() == 0x43);
}

TEST_CASE_METHOD(cpu_test, "DEY")
{
    load(prgadr, std::array{0x88}); // DEY
    cpu.y.assign(0x42);

    tick(2);
    CHECK((int)cpu.y.value() == 0x41);
}

TEST_CASE_METHOD(cpu_test, "ASL-ACC")
{
    load(prgadr, std::array{0x0a}); // ASL A

    SECTION("Bit 1")
    {
        cpu.a.assign(0x02);

        tick(2);
        CHECK((int)cpu.a.value() == 0x4);

    }
    SECTION("Bit 7")
    {
        cpu.a.assign(0x80);

        tick(2);
        CHECK(cpu.p.test(nes::cpu_flag::zero));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
}

TEST_CASE_METHOD(cpu_test, "ASL-ZPX")
{
    load(prgadr, std::array{0x16, 0x10}); // ASL $10,X
    cpu.x.assign(0x3);
    load(0x0013, std::array{0x55});

    tick(6);
    CHECK((int)mem[0x0013] == 0xAA);
}

TEST_CASE_METHOD(cpu_test, "LSR")
{
    load(prgadr, std::array{0x4a}); // LSR A

    SECTION("Bit 1")
    {
        cpu.a.assign(0x2);

        tick(2);
        CHECK((int)cpu.a.value() == 0x1);
    }
    SECTION("Bit 0")
    {
        cpu.a.assign(0x1);

        tick(2);
        CHECK((int)cpu.a.value() == 0x0);
        CHECK(cpu.p.test(nes::cpu_flag::zero));
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
}

TEST_CASE_METHOD(cpu_test, "ROL")
{
    load(prgadr, std::array{0x2a}); // ROL A

    SECTION("Bit 0")
    {
        cpu.a.assign(0x01);

        tick(2);
        CHECK((int)cpu.a.value() == 0x2);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("Bit 7")
    {
        cpu.a.assign(0x80);

        tick(2);
        CHECK((int)cpu.a.value() == 0x00);
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("Bit 8 (Carry)")
    {
        cpu.a.assign(0x00);
        cpu.p.set(nes::cpu_flag::carry);

        tick(2);
        CHECK((int)cpu.a.value() == 0x01);
    }
}

TEST_CASE_METHOD(cpu_test, "ROR")
{
    load(prgadr, std::array{0x6a}); // ROR A

    SECTION("Bit 7")
    {
        cpu.a.assign(0x80);

        tick(2);
        CHECK((int)cpu.a.value() == 0x40);
    }
    SECTION("Bit 8 (Carry)")
    {
        cpu.a.assign(0x00);
        cpu.p.set(nes::cpu_flag::carry);

        tick(2);
        CHECK((int)cpu.a.value() == 0x80);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
    }
    SECTION("Bit 0")
    {
        cpu.a.assign(0x01);

        tick(2);
        CHECK((int)cpu.a.value() == 0x00);
        CHECK(cpu.p.test(nes::cpu_flag::carry));
    }
}

TEST_CASE_METHOD(cpu_test, "LAX")
{
    load(prgadr, std::array{0xa7, 0x10}); // LAX $10
    load(0x0010, std::array{0x27});

    tick(3);
    CHECK((int)cpu.a.value() == 0x27);
    CHECK((int)cpu.x.value() == 0x27);
}

TEST_CASE_METHOD(cpu_test, "SAX")
{
    load(prgadr, std::array{0x87, 0x10}); // SAX $10
    cpu.a.assign(0x03);
    cpu.x.assign(0x0e);

    tick(3);

    CHECK((int)mem[0x0010] == 0x02);
}

TEST_CASE_METHOD(cpu_test, "DCP")
{
    load(prgadr, std::array{0xC7, 0x10}); // DCP $10
    load(0x0010, std::array{0x43});
    cpu.a.assign(0x42);

    tick(5);

    CHECK((int)mem[0x0010] == 0x42);
    CHECK(cpu.p.test(nes::cpu_flag::zero));
    CHECK((int)cpu.a.value() == 0x42);
}


TEST_CASE_METHOD(cpu_test, "ISC")
{
    load(prgadr, std::array{0xE7, 0x10}); // ISC $10
    load(0x0010, std::array{0x41});
    cpu.a.assign(0x42);
    cpu.p.set(nes::cpu_flag::carry);

    tick(5);

    CHECK((int)mem[0x0010] == 0x42);
    CHECK((int)cpu.a.value() == 0x00);
    CHECK(cpu.p.test(nes::cpu_flag::zero));
}

TEST_CASE_METHOD(cpu_test, "SLO")
{
    load(prgadr, std::array{0x07, 0x10}); // SLO $10
    load(0x0010, std::array{0x03});
    cpu.a.assign(0x1);

    tick(5);

    CHECK((int)mem[0x0010] == 0x06);
    CHECK((int)cpu.a.value() == 0x07);
}

TEST_CASE_METHOD(cpu_test, "SLO, Carry bit")
{
    load(prgadr, std::array{0x03, 0x45}); // SLO ($45,X)
    load(0x0047, std::array{0x47, 0x06});
    load(0x0647, std::array{0xA5});
    cpu.x.assign(0x02);
    cpu.a.assign(0xB3);

    tick(8);

    CHECK((int)mem[0x0647] == 0x4A);
    CHECK((int)cpu.a.value() == 0xFB);
    CHECK(cpu.p.test(nes::cpu_flag::carry));
}

TEST_CASE_METHOD(cpu_test, "SRE")
{
    load(prgadr, std::array{0x47, 0x10}); // SRE $10
    load(0x0010, std::array{0x06});
    cpu.a.assign(0x1);

    tick(5);

    CHECK((int)mem[0x0010] == 0x03);
    CHECK((int)cpu.a.value() == 0x02);
}

TEST_CASE_METHOD(cpu_test, "RLA")
{
    load(prgadr, std::array{0x27, 0x10}); // RLA, $10

    SECTION("Bit 0")
    {
        load(0x0010, std::array{0x01});
        cpu.a.assign(0xFF);

        tick(5);

        CHECK((int)mem[0x0010] == 0x2);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
        CHECK((int)cpu.a.value() == 0x2);
    }
    SECTION("Bit 7")
    {
        load(0x0010, std::array{0x80});
        cpu.a.assign(0xFF);

        tick(5);

        CHECK((int)mem[0x0010] == 0x00);
        CHECK(cpu.p.test(nes::cpu_flag::carry));
        CHECK((int)cpu.a.value() == 0x00);
    }
    SECTION("Bit 8 (Carry)")
    {
        load(0x0010, std::array{0x00});
        cpu.p.set(nes::cpu_flag::carry);
        cpu.a.assign(0xFF);

        tick(5);

        CHECK((int)mem[0x0010] == 0x01);
        CHECK((int)cpu.a.value() == 0x01);
    }
}

TEST_CASE_METHOD(cpu_test, "RRA, Carry")
{
    load(prgadr, std::array{0x67, 0x10}); // RRA, $10
    load(0x010, std::array{0xa5});
    cpu.a.assign(0xb2);
    cpu.p.assign(0xe4);

    tick(5);

    CHECK((int)cpu.a.value() == 0x05);
    CHECK((int)cpu.p.value() == 0x25);
}

TEST_CASE_METHOD(cpu_test, "RRA")
{
    load(prgadr, std::array{0x67, 0x10}); // RRA, $10

    SECTION("Bit 7")
    {
        load(0x0010, std::array{0x80});
        cpu.a.assign(0x01);

        tick(5);

        CHECK((int)mem[0x0010] == 0x40);
        CHECK((int)cpu.a.value() == 0x41);
    }
    SECTION("Bit 8 (Carry)")
    {
        load(0x0010, std::array{0x00});
        cpu.p.set(nes::cpu_flag::carry);
        cpu.a.assign(0x01);

        tick(5);

        CHECK((int)mem[0x0010] == 0x80);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
        CHECK((int)cpu.a.value() == 0x81);
    }
    SECTION("Bit 0")
    {
        load(0x0010, std::array{0x01});
        cpu.a.assign(0x01);

        tick(5);

        CHECK((int)mem[0x0010] == 0x00);
        CHECK_FALSE(cpu.p.test(nes::cpu_flag::carry));
        CHECK((int)cpu.a.value() == 0x02);
    }
}