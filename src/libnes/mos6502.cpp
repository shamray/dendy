#include "libnes/mos6502.h"

#include <cassert>
#include <stdexcept>
#include <unordered_map>
#include <string>

using namespace std::string_literals;

namespace nes
{

class unsupported_opcode : public std::runtime_error
{
public:
    unsupported_opcode(uint8_t opcode)
        : std::runtime_error("Unsupported opcode: "s + std::to_string(opcode))
    {}
};

namespace
{

// Operations

auto adc = [](mos6502& cpu, auto operand)
{
};

auto lda = [](mos6502& cpu, auto operand) {
    cpu.a = operand(cpu);
};

auto ldx = [](mos6502& cpu, auto operand) {
    cpu.x = operand(cpu);
};

auto brk = [](mos6502& cpu, auto operand) {
    // TODO: set brake flag
};


// Addressing modes

auto imm = [](mos6502& cpu) {
    return cpu.read(cpu.pc++);
};

auto zp =  [](mos6502& cpu) {
    auto address = cpu.read(cpu.pc++);
    return cpu.read(address);
};

auto zpx = [](mos6502& cpu) {
    auto address = cpu.read(cpu.pc++);
    return cpu.read(cpu.x + address);
};

auto zpy = [](mos6502& cpu) {
    auto address = cpu.read(cpu.pc++);
    return cpu.read(cpu.y + address);
};

auto abs = [](mos6502& cpu) {
    return uint8_t{};
};

auto abx = [](mos6502& cpu) {
    return uint8_t{};
};

auto aby = [](mos6502& cpu) {
    return uint8_t{};
};

auto izx = [](mos6502& cpu) {
    auto index = cpu.read(cpu.pc++) + cpu.x;
    auto address = cpu.read_word(index);
    return cpu.read(address);
};

auto izy = [](mos6502& cpu) {
    auto index = cpu.read(cpu.pc++);
    auto address = cpu.read_word(index) + cpu.y;
    return cpu.read(address);
};

auto imp = [](mos6502& ) -> uint8_t {
    throw std::logic_error("Calling operand function for implied addressing mode");
};


// Instruction set lookup table

const std::unordered_map<uint8_t, mos6502::instruction> instruction_set {

    {0xA9, { lda, imm, 2 }},
    {0xA5, { lda, zp , 3 }},
    {0xB5, { lda, zpx, 4 }},
    {0xAD, { lda, abs, 4 }},
    {0xBD, { lda, abx, 4 }},
    {0xB9, { lda, aby, 4 }},
    {0xA1, { lda, izx, 6 }},
    {0xB1, { lda, izy, 5 }},

    {0xA2, { ldx, imm, 2 }},
    {0xA6, { ldx, zp , 3 }},
    {0xB6, { ldx, zpy, 3 }},

    {0x00, { brk, imp, 7 }}
};

}

mos6502::mos6502(std::vector<uint8_t>& memory)
    : memory_(memory)
{}

void mos6502::tick()
{
    auto opcode = read(pc++);
    auto cmd = decode(opcode)
        .value_or(instruction{
            [opcode](auto...) { throw unsupported_opcode(opcode); },
            imp,
            0
        });
    cmd.op(*this, cmd.arg);
}

auto mos6502::read_word(uint16_t addr) const -> uint16_t
{
    auto lo = read(addr);
    auto hi = read(addr + 1);

    return (hi << 8) | lo;
}

auto mos6502::decode(uint8_t opcode) -> std::optional<instruction>
{
    auto found = instruction_set.find(opcode);
    if (found == std::end(instruction_set))
        return std::nullopt;

    return found->second;
}

}