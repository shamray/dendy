#include "libnes/cpu.h"

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

auto adc = [](auto& cpu, auto operand)
{
};

auto lda = [](auto& cpu, auto operand) {
    cpu.a = operand(cpu);
};

auto ldx = [](auto& cpu, auto operand) {
    cpu.x = operand(cpu);
};

auto brk = [](auto& cpu, auto operand) {
    // TODO: set brake flag
};


// Addressing modes

auto imm = [](auto& cpu) {
    return cpu.read(cpu.pc++);
};

auto zp =  [](auto& cpu) {
    auto address = cpu.read(cpu.pc++);
    return cpu.read(address);
};

auto zpx = [](auto& cpu) {
    auto address = cpu.read(cpu.pc++);
    return cpu.read(cpu.x + address);
};

auto zpy = [](auto& cpu) {
    auto address = cpu.read(cpu.pc++);
    return cpu.read(cpu.y + address);
};

auto abs = [](auto& cpu) {
    auto address = cpu.read_word(cpu.pc);
    cpu.pc += 2;
    return cpu.read(address);
};

auto abx = [](auto& cpu) {
    auto address = cpu.read_word(cpu.pc) + cpu.x;
    cpu.pc += 2;
    return cpu.read(address);
};

auto aby = [](auto& cpu) {
    auto address = cpu.read_word(cpu.pc) + cpu.y;
    cpu.pc += 2;
    return cpu.read(address);
};

auto izx = [](auto& cpu) {
    auto index = cpu.read(cpu.pc++) + cpu.x;
    auto address = cpu.read_word(index);
    return cpu.read(address);
};

auto izy = [](auto& cpu) {
    auto index = cpu.read(cpu.pc++);
    auto address = cpu.read_word(index) + cpu.y;
    return cpu.read(address);
};

auto imp = [](auto& ) -> uint8_t {
    throw std::logic_error("Calling operand function for implied addressing mode");
};


// Instruction set lookup table

const std::unordered_map<uint8_t, cpu::instruction> instruction_set {

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

cpu::cpu(std::vector<uint8_t>& memory)
    : memory_{memory}
{
    pc = read_word(0xFFFC);
}

void cpu::tick()
{
    auto opcode = read(pc++);

    auto instruction = decode(opcode)
        .value_or(cpu::instruction{
            [opcode](auto...) { throw unsupported_opcode(opcode); },
            imp
        });

    instruction.execute(*this);
}

auto cpu::read_word(uint16_t addr) const -> uint16_t
{
    auto lo = read(addr);
    auto hi = read(addr + 1);

    return (hi << 8) | lo;
}

auto cpu::decode(uint8_t opcode) -> std::optional<instruction>
{
    auto found = instruction_set.find(opcode);
    if (found == std::end(instruction_set))
        return std::nullopt;

    return found->second;
}

}