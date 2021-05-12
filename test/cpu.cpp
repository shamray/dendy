#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "libnes/mos6502.h"

class fixture
{
public:
    fixture()
        : mem(64 * 1024, 0)
    {
        assert(mem.size() == 64 * 1024);
    }

    template <class T>
    void program(uint16_t addr, T&& program)
    {
        std::copy(
            std::begin(program), std::end(program),
            mem.begin() + addr
        );
    }

    std::vector<uint8_t> mem;

};

TEST_CASE_METHOD(fixture, "LDA-IMM")
{
    nes::mos6502 cpu(mem);

    program(0x8000, std::vector<uint8_t>{0xa9, 0x55});
    cpu.tick();

    CHECK(cpu.a == 0x55);
}