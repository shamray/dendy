#include "libnes/mos6502.h"

#include <cassert>
#include <stdexcept>
#include <unordered_map>

namespace nes
{

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

auto imp = [](mos6502& ) -> uint8_t {
    throw std::logic_error("Calling operand function for implied addressing mode");
};


// Instruction set lookup table

const std::unordered_map<uint8_t, mos6502::instruction> instruction_set {
    {0xA9, { lda, imm, 2 }},
    {0xA5, { lda, zp , 3 }},

    {0xA2, { ldx, imm, 2 }},
    {0xA6, { ldx, zp , 3 }},
    {0x00, { brk, imp, 7 }}
};

}

mos6502::mos6502(std::vector<uint8_t>& memory)
    : memory_(memory)
{}

void mos6502::tick()
{
    auto opcode = read(pc++);
    auto cmd = decode(opcode).value_or(instruction{});
    cmd.op(*this, cmd.arg);
}

auto mos6502::decode(uint8_t opcode) -> std::optional<instruction>
{
    auto found = instruction_set.find(opcode);
    if (found == std::end(instruction_set))
        return std::nullopt;

    return found->second;
}

}